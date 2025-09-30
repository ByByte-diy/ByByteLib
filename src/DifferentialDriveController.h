#ifndef BYBYTE_DIFF_DRIVE_CONTROLLER_H
#define BYBYTE_DIFF_DRIVE_CONTROLLER_H

#include <Arduino.h>
#include "Types.h"
#include "MotorController.h"

namespace ByByte {

class DifferentialDriveController {
public:
	DifferentialDriveController(MotorController& motorController,
			float wheelSeparation, float wheelRadius, int16_t maxPwm = 255);
	void setTargetVelocity(const Twist& cmd);
	void update();
private:
	MotorController& _motorController;
	float _wheelSeparation;
	float _wheelRadius;
	int16_t _maxPwm;
	Twist _target;
};

} // namespace ByByte

#endif // BYBYTE_DIFF_DRIVE_CONTROLLER_H

