#include <Adafruit_NeoPixel.h>
#include <Adafruit_BluefruitLE_SPI.h>

// --- HARDWARE ---
#define LED_PIN 6
#define LED_COUNT 72 // Total # of neopixels.

// Bluefruit SPI pins.
#define BLUEFRUIT_SPI_CS 8 // Chip select.
#define BLUEFRUIT_SPI_IRQ 7 // Interrupt request.
#define BLUEFRUIT_SPI_RST 4 // Reset.

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// --- LED LAYOUT ---
const uint8_t numRows = 4;
const int rowStarts[] = {0, 19, 38, 55};  // Starting index of each row.
const int rowLengths[] = {19, 19, 17, 17}; // Length of each row.
const bool rowReversed[] = {false, true, false, true}; // Shows whether the rows are reversed.

// --- COLORS ---
const uint32_t hotPink = strip.Color(255, 0, 127);  // Hot pink for heart eyes.

// Multiple shades of blue for sparkle.
const uint32_t blueShades[] = {
  strip.Color(0, 71, 171),
  strip.Color(0, 119, 190),
  strip.Color(30, 144, 255),
  strip.Color(65, 105, 225),
  strip.Color(100, 149, 237),
  strip.Color(135, 206, 250),
  strip.Color(70, 130, 180),
  strip.Color(0, 191, 255),
  strip.Color(25, 25, 112),
  strip.Color(72, 61, 139)
};
const uint8_t numBlueShades = 10;

// Multiple shades of green for the Matrix effect.
const uint32_t matrixGreenShades[] = {
  strip.Color(0, 255, 65),
  strip.Color(0, 200, 50),
  strip.Color(0, 150, 40),
  strip.Color(0, 120, 30),
  strip.Color(0, 90, 25),
  strip.Color(0, 60, 15),
  strip.Color(0, 40, 10),
  strip.Color(0, 20, 5)
};
const uint8_t numMatrixGreenShades = 8;

// --- ANIMATION MODES ---
enum AnimationMode {
  HEART_BLINK,
  RAINBOW_WATERFALL,
  BLUE_SPARKLE,
  MATRIX_GREEN
};
AnimationMode currentMode = HEART_BLINK;

// --- HEART BLINKING ---
unsigned long lastBlinkTime = 0; // Keeps track of when the last blink cycle started.
const unsigned long blinkInterval = 3500; // Total blink cycle time.
const unsigned long eyesOpenTime = 2000;
const unsigned long halfCloseTime = 2100;
const unsigned long fullCloseTime = 2250;
const unsigned long halfOpenTime = 2350;

// The LEDs that form the heart eyes.
const int heartPixels[] = {
  3, 4, 6, 7, // Left heart top.
  34, 33, 32, 31, 30, // Left heart middle.
  41, 42, 43, // Left heart lower.
  67, // Left heart point.
  11, 12, 14, 15, // Right heart top.
  26, 25, 24, 23, 22, // Right heart middle.
  49, 50, 51, // Right heart lower.
  59 // Right heart point.
};
const uint8_t numHeartPixels = 26;

const int halfClosedPixels[] = {
  4, 6, // Left heart top.
  33, 32, 31, // Left heart middle.
  42, // Left heart lower.
  12, 14, // Right heart top.
  25, 24, 23, // Right heart middle.
  50 // Right heart lower.
};
const uint8_t numHalfClosedPixels = 12;

const int closedPixels[] = {
  4, 5, 6, // Left eye.
  12, 13, 14 // Right eye.
};
const uint8_t numClosedPixels = 6;

// --- RAINBOW WATERFALL ---
uint8_t rainbowOffset = 0; // Current color position (0 - 255).
unsigned long lastRainbowUpdate = 0;
const uint8_t rainbowSpeed = 10; // Ms between each frame (lower = faster).

// --- BLUE SPARKLES ---
const uint8_t maxSparkles = 50;
unsigned long lastSparkleUpdate = 0;
const uint8_t fadeAmount = 25;
const uint8_t spawnAttempts = 3;
const uint8_t spawnChance = 95;
const uint8_t sparkleSpeed = 15; // Ms between each frame (lower = faster).
uint8_t sparklePixels[maxSparkles]; // Represents which LED each sparkle is on.
int sparkleBrightness[maxSparkles]; // Fades the sparkle from 255 -> 0.
uint32_t sparkleColors[maxSparkles]; // Gives a random blue shade for each sparkle.
bool sparkleActive[maxSparkles]; // Shows if the slot is in use or not.

