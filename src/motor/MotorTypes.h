#ifndef BYBYTE_MOTOR_TYPES_H
#define BYBYTE_MOTOR_TYPES_H

#include <Arduino.h>

namespace ByByte {

// Motor driver silicon. Selected automatically for the compiled platform
// (TB6612 on Mega, DRV8833 otherwise). Kept as a value only for introspection.
enum class DriverType { DRV8833, TB6612 };

// Direct       : raw per-wheel PWM (-255..255).
// Differential : (linearX, angularZ) Twist kinematics, applied on update().
enum class ControlMode { Direct, Differential };

// Pin bundle covering both driver families.
//   DRV8833 : left/right in1/in2 (the in pins double as PWM on this driver)
//   TB6612  : direction on in1/in2 plus a dedicated PWM per side and a STBY line
struct MotorPins {
	uint8_t leftIn1 = 0, leftIn2 = 0, rightIn1 = 0, rightIn2 = 0;
	uint8_t leftPwm = 0, rightPwm = 0, stby = 0;

	static MotorPins drv8833(uint8_t l1, uint8_t l2, uint8_t r1, uint8_t r2) {
		MotorPins p;
		p.leftIn1 = l1;
		p.leftIn2 = l2;
		p.rightIn1 = r1;
		p.rightIn2 = r2;
		return p;
	}

	static MotorPins tb6612(uint8_t lIn1, uint8_t lIn2, uint8_t lPwm,
	                        uint8_t rIn1, uint8_t rIn2, uint8_t rPwm,
	                        uint8_t standby) {
		MotorPins p;
		p.leftIn1 = lIn1;
		p.leftIn2 = lIn2;
		p.leftPwm = lPwm;
		p.rightIn1 = rIn1;
		p.rightIn2 = rIn2;
		p.rightPwm = rPwm;
		p.stby = standby;
		return p;
	}
};

} // namespace ByByte

#endif // BYBYTE_MOTOR_TYPES_H
