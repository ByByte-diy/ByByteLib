#include "MotorDriver.h"

#include "core/ByByteCore.h"
#include "core/PinCapabilities.h"

namespace ByByte {

namespace {

// True when the caller supplied explicit pins (any field non-zero).
bool pinsProvided(const MotorPins& p) {
	return p.leftIn1 || p.leftIn2 || p.rightIn1 || p.rightIn2 ||
	       p.leftPwm || p.rightPwm || p.stby;
}

// Hardware defaults for the compiled platform (empty when unsupported).
MotorPins platformDefaultPins() {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO
	return MotorPins::drv8833(
		BYBYTE_NANO_LEFT_IN1, BYBYTE_NANO_LEFT_IN2,
		BYBYTE_NANO_RIGHT_IN1, BYBYTE_NANO_RIGHT_IN2);
#elif BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	return MotorPins::tb6612(
		BYBYTE_TB6612_LEFT_IN1, BYBYTE_TB6612_LEFT_IN2, BYBYTE_TB6612_LEFT_PWM,
		BYBYTE_TB6612_RIGHT_IN1, BYBYTE_TB6612_RIGHT_IN2, BYBYTE_TB6612_RIGHT_PWM,
		BYBYTE_TB6612_STBY);
#else
	return MotorPins{};
#endif
}

// Use caller pins when at least one is set, otherwise fall back to platform defaults.
MotorPins resolvePins(const MotorPins& given) {
	return pinsProvided(given) ? given : platformDefaultPins();
}

} // namespace

MotorDriver::MotorDriver(ControlMode mode)
	: _pins(resolvePins(MotorPins{})),
	  _mode(mode),
	  _target{},
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	  _backend(_pins.stby, _pins.leftIn1, _pins.leftIn2, _pins.leftPwm,
	           _pins.rightIn1, _pins.rightIn2, _pins.rightPwm),
#else
	  _backend(_pins.leftIn1, _pins.leftIn2, _pins.rightIn1, _pins.rightIn2),
#endif
	  _diff(_backend, BYBYTE_WHEEL_SEPARATION_M, BYBYTE_WHEEL_RADIUS_M, BYBYTE_MAX_PWM),
	  _begun(false) {}

MotorDriver::MotorDriver(const MotorPins& pins, ControlMode mode)
	: _pins(resolvePins(pins)),
	  _mode(mode),
	  _target{},
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	  _backend(_pins.stby, _pins.leftIn1, _pins.leftIn2, _pins.leftPwm,
	           _pins.rightIn1, _pins.rightIn2, _pins.rightPwm),
#else
	  _backend(_pins.leftIn1, _pins.leftIn2, _pins.rightIn1, _pins.rightIn2),
#endif
	  _diff(_backend, BYBYTE_WHEEL_SEPARATION_M, BYBYTE_WHEEL_RADIUS_M, BYBYTE_MAX_PWM),
	  _begun(false) {}

DriverType MotorDriver::driverType() const noexcept {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	return DriverType::TB6612;
#else
	return DriverType::DRV8833;
#endif
}

bool MotorDriver::resolvedPinsOk() const {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	return _pins.stby != 0 &&
	       _pins.leftIn1 != 0 && _pins.leftIn2 != 0 && _pins.leftPwm != 0 &&
	       _pins.rightIn1 != 0 && _pins.rightIn2 != 0 && _pins.rightPwm != 0;
#else
	return _pins.leftIn1 != 0 && _pins.leftIn2 != 0 &&
	       _pins.rightIn1 != 0 && _pins.rightIn2 != 0;
#endif
}

bool MotorDriver::pwmPinsOk() const {
#if defined(ARDUINO)
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	return isPwmPin(_pins.leftPwm) && isPwmPin(_pins.rightPwm);
#else
	return isPwmPin(_pins.leftIn1) && isPwmPin(_pins.leftIn2) &&
	       isPwmPin(_pins.rightIn1) && isPwmPin(_pins.rightIn2);
#endif
#else
	return true;
#endif
}

bool MotorDriver::begin() {
	if (_begun) return true;
	if (!resolvedPinsOk()) return false;
	if (!pwmPinsOk()) return false;
	if (!_backend.begin()) return false;
	_begun = true;
	return true;
}

void MotorDriver::setMotorSpeeds(int16_t left, int16_t right) {
	_mode = ControlMode::Direct;
	if (!_begun) return;
	_backend.setMotorSpeeds(left, right);
}

void MotorDriver::setTargetVelocity(const Twist& cmd) {
	_mode = ControlMode::Differential;
	_target = cmd;
}

void MotorDriver::setTargetVelocity(float linearX, float angularZ) {
	Twist t;
	t.linearX = linearX;
	t.angularZ = angularZ;
	setTargetVelocity(t);
}

void MotorDriver::update() {
	if (!_begun) return;
	if (_mode == ControlMode::Differential) {
		_diff.setTargetVelocity(_target);
		_diff.update();
	}
}

void MotorDriver::forward(int16_t speed) { setMotorSpeeds(speed, speed); }
void MotorDriver::backward(int16_t speed) { setMotorSpeeds(-speed, -speed); }
void MotorDriver::left(int16_t speed) { setMotorSpeeds(0, speed); }
void MotorDriver::right(int16_t speed) { setMotorSpeeds(speed, 0); }
void MotorDriver::turnLeft(int16_t speed) { setMotorSpeeds(-speed, speed); }
void MotorDriver::turnRight(int16_t speed) { setMotorSpeeds(speed, -speed); }
void MotorDriver::stop() { setMotorSpeeds(0, 0); }

} // namespace ByByte
