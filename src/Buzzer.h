#ifndef BYBYTE_BUZZER_H
#define BYBYTE_BUZZER_H

#include <Arduino.h>
#include "core/ByByteCore.h"

namespace ByByte {

// Non-blocking buzzer API (free functions, single global pin).
//
// Why free functions: a buzzer is a single passive horn driven by tone/noTone.
// There is no need for multiple instances or per-instance state, so a class
// wrapper would only add overhead. State lives file-scope in Buzzer.cpp.
//
// Usage:
//   buzzerBegin();            // uses BYBYTE_HORN_PIN from config
//   buzzerTone(1000, 300);    // 1 kHz for 300 ms
//   buzzerPatternCarHorn();  // start a non-blocking pattern
//   buzzerUpdate();           // call often (e.g. from loop)

// Initialize the buzzer pin. Defaults to the platform horn pin from ByByteConfig.
void buzzerBegin(uint8_t pin = BYBYTE_HORN_PIN);

// Start a tone at freqHz for durationMs (0 = sustain until buzzerNoTone()).
void buzzerTone(uint16_t freqHz, uint16_t durationMs = 0);

// Stop any tone / pattern immediately.
void buzzerNoTone();

// Drive non-blocking patterns. Start one, then keep calling buzzerUpdate().
void buzzerPatternCarHorn(uint16_t repeat = 2, uint16_t baseHz = 440);
void buzzerPatternSiren(uint16_t repeat = 2);
void buzzerPatternR2D2();
void buzzerPatternClick();
void buzzerPatternHappy();
void buzzerPatternSad();
void buzzerPatternSurprise();
void buzzerPatternDisconnect();
void buzzerPatternButton();

// Advance finite-duration tones and pattern sequences. Call from loop().
void buzzerUpdate();

} // namespace ByByte

#endif // BYBYTE_BUZZER_H
