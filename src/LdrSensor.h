#ifndef BYBYTE_LDR_SENSOR_H
#define BYBYTE_LDR_SENSOR_H

#include <Arduino.h>
#include "configs/ByByteConfig.h"

namespace ByByte {

class LdrSensor {
public:
	LdrSensor(uint8_t adcPin = BYBYTE_LDR_ADC_PIN, bool invert = false)
		: _adcPin(adcPin), _invert(invert), _min(1023), _max(0) {}
	void begin() {}
	uint16_t readRaw() {
		analogReference(DEFAULT);
		delay(2);
		(void)analogRead(_adcPin);
		return analogRead(_adcPin);
	}
	void autoCalibrate(uint16_t steps = 50, uint16_t delayMs = 20) {
		for (uint16_t i=0;i<steps;i++) { calibrateStep(); delay(delayMs); }
	}
	void calibrateStep() {
		uint16_t v = readRaw();
		if (v < _min) _min = v;
		if (v > _max) _max = v;
	}
	uint16_t readNormalized() {
		uint16_t v = readRaw();
		if (_max <= _min) return 0;
		long span = (long)_max - (long)_min; if (span < 5) span = 5;
		long num = _invert ? ((long)_max - (long)v) : ((long)v - (long)_min);
		if (num < 0) num = 0;
		if (num > span) num = span;
		return (uint16_t)((num * 1000L) / span);
	}
private:
	uint8_t _adcPin;
	bool _invert;
	uint16_t _min, _max;
};

} // namespace ByByte

#endif // BYBYTE_LDR_SENSOR_H
