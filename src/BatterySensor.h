#ifndef BYBYTE_BATTERY_SENSOR_H
#define BYBYTE_BATTERY_SENSOR_H

#include <Arduino.h>
#include "ByByteConfig.h"

namespace ByByte {

class BatterySensor {
public:
	BatterySensor(uint8_t adcPin = BYBYTE_BAT_ADC_PIN, uint16_t vrefMv = BYBYTE_BAT_VREF_MV, uint8_t chrgPin = BYBYTE_CHRG_PIN)
		: _adcPin(adcPin), _vrefMv(vrefMv), _chrgPin(chrgPin) {}
	void begin() {
		if (_chrgPin != 0xFF) pinMode(_chrgPin, INPUT_PULLUP);
	}
	float readVoltage() {
		// Always DEFAULT (~5V) reference assumed by platform
		(uint16_t)analogRead(_adcPin); // dummy
		uint16_t raw = analogRead(_adcPin);
		return (float)_vrefMv * (float)raw / 1023.0f / 1000.0f;
	}
	bool isCharging() const {
		if (_chrgPin == 0xFF) return false;
		int v = digitalRead(_chrgPin);
		return (v == LOW);
	}
	// Simple linear SoC estimation for 1S Li-ion between 3.0V and 4.2V
	uint8_t estimateSocPercent(float voltage) const {
		if (voltage < 3.0f) return 0;
		if (voltage > 4.2f) return 100;
		return (uint8_t)((voltage - 3.0f) * (100.0f / 1.2f));
	}
	void setVrefMv(uint16_t mv) { _vrefMv = mv; }
private:
	uint8_t _adcPin;
	uint16_t _vrefMv;
	uint8_t _chrgPin;
};

} // namespace ByByte

#endif // BYBYTE_BATTERY_SENSOR_H
