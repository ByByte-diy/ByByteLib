#include "Buzzer.h"

namespace ByByte {

// ---------------------------------------------------------------------------
// Built-in tone tables — stored in flash (PROGMEM) to save SRAM.
// Indexing must go through readSeq() / pgm_read_word(); do NOT index these
// tables directly with operator[] on AVR (Harvard architecture).
// ---------------------------------------------------------------------------
static const uint16_t CAR_FREQ[] PROGMEM = { 440, 0, 480 };
static const uint16_t CAR_DUR[]  PROGMEM = { 350, 120, 350 };
static const uint8_t  CAR_LEN    = sizeof(CAR_FREQ) / sizeof(CAR_FREQ[0]);

static const uint16_t SIREN_FREQ[] PROGMEM = { 1000, 0, 1000, 0, 1000, 0, 1000, 0 };
static const uint16_t SIREN_DUR[]  PROGMEM = { 200, 200, 200, 200, 200, 200, 200, 200 };
static const uint8_t  SIREN_LEN   = sizeof(SIREN_FREQ) / sizeof(SIREN_FREQ[0]);

static const uint16_t R2D2_FREQ[] PROGMEM = {
	1200, 1400, 1650, 1950, 2250,
	0,
	2100, 1700, 1300,
	0,
	1000, 1250, 1500, 1750, 2100,
	0,
	2300, 1800, 1400, 1100
};
static const uint16_t R2D2_DUR[] PROGMEM = {
	40, 40, 40, 40, 60,
	40,
	50, 50, 70,
	40,
	35, 35, 35, 35, 70,
	40,
	45, 45, 55, 80
};
static const uint8_t R2D2_LEN = sizeof(R2D2_FREQ) / sizeof(R2D2_FREQ[0]);

static const uint16_t CLICK_FREQ[] PROGMEM = { 3000 };
static const uint16_t CLICK_DUR[]  PROGMEM = { 30 };
static const uint8_t  CLICK_LEN   = sizeof(CLICK_FREQ) / sizeof(CLICK_FREQ[0]);

// ---------------------------------------------------------------------------
// Single global buzzer state (one horn pin per board).
// ---------------------------------------------------------------------------
static uint8_t g_pin = 0;
static uint32_t g_untilMs = 0;

// Pattern/sequence player state.
static const uint16_t* g_seqFreq;
static const uint16_t* g_seqDur;
static uint8_t g_seqLen;
static uint8_t g_seqIdx;
static uint16_t g_seqRepeat;
static uint32_t g_seqNextMs;
static bool g_seqProgmem; // true  -> tables in PROGMEM (pgm_read_word)
                         // false -> tables in RAM (g_tmpFreq/g_tmpDur)

// Temporary buffers for patterns whose pitch is a runtime argument.
static uint16_t g_tmpFreq[8];
static uint16_t g_tmpDur[8];

namespace {
inline uint16_t readSeq(const uint16_t* p, uint8_t i, bool progmem) {
	if (progmem) return pgm_read_word(p + i);
	return p[i];
}

void startPwm(uint16_t freqHz) {
	if (freqHz == 0) { ::noTone(g_pin); digitalWrite(g_pin, LOW); return; }
	pinMode(g_pin, OUTPUT);
	::tone(g_pin, freqHz);
}

void stopPwm() {
	::noTone(g_pin);
	digitalWrite(g_pin, LOW);
}

void startSequence(const uint16_t* f, const uint16_t* d, uint8_t n, uint16_t repeat, bool progmem) {
	g_seqFreq = f;
	g_seqDur = d;
	g_seqLen = n;
	g_seqIdx = 0;
	g_seqRepeat = repeat;
	g_seqNextMs = 0;
	g_untilMs = 0;
	g_seqProgmem = progmem;
}
} // namespace

void buzzerBegin(uint8_t pin) {
	g_pin = pin;
	pinMode(g_pin, OUTPUT);
	stopPwm();
}

void buzzerTone(uint16_t freqHz, uint16_t durationMs) {
	g_seqFreq = nullptr;
	g_seqLen = 0;
	g_seqIdx = 0;
	g_seqRepeat = 0;
	g_seqNextMs = 0;
	g_seqProgmem = false;
	startPwm(freqHz);
	g_untilMs = (durationMs == 0) ? 0 : (millis() + durationMs);
}

void buzzerNoTone() {
	g_untilMs = 0;
	g_seqProgmem = false;
	stopPwm();
}

void buzzerPatternCarHorn(uint16_t repeat, uint16_t baseHz) {
	g_tmpFreq[0] = baseHz;          g_tmpDur[0] = 350;
	g_tmpFreq[1] = 0;               g_tmpDur[1] = 120;
	g_tmpFreq[2] = baseHz + 40;     g_tmpDur[2] = 350;
	startSequence(g_tmpFreq, g_tmpDur, CAR_LEN, repeat, /*progmem=*/false);
}

void buzzerPatternSiren(uint16_t repeat) {
	startSequence(SIREN_FREQ, SIREN_DUR, SIREN_LEN, repeat, /*progmem=*/true);
}

void buzzerPatternR2D2() {
	startSequence(R2D2_FREQ, R2D2_DUR, R2D2_LEN, 1, /*progmem=*/true);
}

void buzzerPatternClick() {
	startSequence(CLICK_FREQ, CLICK_DUR, CLICK_LEN, 1, /*progmem=*/true);
}

void buzzerPatternHappy() {
	g_tmpFreq[0] = 800;  g_tmpDur[0] = 120;
	g_tmpFreq[1] = 1000; g_tmpDur[1] = 120;
	g_tmpFreq[2] = 1300; g_tmpDur[2] = 180;
	startSequence(g_tmpFreq, g_tmpDur, 3, 1, /*progmem=*/false);
}

void buzzerPatternSad() {
	g_tmpFreq[0] = 700;  g_tmpDur[0] = 250;
	g_tmpFreq[1] = 0;    g_tmpDur[1] = 100;
	g_tmpFreq[2] = 500;  g_tmpDur[2] = 300;
	startSequence(g_tmpFreq, g_tmpDur, 3, 1, /*progmem=*/false);
}

void buzzerPatternSurprise() {
	g_tmpFreq[0] = 1200; g_tmpDur[0] = 100;
	g_tmpFreq[1] = 1600; g_tmpDur[1] = 120;
	g_tmpFreq[2] = 1000; g_tmpDur[2] = 140;
	startSequence(g_tmpFreq, g_tmpDur, 3, 1, /*progmem=*/false);
}

void buzzerPatternDisconnect() {
	g_tmpFreq[0] = 900;  g_tmpDur[0] = 180;
	g_tmpFreq[1] = 0;    g_tmpDur[1] = 100;
	g_tmpFreq[2] = 700;  g_tmpDur[2] = 220;
	startSequence(g_tmpFreq, g_tmpDur, 3, 1, /*progmem=*/false);
}

void buzzerPatternButton() {
	g_tmpFreq[0] = 2500; g_tmpDur[0] = 30;
	g_tmpFreq[1] = 0;    g_tmpDur[1] = 40;
	g_tmpFreq[2] = 2500; g_tmpDur[2] = 30;
	startSequence(g_tmpFreq, g_tmpDur, 3, 1, /*progmem=*/false);
}

void buzzerUpdate() {
	unsigned long now = millis();

	// Finite single-tone duration.
	if (g_untilMs && now >= g_untilMs) {
		stopPwm();
		g_untilMs = 0;
	}

	// Pattern/sequence stepping.
	if (g_seqFreq && now >= g_seqNextMs) {
		if (g_seqIdx >= g_seqLen) {
			if (g_seqRepeat > 1) { g_seqRepeat--; g_seqIdx = 0; }
			else { stopPwm(); g_seqFreq = nullptr; return; }
		}
		uint16_t f = readSeq(g_seqFreq, g_seqIdx, g_seqProgmem);
		uint16_t d = readSeq(g_seqDur,  g_seqIdx, g_seqProgmem);
		g_seqIdx++;
		g_seqNextMs = now + d;
		startPwm(f);
	}
}

} // namespace ByByte
