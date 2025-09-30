#include "Servo.h"
#include "ServoManager.h"

namespace ByByte {

Servo::Servo() : _channel(-1), _pin(0), _continuous(false), _stopUs(1500), _deadbandUs(20), _minUs(500), _maxUs(2500) {}

bool Servo::attach(uint8_t pin, uint16_t minUs, uint16_t maxUs, uint8_t frameHz) {
	#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		_minUs = minUs; _maxUs = maxUs;
		ServoManager::begin(frameHz);
		for (uint8_t ch = 0; ch < 3; ++ch) {
			if (ServoManager::attach(ch, pin, minUs, maxUs)) { _channel = ch; _pin = pin; return true; }
		}
		return false;
	#else
		(void)pin; (void)minUs; (void)maxUs; (void)frameHz; return false;
	#endif
}

void Servo::detach() {
	#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		if (_channel >= 0) { ServoManager::detach((uint8_t)_channel); _channel = -1; }
	#endif
}

void Servo::write(uint8_t angle) {
	#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		if (_channel >= 0) {
			if (_continuous) {
				// In continuous mode, write() acts as speed shortcut: 0..180 -> -100..100
				int8_t sp = (int8_t)((int16_t)angle - 90) * 100 / 90;
				writeSpeed(sp);
			} else {
				ServoManager::writeAngle((uint8_t)_channel, angle);
			}
		}
	#else
		(void)angle;
	#endif
}

void Servo::writeMicroseconds(uint16_t us) {
	#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		if (_channel >= 0) ServoManager::writeMicros((uint8_t)_channel, us);
	#else
		(void)us;
	#endif
}

bool Servo::attached() const { return _channel >= 0; }

void Servo::setContinuous(bool enable, uint16_t stopUs, uint16_t deadbandUs) {
	_continuous = enable;
	_stopUs = stopUs;
	_deadbandUs = deadbandUs;
}

void Servo::writeSpeed(int8_t speedPercent) {
	#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		if (_channel < 0) return;
		if (speedPercent > 100) speedPercent = 100; if (speedPercent < -100) speedPercent = -100;
		if (!_continuous) {
			// If not in continuous mode, treat as angle 0..180 mapped to -100..100 already in write()
			int16_t us = _minUs + (int32_t)(_maxUs - _minUs) * (speedPercent + 100) / 200;
			ServoManager::writeMicros((uint8_t)_channel, (uint16_t)us);
			return;
		}
		// Continuous: center=stopUs, deadband around center
		if (speedPercent > -2 && speedPercent < 2) {
			ServoManager::writeMicros((uint8_t)_channel, _stopUs);
			return;
		}
		// Map -100..100 to [minUs..stopUs-deadband] and [stopUs+deadband..maxUs]
		uint16_t us;
		if (speedPercent > 0) {
			us = _stopUs + _deadbandUs + (uint32_t)(_maxUs - (_stopUs + _deadbandUs)) * speedPercent / 100;
		} else {
			int8_t sp = -speedPercent;
			uint16_t maxLeft = (_stopUs > _deadbandUs) ? (_stopUs - _deadbandUs) : _stopUs;
			us = maxLeft - (uint32_t)(maxLeft - _minUs) * sp / 100;
		}
		ServoManager::writeMicros((uint8_t)_channel, us);
	#else
		(void)speedPercent;
	#endif
}

} // namespace ByByte
