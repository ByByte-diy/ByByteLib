#ifndef BYBYTE_MOTOR_UTILS_H
#define BYBYTE_MOTOR_UTILS_H

#include <Arduino.h>

namespace ByByte {

static inline int16_t clampPwm(int16_t value) {
	if (value > 255) return 255;
	if (value < -255) return -255;
	return value;
}

// DRV8833-style: dual-PWM H-bridge on in1/in2
static inline void writeHBridgePwm(uint8_t in1, uint8_t in2, int16_t value) {
	int16_t v = clampPwm(value);
	if (v >= 0) {
		analogWrite(in1, v);
		analogWrite(in2, 0);
	} else {
		analogWrite(in1, 0);
		analogWrite(in2, -v);
	}
}

// TB6612-style: DIR on in1/in2, PWM on pwmPin
static inline void writeTb6612Channel(uint8_t in1, uint8_t in2, uint8_t pwmPin, int16_t value) {
	int16_t v = clampPwm(value);
	if (v >= 0) {
		digitalWrite(in1, HIGH);
		digitalWrite(in2, LOW);
		analogWrite(pwmPin, v);
	} else {
		digitalWrite(in1, LOW);
		digitalWrite(in2, HIGH);
		analogWrite(pwmPin, -v);
	}
}

} // namespace ByByte

#endif // BYBYTE_MOTOR_UTILS_H
