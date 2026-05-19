#include "ServoManager.h"
#include "configs/ByByteConfig.h"

#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
#include <avr/io.h>
#include <avr/interrupt.h>

namespace ByByte {

// Default tick: 20 kHz -> 50us per tick
static const uint16_t DEFAULT_TICK_US = 50;
static const uint16_t DEFAULT_MIN_US = 1000;
static const uint16_t DEFAULT_MAX_US = 2000;

ServoManager::Channel ServoManager::_ch[3] = {
	{BYBYTE_SERVO0_PIN, nullptr, 0, (uint16_t)(DEFAULT_MIN_US / DEFAULT_TICK_US), DEFAULT_MIN_US, DEFAULT_MAX_US, false},
	{BYBYTE_SERVO1_PIN, nullptr, 0, (uint16_t)(DEFAULT_MIN_US / DEFAULT_TICK_US), DEFAULT_MIN_US, DEFAULT_MAX_US, false},
	{BYBYTE_SERVO2_PIN, nullptr, 0, (uint16_t)(DEFAULT_MIN_US / DEFAULT_TICK_US), DEFAULT_MIN_US, DEFAULT_MAX_US, false}
};
volatile uint16_t ServoManager::_frameTicks = 400; // 20ms @50us
volatile uint16_t ServoManager::_tick = 0;
uint16_t ServoManager::_tickUs = DEFAULT_TICK_US;
uint8_t ServoManager::_frameHz = 50;
bool ServoManager::_running = false;

static inline volatile uint8_t* pinToPort(uint8_t pin, uint8_t& bitMask) {
	volatile uint8_t* port = nullptr;
	switch (pin) {
		case 30: port = &PORTC; bitMask = _BV(7); break; // PC7
		case 31: port = &PORTC; bitMask = _BV(6); break; // PC6
		case 32: port = &PORTC; bitMask = _BV(5); break; // PC5
		default: port = &PORTC; bitMask = 0; break;
	}
	return port;
}

void ServoManager::begin(uint8_t frameHz) {
	_frameHz = (frameHz == 25) ? 25 : 50;
	recomputeFrame();
	ensureStarted();
}

void ServoManager::setFrameRate(uint8_t frameHz) {
	_frameHz = (frameHz == 25) ? 25 : 50;
	recomputeFrame();
}

bool ServoManager::attach(uint8_t channel, uint8_t pin, uint16_t minUs, uint16_t maxUs) {
	if (channel > 2) return false;
	if (_ch[channel].enabled) return false; // already in use, do not override
	if (minUs < 400) minUs = 400; if (maxUs > 2600) maxUs = 2600; if (minUs >= maxUs) { minUs = DEFAULT_MIN_US; maxUs = DEFAULT_MAX_US; }
	uint8_t mask;
	volatile uint8_t* port = pinToPort(pin, mask);
	if (!mask) return false;
	_ch[channel].pin = pin;
	_ch[channel].port = port;
	_ch[channel].bitMask = mask;
	_ch[channel].minUs = minUs;
	_ch[channel].maxUs = maxUs;
	_ch[channel].pulseTicks = (uint16_t)(minUs / _tickUs);
	_ch[channel].enabled = true;
	pinMode(pin, OUTPUT);
	ensureStarted();
	return true;
}

void ServoManager::detach(uint8_t channel) {
	if (channel > 2) return;
	_ch[channel].enabled = false;
}

void ServoManager::writeMicros(uint8_t channel, uint16_t micros) {
	if (channel > 2) return;
	if (!_ch[channel].enabled) return;
	if (micros < _ch[channel].minUs) micros = _ch[channel].minUs;
	if (micros > _ch[channel].maxUs) micros = _ch[channel].maxUs;
	_ch[channel].pulseTicks = (uint16_t)(micros / _tickUs);
}

void ServoManager::writeAngle(uint8_t channel, uint8_t degrees) {
	if (channel > 2) return;
	if (!_ch[channel].enabled) return;
	if (degrees > 180) degrees = 180;
	uint16_t us = _ch[channel].minUs + (uint32_t)(_ch[channel].maxUs - _ch[channel].minUs) * degrees / 180;
	writeMicros(channel, us);
}

void ServoManager::ensureStarted() {
	if (_running) return;
	// Setup timer at 20kHz fixed tick (50us) regardless of frame rate
	cli();
	TCCR3A = 0;
	TCCR3B = (1<<WGM32) | (1<<CS31);
	OCR3A = 99;
	TCNT3 = 0;
	TIFR3 = (1<<OCF3A);
	TIMSK3 |= (1<<OCIE3A);
	sei();
	_running = true;
}

void ServoManager::recomputeFrame() {
	_tickUs = DEFAULT_TICK_US; // keep tick constant
	uint16_t frameUs = (_frameHz == 25) ? 40000 : 20000;
	_frameTicks = frameUs / _tickUs;
	if (_frameTicks < 300) _frameTicks = 300; // guard
}

void ServoManager::isrTick() {
	if (_tick == 0) {
		for (uint8_t i=0;i<3;i++) if (_ch[i].enabled) { *_ch[i].port |= _ch[i].bitMask; }
	}
	for (uint8_t i=0;i<3;i++) {
		if (_ch[i].enabled && _tick == _ch[i].pulseTicks) { *_ch[i].port &= ~_ch[i].bitMask; }
	}
	if (++_tick >= _frameTicks) {
		_tick = 0;
	}
}

ISR(TIMER3_COMPA_vect) {
	ServoManager::isrTick();
}

} // namespace ByByte

#endif // MEGA
