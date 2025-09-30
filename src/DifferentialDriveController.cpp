#include "DifferentialDriveController.h"

namespace ByByte {

DifferentialDriveController::DifferentialDriveController(MotorController& motorController,
			float wheelSeparation, float wheelRadius, int16_t maxPwm)
	: _motorController(motorController), _wheelSeparation(wheelSeparation), _wheelRadius(wheelRadius), _maxPwm(maxPwm) {
	_target.linearX = 0.0f;
	_target.angularZ = 0.0f;
}

void DifferentialDriveController::setTargetVelocity(const Twist& cmd) {
	_target = cmd;
}

void DifferentialDriveController::update() {
	const float v = _target.linearX;
	const float w = _target.angularZ;
	const float halfL = _wheelSeparation * 0.5f;
	const float invR = 1.0f / _wheelRadius;

	const float wLeft = (v - w * halfL) * invR;
	const float wRight = (v + w * halfL) * invR;

	int16_t pwmLeft = (int16_t)wLeft;
	if (pwmLeft > _maxPwm) pwmLeft = _maxPwm; else if (pwmLeft < -_maxPwm) pwmLeft = -_maxPwm;
	int16_t pwmRight = (int16_t)wRight;
	if (pwmRight > _maxPwm) pwmRight = _maxPwm; else if (pwmRight < -_maxPwm) pwmRight = -_maxPwm;

	_motorController.setMotorSpeeds(pwmLeft, pwmRight);
}

} // namespace ByByte

