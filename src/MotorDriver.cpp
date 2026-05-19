#include "MotorDriver.h"

#include "configs/ByByteConfig.h"
#include "PinCapabilities.h"
#include "DRV8833MotorController.h"
#include "TB6612MotorController.h"

namespace ByByte {

DriverType MotorDriver::driverForBuildTarget() {
	#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	return DriverType::TB6612;
	#else
	return DriverType::DRV8833;
	#endif
}

MotorDriver::MotorDriver(DriverType driver, ControlMode mode)
	: _motor(nullptr),
	  _diffDrive(nullptr),
	  _mode(mode),
	  _driverType(driver) {
	_target.linearX = 0;
	_target.angularZ = 0;
}

MotorDriver::MotorDriver(DriverType driver, const MotorPins& pins, ControlMode mode)
	: _motor(nullptr),
	  _diffDrive(nullptr),
	  _mode(mode),
	  _driverType(driver),
	  _pins(pins) {
	_target.linearX = 0;
	_target.angularZ = 0;
}

MotorDriver::~MotorDriver() {
	delete _diffDrive;
	_diffDrive = nullptr;
	delete _motor;
	_motor = nullptr;
}

void MotorDriver::resolveDefaults() {
	if (_driverType == DriverType::DRV8833) {
		if (_pins.leftIn1 == 0 && _pins.leftIn2 == 0 && _pins.rightIn1 == 0 && _pins.rightIn2 == 0) {
			#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO
			_pins = MotorPins::drv8833(
				BYBYTE_NANO_LEFT_IN1,
				BYBYTE_NANO_LEFT_IN2,
				BYBYTE_NANO_RIGHT_IN1,
				BYBYTE_NANO_RIGHT_IN2);
			#endif
		}
	} else if (_driverType == DriverType::TB6612) {
		if (_pins.leftPwm == 0 && _pins.rightPwm == 0 && _pins.leftIn1 == 0 && _pins.leftIn2 == 0 && _pins.rightIn1 == 0 &&
			_pins.rightIn2 == 0) {
			#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
			_pins = MotorPins::tb6612(
				BYBYTE_TB6612_LEFT_IN1,
				BYBYTE_TB6612_LEFT_IN2,
				BYBYTE_TB6612_LEFT_PWM,
				BYBYTE_TB6612_RIGHT_IN1,
				BYBYTE_TB6612_RIGHT_IN2,
				BYBYTE_TB6612_RIGHT_PWM,
				BYBYTE_TB6612_STBY);
			#endif
		}
	}
}

bool MotorDriver::resolvedPinsOk() const {
	if (_driverType == DriverType::DRV8833) {
		return _pins.leftIn1 != 0 && _pins.leftIn2 != 0 && _pins.rightIn1 != 0 && _pins.rightIn2 != 0;
	}
	return _pins.stby != 0 && _pins.leftIn1 != 0 && _pins.leftIn2 != 0 &&
		   _pins.leftPwm != 0 && _pins.rightIn1 != 0 && _pins.rightIn2 != 0 && _pins.rightPwm != 0;
}

bool MotorDriver::pwmPinsOk() const {
	#if defined(ARDUINO)
	if (_driverType == DriverType::DRV8833) {
		return isPwmPin(_pins.leftIn1) && isPwmPin(_pins.leftIn2) &&
			   isPwmPin(_pins.rightIn1) && isPwmPin(_pins.rightIn2);
	}
	return isPwmPin(_pins.leftPwm) && isPwmPin(_pins.rightPwm);
	#else
	return true;
	#endif
}

bool MotorDriver::begin() {
	resolveDefaults();
	if (!resolvedPinsOk()) return false;
	if (!pwmPinsOk()) return false;
	createMotorController();
	if (!_motor) return false;
	return _motor->begin();
}

void MotorDriver::createMotorController() {
	if (_driverType == DriverType::DRV8833) {
		_motor = new DRV8833MotorController(
			_pins.leftIn1, 
			_pins.leftIn2, 
			_pins.rightIn1, 
			_pins.rightIn2);
	} else if (_driverType == DriverType::TB6612) {
		_motor = new TB6612MotorController(
			_pins.stby,
			_pins.leftIn1,
			_pins.leftIn2,
			_pins.leftPwm,
			_pins.rightIn1,
			_pins.rightIn2,
			_pins.rightPwm);
	}
}

void MotorDriver::ensureDifferential() {
	if (_mode == ControlMode::Differential && !_diffDrive && _motor) {
		_diffDrive = new DifferentialDriveController(
			*_motor,
			BYBYTE_WHEEL_SEPARATION_M,
			BYBYTE_WHEEL_RADIUS_M,
			BYBYTE_MAX_PWM);
	}
}

void MotorDriver::setMotorSpeeds(int16_t left, int16_t right) {
	_mode = ControlMode::Direct;
	if (_motor) _motor->setMotorSpeeds(left, right);
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
	ensureDifferential();
	if (_mode == ControlMode::Differential && _diffDrive) {
		_diffDrive->setTargetVelocity(_target);
		_diffDrive->update();
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
