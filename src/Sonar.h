#ifndef BYBYTE_SONAR_H
#define BYBYTE_SONAR_H

#include <Arduino.h>
#include "core/TimerManager.h"
#include "core/ByByteCore.h"
#include "core/PcintManager.h"

namespace ByByte {

class Sonar {
public:
	Sonar(uint8_t trigPin, uint8_t echoPin, uint16_t maxRangeCm = 400)
		: _trig(trigPin), _echo(echoPin), _maxCm(maxRangeCm), _state(Idle),
		  _lastCm(0), _riseUs(0), _startUs(0), _lastPingMs(0) {}

	void begin() {
		pinMode(_trig, OUTPUT);
		pinMode(_echo, INPUT);
		digitalWrite(_trig, LOW);
		instance(this);
		#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		// Use 1ms scheduler on Mega as well (disable Timer1 ISR to avoid conflicts)
		TimerManager::getInstance().subscribe(TimerInterval::MILLISECOND_1, &Sonar::onTickStatic, false);
		#elif BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO
		// Subscribe to PCINT for echo pin edges
		PcintManager::subscribe(_echo, &Sonar::onPcintStatic);
		// Periodic trigger via TimerManager 1ms (lightweight)
		TimerManager::getInstance().subscribe(TimerInterval::MILLISECOND_1, &Sonar::onTickStatic, false);
		#endif
	}

	void end() {
		#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		TimerManager::getInstance().unsubscribe(&Sonar::onTickStatic);
		#elif BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO
		TimerManager::getInstance().unsubscribe(&Sonar::onTickStatic);
		PcintManager::unsubscribe(_echo);
		#endif
	}

	// Returns last measured distance in cm (0 means out-of-range/timeout)
	uint16_t readCm() const { return _lastCm; }

private:
	enum State { Idle, WaitHigh, Measuring };

	static void onTickStatic() { if (instance()) instance()->onTick(); }
	static void onPcintStatic() { if (instance()) instance()->onPcint(); }

	static Sonar* instance(Sonar* set = nullptr) {
		static Sonar* self = nullptr;
		if (set) self = set;
		return self;
	}

	// Common 1ms scheduler (used for Nano TRIG, also safe for Mega)
	void onTick() {
		unsigned long nowMs = millis();
		if (_state == Idle && (nowMs - _lastPingMs >= 100)) {
			// Send 10us trigger pulse
			digitalWrite(_trig, HIGH);
			delayMicroseconds(10);
			digitalWrite(_trig, LOW);
			_startUs = micros();
			_lastPingMs = nowMs;
			_state = WaitHigh;
		}
	}

	// Nano: edge detection via PCINT
	void onPcint() {
		if (_state == Idle) return; // ignore stray
		uint8_t level = digitalRead(_echo);
		unsigned long nowUs = micros();
		if (_state == WaitHigh && level == HIGH) {
			_riseUs = nowUs;
			_state = Measuring;
			return;
		}
		if (_state == Measuring && level == LOW) {
			unsigned long durUs = nowUs - _riseUs;
			unsigned int cm = (unsigned int)(durUs / 58UL);
			_lastCm = (cm > _maxCm) ? 0 : cm;
			_state = Idle;
		}
	}

	// Timer1 polling implementation removed to avoid ISR conflicts

private:
	uint8_t _trig;
	uint8_t _echo;
	uint16_t _maxCm;
	volatile State _state;
	volatile uint16_t _lastCm;
	volatile unsigned long _riseUs;
	volatile unsigned long _startUs;
	volatile unsigned long _lastPingMs;

public:
	// Factory to register instance for static tick
	static Sonar* create(uint8_t trigPin, uint8_t echoPin, uint16_t maxRangeCm = 400) {
		Sonar* s = new Sonar(trigPin, echoPin, maxRangeCm);
		instance(s);
		return s;
	}
};

} // namespace ByByte

#endif // BYBYTE_SONAR_H

