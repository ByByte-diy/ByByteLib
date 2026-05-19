#ifndef BYBYTE_MOTOR_CONTROLLER_H
#define BYBYTE_MOTOR_CONTROLLER_H

#include <Arduino.h>

namespace ByByte {

/** Abstract brushed dual-motor backend (DRV8833/TB6612 implementations). */
class MotorController {
public:
	MotorController() = default;
	virtual ~MotorController() = default;
	virtual bool begin() = 0;
	virtual void setMotorSpeeds(int16_t left, int16_t right) = 0;

	MotorController(const MotorController&) = delete;
	MotorController& operator=(const MotorController&) = delete;
};

} // namespace ByByte

#endif // BYBYTE_MOTOR_CONTROLLER_H
