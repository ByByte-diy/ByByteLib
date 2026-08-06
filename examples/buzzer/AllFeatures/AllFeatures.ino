/*
 * ByByteLib - Buzzer: All Features Demo (Mega/Nano)
 *
 * Shows: buzzerTone(), buzzerNoTone(), and built-in patterns:
 *  - buzzerPatternCarHorn(), buzzerPatternSiren(), buzzerPatternR2D2(),
 *    buzzerPatternClick(), plus emotion cues.
 *
 * The buzzer is exposed as free functions (no class, no singleton): call
 * buzzerBegin(), then start tones/patterns and buzzerUpdate() from loop().
 *
 * Pins (from ByByteCore configs/ByByteConfig.h):
 *  - Mega: D45 (OC5B)
 *  - Nano: D11 (OC2A)
 */
#include <Buzzer.h>

using namespace ByByte;

// Simple non-blocking state machine
static uint8_t stepIdx = 0;
static unsigned long stepUntil = 0;

static void startStep(uint8_t idx) {
	stepIdx = idx;
	unsigned long now = millis();
	switch (stepIdx) {
		case 0:
			Serial.println(F("Tone 1kHz 300ms"));
			buzzerTone(1000, 300);
			stepUntil = now + 400;
			break;
		case 1:
			Serial.println(F("Tone 2kHz 300ms"));
			buzzerTone(2000, 300);
			stepUntil = now + 400;
			break;
		case 2:
			Serial.println(F("Car horn"));
			buzzerPatternCarHorn(2);
			stepUntil = now + 1200;
			break;
		case 3:
			Serial.println(F("Siren"));
			buzzerPatternSiren(3);
			stepUntil = now + 2200;
			break;
		case 4:
			Serial.println(F("R2D2"));
			buzzerPatternR2D2();
			stepUntil = now + 1200;
			break;
		case 5:
			Serial.println(F("Click"));
			buzzerPatternClick();
			stepUntil = now + 100;
			break;
		case 6:
			Serial.println(F("Pause"));
			buzzerNoTone();
			stepUntil = now + 700;
			break;
		case 7:
			Serial.println(F("Happy"));
			buzzerPatternHappy();
			stepUntil = now + 500;
			break;
		case 8:
			Serial.println(F("Sad"));
			buzzerPatternSad();
			stepUntil = now + 600;
			break;
		case 9:
			Serial.println(F("Surprise"));
			buzzerPatternSurprise();
			stepUntil = now + 400;
			break;
		case 10:
			Serial.println(F("Disconnect"));
			buzzerPatternDisconnect();
			stepUntil = now + 600;
			break;
		case 11:
			Serial.println(F("Button click 2x"));
			buzzerPatternButton();
			stepUntil = now + 150;
			break;
		case 12:
			Serial.println(F("Car horn base=520Hz"));
			buzzerPatternCarHorn(2, 520);
			stepUntil = now + 900;
			break;
		default:
			stepIdx = 0; startStep(0); return;
	}
}

void setup() {
	Serial.begin(115200);
	Serial.println(F("=== Buzzer All Features Demo ==="));
	buzzerBegin(); // uses BYBYTE_HORN_PIN from config
	Serial.print(F("Pin: ")); Serial.println((int)BYBYTE_HORN_PIN);
	startStep(0);
}

void loop() {
	unsigned long now = millis();
	if (now >= stepUntil) {
		uint8_t next = stepIdx + 1;
		if (next > 12) next = 0;
		startStep(next);
	}
	buzzerUpdate();
}