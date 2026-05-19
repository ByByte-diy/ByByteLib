#ifndef BYBYTE_SIDE_IR_SENSORS_H
#define BYBYTE_SIDE_IR_SENSORS_H

#include <Arduino.h>
#include "configs/ByByteConfig.h"

namespace ByByte {

class SideIrSensors {
public:
	SideIrSensors(uint8_t powerPin = BYBYTE_IR_POWER_PIN,
				  uint8_t leftAnalogPin = BYBYTE_IR_LEFT_PIN,
				  uint8_t rightAnalogPin = BYBYTE_IR_RIGHT_PIN,
				  bool invertNormalized = true)
		: _powerPin(powerPin), _leftPin(leftAnalogPin), _rightPin(rightAnalogPin),
		  _leftMin(1023), _leftMax(0), _rightMin(1023), _rightMax(0), _invert(invertNormalized) {}

	void begin() {
		pinMode(_powerPin, OUTPUT);
		digitalWrite(_powerPin, LOW); // off by default
	}

	// Powers IR, samples analogs with averaging, powers off; returns raw readings
	void sample(uint16_t& leftRaw, uint16_t& rightRaw, uint16_t settleMs = 5, uint8_t samples = 4) {
		digitalWrite(_powerPin, HIGH);
		delay(settleMs);
		uint32_t accL = 0, accR = 0;
		for (uint8_t i = 0; i < samples; ++i) {
			accL += analogRead(_leftPin);
			accR += analogRead(_rightPin);
		}
		leftRaw = (uint16_t)(accL / samples);
		rightRaw = (uint16_t)(accR / samples);
		digitalWrite(_powerPin, LOW);
	}

	// Updates calibration ranges using current sample
	void calibrateStep() {
		uint16_t l, r; sample(l, r);
		if (l < _leftMin) _leftMin = l;
		if (l > _leftMax) _leftMax = l;
		if (r < _rightMin) _rightMin = r;
		if (r > _rightMax) _rightMax = r;
	}

	// Auto-calibration loop: user can call in setup while moving robot
	void autoCalibrate(uint16_t steps = 50, uint16_t delayMs = 20) {
		for (uint16_t i = 0; i < steps; ++i) { calibrateStep(); delay(delayMs); }
	}

	// Reads normalized values 0..1000 based on calibration
	// If invert=true (default), closer (lower voltage) -> higher normalized value
	void readNormalized(uint16_t& leftNorm, uint16_t& rightNorm) {
		uint16_t l, r; sample(l, r, 5, 4);
		leftNorm = normalize(l, _leftMin, _leftMax, _invert);
		rightNorm = normalize(r, _rightMin, _rightMax, _invert);
	}

	// Access to raw calibration
	uint16_t leftMin() const { return _leftMin; }
	uint16_t leftMax() const { return _leftMax; }
	uint16_t rightMin() const { return _rightMin; }
	uint16_t rightMax() const { return _rightMax; }

	void setInvert(bool invert) { _invert = invert; }
	bool invert() const { return _invert; }

private:
	static uint16_t normalize(uint16_t v, uint16_t vmin, uint16_t vmax, bool invert) {
		if (vmax <= vmin) return 0;
		long span = (long)vmax - (long)vmin;
		// Guard against too small span causing numeric flip
		if (span < 5) span = 5;
		long num = invert ? ((long)vmax - (long)v) : ((long)v - (long)vmin);
		if (num < 0) num = 0;
		if (num > span) num = span;
		long res = (num * 1000L) / span;
		if (res < 0) res = 0;
		if (res > 1000) res = 1000;
		return (uint16_t)res;
	}

	uint8_t _powerPin;
	uint8_t _leftPin;
	uint8_t _rightPin;
	uint16_t _leftMin, _leftMax;
	uint16_t _rightMin, _rightMax;
	bool _invert;
};

} // namespace ByByte

#endif // BYBYTE_SIDE_IR_SENSORS_H