// --- MATRIX RAIN ---
const uint8_t maxDrops = 15;
unsigned long lastMatrixUpdate = 0;
const uint8_t matrixSpeed = 50; // Ms between each frame (lower = faster).
const uint8_t matrixSpawnChance = 40;
const uint8_t maxColumn = 19;
const uint8_t minTrailLength = 3;
const uint8_t maxTrailLength = 6;
struct MatrixDrop {
  uint8_t column; // X position (0 - 18).
  uint8_t position; // Y position (0 - 3).
  uint8_t length; // Fading trail behind (makes it look like Matrix rain).
  bool active;
};
MatrixDrop matrixDrops[maxDrops];

void setup() {
  Serial.begin(115200);
  
  // Initialize NeoPixels.
  strip.begin();
  strip.setBrightness(50);
  strip.show();
  
  // Initialize sparkle arrays.
  for (int i = 0; i < maxSparkles; i++) {
    sparkleActive[i] = false;
    sparkleBrightness[i] = 0;
    sparkleColors[i] = blueShades[0];
  }
  
  // Initialize matrix drops.
  for (int i = 0; i < maxDrops; i++) {
    matrixDrops[i].active = false;
    matrixDrops[i].column = 0;
    matrixDrops[i].position = 0;
  }
  
  // Initialize Bluefruit.
  if (!ble.begin(false)) {
    Serial.println(F("Bluefruit not found"));
    while (1);
  }
  
  ble.echo(false); // Disables echo command.
  ble.verbose(false); // Disables debug output.
  ble.sendCommandCheckOK(F("AT+BleHIDEn=On"));
  ble.reset(); // Apply Bluetooth settings.
}

void loop() {
  ble.println("AT+BLEUARTRX"); // Checks for Bluetooth button presses.
  ble.readline(); // Read response into buffer.
  if (strcmp(ble.buffer, "OK") != 0) { // Check to see if data received.
    String command = String(ble.buffer);
    handleBluetoothCommand(command);
  }
  
  // Run animation based on current mode.
  switch (currentMode) {
    case HEART_BLINK:
      runHeartBlinkAnimation();
      break;
    case RAINBOW_WATERFALL:
      runRainbowWaterfall();
      break;
    case BLUE_SPARKLE:
      runBlueSparkle();
      break;
    case MATRIX_GREEN:
      runMatrixGreen();
      break;
  }
  
  delay(10);
}

void handleBluetoothCommand(String cmd) {
  cmd.trim();
  
  // Button 1 for heart eyes.
  if (cmd.startsWith("!B1")) {
    currentMode = HEART_BLINK;
    lastBlinkTime = millis(); // Reset the blink cycle.
  }
  // Button 2 for rainbow effect.
  else if (cmd.startsWith("!B2")) {
    currentMode = RAINBOW_WATERFALL;
  }
  // Button 3 for blue sparkles.
  else if (cmd.startsWith("!B3")) {
    currentMode = BLUE_SPARKLE;
  }
  // Button 4 for Matrix.
  else if (cmd.startsWith("!B4")) {
    currentMode = MATRIX_GREEN;
  }
}

void runHeartBlinkAnimation() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastBlinkTime >= blinkInterval) {
    lastBlinkTime = currentTime;
  }
  
  unsigned long elapsed = currentTime - lastBlinkTime;
  
  if (elapsed < eyesOpenTime) {
    heartEyes();
  } else if (elapsed < halfCloseTime) {
    halfClosedHearts();
  } else if (elapsed < fullCloseTime) {
    closedHearts();
  } else if (elapsed < halfOpenTime) {
    halfClosedHearts();
  } else {
    heartEyes();
  }
}

// Helper to convert row and column to LED index.
int getPixelIndex(int row, int col) {
  if (rowReversed[row]) {
    return rowStarts[row] + (rowLengths[row] - 1 - col);
  }
  return rowStarts[row] + col;
}

