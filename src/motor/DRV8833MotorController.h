#ifndef BYBYTE_DRV8833_MOTOR_CONTROLLER_H
#define BYBYTE_DRV8833_MOTOR_CONTROLLER_H

#include <Arduino.h>
#include "MotorController.h"

namespace ByByte {

/** DRV8833 dual-H-bridge backend: one in1/in2 pair per side, each pin is PWM-capable. */
class DRV8833MotorController : public MotorController {
public:
	DRV8833MotorController(uint8_t leftIn1, uint8_t leftIn2,
	                       uint8_t rightIn1, uint8_t rightIn2);
	bool begin() override;
	void setMotorSpeeds(int16_t left, int16_t right) override;

private:
	void driveHBridgePwm(uint8_t in1, uint8_t in2, int16_t value);
	uint8_t _lIn1;
	uint8_t _lIn2;
	uint8_t _rIn1;
	uint8_t _rIn2;
};

} // namespace ByByte

#endif // BYBYTE_DRV8833_MOTOR_CONTROLLER_H
