/*
 * ByByteLib - Auto Lamp with LDR + WS2812 (Mega/Nano)
 *
 * Purpose:
 * - Reads ambient light via LDR, auto-controls WS2812 headlights/tail lights
 * - Auto-calibrates LDR at startup for robust normalization
 *
 * Hardware:
 * - Mega: pin 23, 4 LEDs (0-1 headlights, 2-3 tails)
 * - Nano: pin 7, 2 LEDs (0-1 headlights)
 *
 * Module-only usage:
 * - Pulls in ByByteSensors + external NeoPixel lib.
 */
#include <LdrSensor.h>
#include <Adafruit_NeoPixel.h>

using namespace ByByte;

#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
static const uint8_t LED_PIN = 23;
static const uint16_t NUM_LEDS = 4;
#else
static const uint8_t LED_PIN = 7;
static const uint16_t NUM_LEDS = 2;
#endif

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
LdrSensor ldr(BYBYTE_LDR_ADC_PIN, true); // invert if needed

void setup() {
	Serial.begin(9600);
	strip.begin();
	strip.show();
	ldr.begin();
	ldr.autoCalibrate(100, 10);
}

void setHeadlights(bool on) {
	uint32_t front = on ? strip.Color(255, 255, 200) : strip.Color(0, 0, 0);
	uint32_t tail = on ? strip.Color(80, 0, 0) : strip.Color(0, 0, 0);
	strip.setPixelColor(0, front);
	strip.setPixelColor(1, front);
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	strip.setPixelColor(2, tail);
	strip.setPixelColor(3, tail);
#endif
	strip.show();
}

void loop() {
	uint16_t norm = ldr.readNormalized(); // 0..1000 (0 bright, 1000 dark if invert=true)
	bool dark = norm > 600; // threshold
	setHeadlights(dark);
	Serial.print(F("LDR norm=")); Serial.print(norm);
	Serial.print(F(" -> ")); Serial.println(dark ? F("LAMPS ON") : F("LAMPS OFF"));
	delay(100);
}