void runRainbowWaterfall() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastRainbowUpdate >= rainbowSpeed) {
    lastRainbowUpdate = currentTime;
    
    for (int row = 0; row < numRows; row++) {
      for (int col = 0; col < rowLengths[row]; col++) {
        int hue = (rainbowOffset + (row * 16) + (col * 4)) % 256; // Offsets by row and column for wave effect.
        strip.setPixelColor(getPixelIndex(row, col), Wheel(hue));
      }
    }
    
    strip.show();
    rainbowOffset = (rainbowOffset + 2) % 256;
  }
}

void runBlueSparkle() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSparkleUpdate >= sparkleSpeed) {
    lastSparkleUpdate = currentTime;
    
    strip.clear();
    
    // Fade existing sparkles.
    for (int i = 0; i < maxSparkles; i++) {
      if (sparkleActive[i]) {
        sparkleBrightness[i] -= fadeAmount;
        if (sparkleBrightness[i] <= 0) {
          sparkleActive[i] = false;
          sparkleBrightness[i] = 0;
        } else {
          // Scale RGB by brightness.
          uint32_t color = sparkleColors[i];
          uint8_t r = ((color >> 16) & 0xFF) * sparkleBrightness[i] / 255;
          uint8_t g = ((color >> 8) & 0xFF) * sparkleBrightness[i] / 255;
          uint8_t b = (color & 0xFF) * sparkleBrightness[i] / 255;
          strip.setPixelColor(sparklePixels[i], strip.Color(r, g, b));
        }
      }
    }
    
    // Spawn more sparkles.
    for (int spawn = 0; spawn < spawnAttempts; spawn++) {
      if (random(100) < spawnChance) {
        for (int i = 0; i < maxSparkles; i++) {
          if (!sparkleActive[i]) {
            sparklePixels[i] = random(LED_COUNT);
            sparkleBrightness[i] = 255;
            sparkleColors[i] = blueShades[random(numBlueShades)];
            sparkleActive[i] = true;
            break;
          }
        }
      }
    }
    
    strip.show();
  }
}

void drawMatrixDrop(MatrixDrop &drop) {
  for (int row = 0; row < numRows; row++) {
    int dropRow = drop.position - row;
    if (dropRow < 0 || dropRow >= numRows) continue;
    if (drop.column >= rowLengths[dropRow]) continue;
    
    int trailPos = min(drop.position - dropRow, numMatrixGreenShades - 1);
    strip.setPixelColor(getPixelIndex(dropRow, drop.column), matrixGreenShades[trailPos]);
  }
}

void runMatrixGreen() {
  if (millis() - lastMatrixUpdate < matrixSpeed) return;
  lastMatrixUpdate = millis();
  
  // Fade to 75%.
  for (int i = 0; i < LED_COUNT; i++) {
    uint32_t c = strip.getPixelColor(i);
    strip.setPixelColor(i, strip.Color(
      ((c >> 16) & 0xFF) * 6 / 8,
      ((c >> 8) & 0xFF) * 6 / 8,
      (c & 0xFF) * 6 / 8
    ));
  }
  
  // Update drops.
  for (int i = 0; i < maxDrops; i++) {
    if (!matrixDrops[i].active) continue;
    matrixDrops[i].position++;
    if (matrixDrops[i].position > numRows + matrixDrops[i].length) {
      matrixDrops[i].active = false;
    } else {
      drawMatrixDrop(matrixDrops[i]);
    }
  }
  
  // Spawn new drop.
  if (random(100) < matrixSpawnChance) {
    for (int i = 0; i < maxDrops; i++) {
      if (!matrixDrops[i].active) {
        matrixDrops[i].column = random(maxColumn);
        matrixDrops[i].position = 0;
        matrixDrops[i].length = random(minTrailLength, maxTrailLength);
        matrixDrops[i].active = true;
        break;
      }
    }
  }
  
  strip.show();
}

// Input a value 0 to 255 to get a color value.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void heartEyes() {
  strip.clear();
  for (int i = 0; i < numHeartPixels; i++) {
    strip.setPixelColor(heartPixels[i], hotPink);
  }
  strip.show();
}

void halfClosedHearts() {
  strip.clear();
  for (int i = 0; i < numHalfClosedPixels; i++) {
    strip.setPixelColor(halfClosedPixels[i], hotPink);
  }
  strip.show();
}

void closedHearts() {
  strip.clear();
  for (int i = 0; i < numClosedPixels; i++) {
    strip.setPixelColor(closedPixels[i], hotPink);
  }
  strip.show();
}