#include "DifferentialDriveController.h"

namespace ByByte {

DifferentialDriveController::DifferentialDriveController(
	MotorController& motorController,
	float wheelSeparation,
	float wheelRadius,
	int16_t maxPwm)
	: _motorController(motorController),
	  _wheelSeparation(wheelSeparation),
	  _wheelRadius(wheelRadius),
	  _maxPwm(maxPwm) {
	_target.linearX = 0.0f;
	_target.angularZ = 0.0f;
}

void DifferentialDriveController::setTargetVelocity(const Twist& cmd) {
	_target = cmd;
}

void DifferentialDriveController::update() {
	const float v = _target.linearX;
	const float w = _target.angularZ;
	const float halfTrack = _wheelSeparation * 0.5f;
	// Twist + wheel dims map to commanded wheel rotational sense, then PWM (see Types.h notes).
	const float wheelRadius = _wheelRadius > 1e-6f ? _wheelRadius : 1e-6f;
	const float invR = 1.0f / wheelRadius;

	const float wLeft = (v - w * halfTrack) * invR;
	const float wRight = (v + w * halfTrack) * invR;

	int16_t pwmLeft = (int16_t)wLeft;
	if (pwmLeft > _maxPwm) pwmLeft = _maxPwm;
	else if (pwmLeft < -_maxPwm)
		pwmLeft = -_maxPwm;

	int16_t pwmRight = (int16_t)wRight;
	if (pwmRight > _maxPwm) pwmRight = _maxPwm;
	else if (pwmRight < -_maxPwm)
		pwmRight = -_maxPwm;

	_motorController.setMotorSpeeds(pwmLeft, pwmRight);
}

} // namespace ByByte
