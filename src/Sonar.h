#ifndef BYBYTE_SONAR_H
#define BYBYTE_SONAR_H

#include <Arduino.h>
#include "core/ByByteCore.h"
#include "core/PcintManager.h"

namespace ByByte {

	class Sonar {
	public:
		Sonar()
			: Sonar(BYBYTE_SONAR_TRIG_PIN, BYBYTE_SONAR_ECHO_PIN, BYBYTE_SONAR_MAX_CM) {}

		Sonar(uint8_t trigPin, uint8_t echoPin)
			: Sonar(trigPin, echoPin, BYBYTE_SONAR_MAX_CM) {}

		Sonar(uint8_t trigPin, uint8_t echoPin, uint16_t maxRangeCm)
			: _trig(trigPin), _echo(echoPin), _maxCm(maxRangeCm), _state(Idle),
			_lastCm(0), _riseUs(0), _triggerStartedUs(0), _lastPingMs(0), _pcintNumber(0xFF) {}

		void begin() {
			pinMode(_trig, OUTPUT);
			pinMode(_echo, INPUT);
			digitalWrite(_trig, LOW);
			_lastCm = 0;
			_state = Idle;
			_riseUs = 0;
			_triggerStartedUs = 0;
			_lastPingMs = 0;
			instance(this);

			#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
			// Mega uses a direct pulseIn-based measurement because its sonar pins are not on PCINT-capable ports.
			#else
			_pcintNumber = resolvePcintNumber(_echo);
			if (_pcintNumber != 0xFF) {
				PcintManager::subscribePin(_echo, &Sonar::onPcintStatic, false);
			}
			#endif
		}

		void end() {
			#if BYBYTE_PLATFORM_ID != BYBYTE_PLATFORM_MEGA
			if (_pcintNumber != 0xFF) {
				PcintManager::unsubscribe(_pcintNumber);
			}
			#endif
		}

		// Returns last measured distance in cm (0 means out-of-range/timeout)
		uint16_t readCm() {
			#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
			unsigned long nowMs = millis();
			if ((nowMs - _lastPingMs) >= 70UL) {
				trigger();
				unsigned long echoUs = pulseIn(_echo, HIGH, 30000UL);
				if (echoUs > 0) {
					unsigned int cm = (unsigned int)(echoUs / 58UL);
					_lastCm = (cm > _maxCm) ? 0 : cm;
				} else {
					_lastCm = 0;
				}
				_lastPingMs = nowMs;
				_state = Idle;
			}
			return _lastCm;
			#else
			if (_state == Idle) {
				unsigned long nowMs = millis();
				if ((nowMs - _lastPingMs) >= 70UL) {
					trigger();
				}
			} else {
				checkTimeout();
			}
			return _lastCm;
			#endif
		}

	private:
		enum State { Idle, WaitHigh, Measuring };

		static void onPcintStatic() { if (instance()) instance()->onPcint(); }

		static Sonar* instance(Sonar* set = nullptr) {
			static Sonar* self = nullptr;
			if (set) self = set;
			return self;
		}

		static uint8_t resolvePcintNumber(uint8_t pin) {
			volatile uint8_t* pcmsk = digitalPinToPCMSK(pin);
			if (!pcmsk) {
				return 0xFF;
			}
			uint8_t group = digitalPinToPCICRbit(pin);
			uint8_t bit = digitalPinToPCMSKbit(pin);
			return static_cast<uint8_t>(group * 8 + bit);
		}

		void trigger() {
			digitalWrite(_trig, LOW);
			delayMicroseconds(2);
			digitalWrite(_trig, HIGH);
			delayMicroseconds(10);
			digitalWrite(_trig, LOW);
			_triggerStartedUs = micros();
			_lastPingMs = millis();
			_state = WaitHigh;
		}

		void checkTimeout() {
			unsigned long nowUs = micros();
			if (_state == WaitHigh && (nowUs - _triggerStartedUs) > 30000UL) {
				_lastCm = 0;
				_state = Idle;
			} else if (_state == Measuring && (nowUs - _riseUs) > 30000UL) {
				_lastCm = 0;
				_state = Idle;
			}
		}

		void onPcint() {
			if (_state == Idle) return;
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

	private:
		uint8_t _trig;
		uint8_t _echo;
		uint16_t _maxCm;
		volatile State _state;
		volatile uint16_t _lastCm;
		volatile unsigned long _riseUs;
		volatile unsigned long _triggerStartedUs;
		volatile unsigned long _lastPingMs;
		volatile uint8_t _pcintNumber;

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

