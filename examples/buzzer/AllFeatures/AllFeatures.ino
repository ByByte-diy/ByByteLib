/*
 * ByByteLib - Buzzer: All Features Demo (Mega/Nano)
 *
 * Shows: tone(), noTone(), and built-in patterns:
 *  - patternCarHorn(), patternSiren(), patternR2D2(), patternClick()
 *
 * Pins (from src/configs/ByByteConfig.h):
 *  - Mega: D45 (OC5B)
 *  - Nano: D11 (OC2A)
 */

#include <ByByteLib.h>


using namespace ByByte;

Buzzer buzzer; // uses BYBYTE_HORN_PIN from config

// Simple non-blocking state machine
static uint8_t stepIdx = 0;
static unsigned long stepUntil = 0;

static void startStep(uint8_t idx) {
  stepIdx = idx;
  unsigned long now = millis();
  switch (stepIdx) {
    case 0:
      Serial.println(F("Tone 1kHz 300ms"));
      buzzer.tone(1000, 300);
      stepUntil = now + 400;
      break;
    case 1:
      Serial.println(F("Tone 2kHz 300ms"));
      buzzer.tone(2000, 300);
      stepUntil = now + 400;
      break;
    case 2:
      Serial.println(F("Car horn"));
      buzzer.patternCarHorn(2);
      stepUntil = now + 1200;
      break;
    case 3:
      Serial.println(F("Siren"));
      buzzer.patternSiren(3);
      stepUntil = now + 2200;
      break;
    case 4:
      Serial.println(F("R2D2"));
      buzzer.patternR2D2();
      stepUntil = now + 1200;
      break;
    case 5:
      Serial.println(F("Click"));
      buzzer.patternClick();
      stepUntil = now + 100;
      break;
    case 6:
      Serial.println(F("Pause"));
      buzzer.noTone();
      stepUntil = now + 700;
      break;
    case 7:
      Serial.println(F("Happy"));
      buzzer.patternHappy();
      stepUntil = now + 500;
      break;
    case 8:
      Serial.println(F("Sad"));
      buzzer.patternSad();
      stepUntil = now + 600;
      break;
    case 9:
      Serial.println(F("Surprise"));
      buzzer.patternSurprise();
      stepUntil = now + 400;
      break;
    case 10:
      Serial.println(F("Disconnect"));
      buzzer.patternDisconnect();
      stepUntil = now + 600;
      break;
    case 11:
      Serial.println(F("Button click 2x"));
      buzzer.patternButton();
      stepUntil = now + 150;
      break;
    case 12:
      Serial.println(F("Car horn base=520Hz"));
      buzzer.patternCarHorn(2, 520);
      stepUntil = now + 900;
      break;
    default:
      stepIdx = 0; startStep(0); return;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("=== Buzzer All Features Demo ==="));
  buzzer.begin();
  Serial.print(F("Pin: ")); Serial.println((int)BYBYTE_HORN_PIN);
  startStep(0);
}

void loop() {
  unsigned long now = millis();
  // advance steps when time elapsed
  if (now >= stepUntil) {
    uint8_t next = stepIdx + 1;
    if (next > 12) next = 0;
    startStep(next);
  }
  // keep patterns progressing
  buzzer.update();
}


