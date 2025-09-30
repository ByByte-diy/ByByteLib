#include "Buzzer.h"

namespace ByByte {

// Predefined sequences
static const uint16_t CAR_FREQ[] = { 440, 0, 480 };
static const uint16_t CAR_DUR[]  = { 350, 120, 350 };
static const uint8_t  CAR_LEN = 3;

// Intermittent alarm: on/off beeps at fixed tone
static const uint16_t SIREN_FREQ[] = { 1000, 0, 1000, 0, 1000, 0, 1000, 0 };
static const uint16_t SIREN_DUR[]  = { 200, 200, 200, 200, 200, 200, 200, 200 };
static const uint8_t  SIREN_LEN = sizeof(SIREN_FREQ)/sizeof(SIREN_FREQ[0]);

// R2D2-like chirps: fast upsweeps/downsweeps and short bleeps
static const uint16_t R2D2_FREQ[] = {
  1200, 1400, 1650, 1950, 2250,   // quick upsweep
  0,                              // short pause
  2100, 1700, 1300,               // down chirp
  0,                              // short pause
  1000, 1250, 1500, 1750, 2100,   // playful rising ladder
  0,
  2300, 1800, 1400, 1100          // fall
};
static const uint16_t R2D2_DUR[]  = {
   40,   40,   40,   40,   60,
   40,
   50,   50,   70,
   40,
   35,   35,   35,   35,   70,
   40,
   45,   45,   55,   80
};
static const uint8_t  R2D2_LEN = sizeof(R2D2_FREQ)/sizeof(R2D2_FREQ[0]);

static const uint16_t CLICK_FREQ[] = { 3000 };
static const uint16_t CLICK_DUR[]  = { 30 };
static const uint8_t  CLICK_LEN = 1;

Buzzer::Buzzer(uint8_t pin) : _pin(pin), _untilMs(0), _seqFreq(nullptr), _seqDur(nullptr), _seqLen(0), _seqIdx(0), _seqRepeat(0), _seqNextMs(0) {}

void Buzzer::begin() {
	pinMode(_pin, OUTPUT);
	stopPwm();
}

void Buzzer::startPwm(uint16_t freqHz) {
	if (freqHz == 0) { stopPwm(); return; }
	pinMode(_pin, OUTPUT);
	::tone(_pin, freqHz);
}

void Buzzer::stopPwm() {
	::noTone(_pin);
	digitalWrite(_pin, LOW);
}

void Buzzer::tone(uint16_t freqHz, uint16_t durationMs) {
	_seqFreq = nullptr; _seqDur = nullptr; _seqLen = 0; _seqIdx = 0; _seqRepeat = 0; _seqNextMs = 0;
	startPwm(freqHz);
	_untilMs = (durationMs == 0) ? 0 : (millis() + durationMs);
}

void Buzzer::noTone() {
	_untilMs = 0;
	stopPwm();
}

void Buzzer::startSequence(const uint16_t* f, const uint16_t* d, uint8_t n, uint16_t repeat) {
	_seqFreq = f; _seqDur = d; _seqLen = n; _seqIdx = 0; _seqRepeat = repeat; _seqNextMs = 0; _untilMs = 0;
}

void Buzzer::patternCarHorn(uint16_t repeat, uint16_t baseHz) {
	// Build temporary pattern using base pitch and slightly higher second beep
	_tmpFreq[0] = baseHz;
	_tmpDur[0]  = 350;
	_tmpFreq[1] = 0;     _tmpDur[1] = 120;
	_tmpFreq[2] = baseHz + 40; _tmpDur[2] = 350;
	startSequence(_tmpFreq, _tmpDur, 3, repeat);
}

void Buzzer::patternSiren(uint16_t repeat) {
	startSequence(SIREN_FREQ, SIREN_DUR, SIREN_LEN, repeat);
}

void Buzzer::patternR2D2() {
	startSequence(R2D2_FREQ, R2D2_DUR, R2D2_LEN, 1);
}

void Buzzer::patternClick() {
	startSequence(CLICK_FREQ, CLICK_DUR, CLICK_LEN, 1);
}

// OttoDIY-inspired cues (frequencies approximated; durations short for snappy cues)
void Buzzer::patternHappy() {
    // rising three-tone
    _tmpFreq[0]=800; _tmpDur[0]=120;
    _tmpFreq[1]=1000; _tmpDur[1]=120;
    _tmpFreq[2]=1300; _tmpDur[2]=180;
    startSequence(_tmpFreq, _tmpDur, 3, 1);
}

void Buzzer::patternSad() {
    // falling two-tone with pause
    _tmpFreq[0]=700; _tmpDur[0]=250;
    _tmpFreq[1]=0;   _tmpDur[1]=100;
    _tmpFreq[2]=500; _tmpDur[2]=300;
    startSequence(_tmpFreq, _tmpDur, 3, 1);
}

void Buzzer::patternSurprise() {
    // quick up-down
    _tmpFreq[0]=1200; _tmpDur[0]=100;
    _tmpFreq[1]=1600; _tmpDur[1]=120;
    _tmpFreq[2]=1000; _tmpDur[2]=140;
    startSequence(_tmpFreq, _tmpDur, 3, 1);
}

void Buzzer::patternDisconnect() {
    // two descending beeps
    _tmpFreq[0]=900; _tmpDur[0]=180;
    _tmpFreq[1]=0;   _tmpDur[1]=100;
    _tmpFreq[2]=700; _tmpDur[2]=220;
    startSequence(_tmpFreq, _tmpDur, 3, 1);
}

void Buzzer::patternButton() {
    // short double click
    _tmpFreq[0]=2500; _tmpDur[0]=30;
    _tmpFreq[1]=0;    _tmpDur[1]=40;
    _tmpFreq[2]=2500; _tmpDur[2]=30;
    startSequence(_tmpFreq, _tmpDur, 3, 1);
}

void Buzzer::update() {
	unsigned long now = millis();
	// Handle finite tone duration
	if (_untilMs && now >= _untilMs) {
		stopPwm();
		_untilMs = 0;
	}
	// Handle sequences
	if (_seqFreq && now >= _seqNextMs) {
		if (_seqIdx >= _seqLen) {
			if (_seqRepeat > 1) { _seqRepeat--; _seqIdx = 0; } else { stopPwm(); _seqFreq = nullptr; return; }
		}
		uint16_t f = _seqFreq[_seqIdx];
		uint16_t d = _seqDur[_seqIdx];
		_seqIdx++;
		_seqNextMs = now + d;
		startPwm(f);
	}
}

} // namespace ByByte


