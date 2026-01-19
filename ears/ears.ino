#include <Adafruit_NeoPixel.h>

// --- HARDWARE ---
#define RING1_PIN 6
#define RING2_PIN 7
#define RING_COUNT 12 // # of LEDs per ring.

const uint8_t ringBrightness = 40;
const uint8_t rainbowCycles = 5; // # of times to loop through the rainbow.
const uint8_t rainbowSpeed = 5; // Ms between frames (lower = faster).

Adafruit_NeoPixel ring1(RING_COUNT, RING1_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel ring2(RING_COUNT, RING2_PIN, NEO_GRBW + NEO_KHZ800);

void setup() {
  ring1.begin();
  ring2.begin();
  ring1.setBrightness(ringBrightness);
  ring2.setBrightness(ringBrightness);
  ring1.show();
  ring2.show();
}

void loop() {
  rainbowCycle(rainbowSpeed);
}

// Cycle both rings through the rainbow.
void rainbowCycle(int wait) {
  for (int i = 0; i < 256 * rainbowCycles; i++) {
    for (int j = 0; j < RING_COUNT; j++) {
      uint32_t color = Wheel(((j * 256 / RING_COUNT) + i) & 255);
      ring1.setPixelColor(j, color);
      ring2.setPixelColor(j, color);
    }
    ring1.show();
    ring2.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return ring1.Color(255 - WheelPos * 3, 0, WheelPos * 3, 0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return ring1.Color(0, WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  WheelPos -= 170;
  return ring1.Color(WheelPos * 3, 255 - WheelPos * 3, 0, 0);
}