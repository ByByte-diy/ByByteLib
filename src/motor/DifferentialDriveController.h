#ifndef BYBYTE_DIFF_DRIVE_CONTROLLER_H
#define BYBYTE_DIFF_DRIVE_CONTROLLER_H

#include "core/Types.h"
#include "MotorController.h"

namespace ByByte {

/**
 * Differential (left/right PWM) kinematics from Twist.linearX / Twist.angularZ.
 * Inputs are control units scaled with wheel geometry (see MotorDriver) and then
 * clamped to the configured PWM range.
 */
class DifferentialDriveController {
public:
	DifferentialDriveController(MotorController& motorController,
	                            float wheelSeparation,
	                            float wheelRadius,
	                            int16_t maxPwm = 255);

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
