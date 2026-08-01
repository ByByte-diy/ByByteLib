#ifndef BYBYTE_BUZZER_H
#define BYBYTE_BUZZER_H

#include <Arduino.h>
#include "configs/ByByteConfig.h"

namespace ByByte {

// Non-blocking hardware-PWM buzzer
class Buzzer {
public:
    explicit Buzzer(uint8_t pin = BYBYTE_HORN_PIN);
    void begin();

    // Start tone at frequency (Hz) and optional duration (ms). duration=0 -> sustain
    void tone(uint16_t freqHz, uint16_t durationMs = 0);
    void noTone();

    // Call often (e.g., in loop) to stop tone when duration elapsed
    void update();

    // Built-in patterns (non-blocking): call, then keep calling update()
    void patternCarHorn(uint16_t repeat = 2, uint16_t baseHz = 440);   // two beeps, adjustable pitch
    void patternSiren(uint16_t repeat = 2);     // alternating hi/lo
    void patternR2D2();                         // fun sequence
    void patternClick();                        // short key click
    // Multi-tone cues
    void patternHappy();
    void patternSad();
    void patternSurprise();
    void patternDisconnect();
    void patternButton();

private:
    uint8_t _pin;
    uint32_t _untilMs;

    // pattern state
    const uint16_t* _seqFreq;
    const uint16_t* _seqDur;
    uint8_t _seqLen;
    uint8_t _seqIdx;
    uint16_t _seqRepeat;
    uint32_t _seqNextMs;
    bool _seqProgmem;   // true  -> _seqFreq/_seqDur live in PROGMEM (read via pgm_read_word)
                        // false -> they live in RAM (e.g. _tmpFreq/_tmpDur)

    // Temporary buffers for custom patterns (small fixed capacity)
    uint16_t _tmpFreq[8];
    uint16_t _tmpDur[8];

    void startPwm(uint16_t freqHz);
    void stopPwm();
    void startSequence(const uint16_t* f, const uint16_t* d, uint8_t n, uint16_t repeat, bool progmem);
};

} // namespace ByByte

#endif // BYBYTE_BUZZER_H


