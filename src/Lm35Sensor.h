#ifndef BYBYTE_LM35_SENSOR_H
#define BYBYTE_LM35_SENSOR_H

#include <Arduino.h>
#include "ByByteConfig.h"

namespace ByByte {

class Lm35Sensor {
public:
	Lm35Sensor(uint8_t adcPin = BYBYTE_TMP_ADC_PIN) : _adcPin(adcPin) {}
	void begin() {}
	float readCelsius() {
		#if defined(__AVR_ATmega2560__)
		analogReference(INTERNAL1V1);
		#else
		analogReference(DEFAULT);
		#endif
		delay(5);
		(void)analogRead(_adcPin);
		uint16_t raw = analogRead(_adcPin);
		#if defined(__AVR_ATmega2560__)
		float mv = (float)raw * 1100.0f / 1023.0f;
		#else
		float mv = (float)raw * 5000.0f / 1023.0f;
		#endif
		return mv / 10.0f; // 10mV/°C
	}
	float readFahrenheit() {
		float c = readCelsius();
		return c * 9.0f / 5.0f + 32.0f;
	}
private:
	uint8_t _adcPin;
};

} // namespace ByByte

#endif // BYBYTE_LM35_SENSOR_H
