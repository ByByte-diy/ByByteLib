#include "MotorDriver.h"

namespace ByByte {

// Constructors
MotorDriver::MotorDriver()
	: _motor(nullptr), _diffDrive(nullptr), _mode(ControlMode::Direct), _driverType(DriverType::Auto) {
	_target.linearX = 0; _target.angularZ = 0;
}

MotorDriver::MotorDriver(DriverType driver)
	: _motor(nullptr), _diffDrive(nullptr), _mode(ControlMode::Direct), _driverType(driver) {
	_target.linearX = 0; _target.angularZ = 0;
}

MotorDriver::MotorDriver(const MotorPins& pins)
	: _motor(nullptr), _diffDrive(nullptr), _mode(ControlMode::Direct), _driverType(DriverType::Auto), _pins(pins) {
	_target.linearX = 0; _target.angularZ = 0;
}

MotorDriver::MotorDriver(DriverType driver, const MotorPins& pins, ControlMode mode)
	: _motor(nullptr), _diffDrive(nullptr), _mode(mode), _driverType(driver), _pins(pins) {
	_target.linearX = 0; _target.angularZ = 0;
}

// Resolve defaults and validate
void MotorDriver::resolveDefaults() {
	// Determine driver if Auto
	if (_driverType == DriverType::Auto) {
		#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO
			_driverType = DriverType::DRV8833;
		#elif BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
			_driverType = DriverType::TB6612;
		#else
			_driverType = DriverType::DRV8833;
		#endif
	}
	// Fill pins if missing (platform-specific default maps)
	if (_driverType == DriverType::DRV8833) {
		if (_pins.leftIn1 == 0 && _pins.leftIn2 == 0 && _pins.rightIn1 == 0 && _pins.rightIn2 == 0) {
			#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO
				_pins = MotorPins::drv8833(BYBYTE_NANO_LEFT_IN1, BYBYTE_NANO_LEFT_IN2, BYBYTE_NANO_RIGHT_IN1, BYBYTE_NANO_RIGHT_IN2);
			#endif
		}
	} else if (_driverType == DriverType::TB6612) {
		if (_pins.leftPwm == 0 && _pins.rightPwm == 0 && _pins.leftIn1 == 0 && _pins.leftIn2 == 0 && _pins.rightIn1 == 0 && _pins.rightIn2 == 0) {
			#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
				_pins = MotorPins::tb6612(BYBYTE_TB6612_LEFT_IN1, BYBYTE_TB6612_LEFT_IN2, BYBYTE_TB6612_LEFT_PWM,
										 BYBYTE_TB6612_RIGHT_IN1, BYBYTE_TB6612_RIGHT_IN2, BYBYTE_TB6612_RIGHT_PWM, BYBYTE_TB6612_STBY);
			#endif
		}
	}
}

void MotorDriver::validatePinsCompileTime() const {
	// DRV8833 requires all four inputs to be PWM-capable
	if (_driverType == DriverType::DRV8833) {
		#if defined(ARDUINO)
			if (!isPwmPin(_pins.leftIn1) || !isPwmPin(_pins.leftIn2) || !isPwmPin(_pins.rightIn1) || !isPwmPin(_pins.rightIn2)) {
				#warning "DRV8833 requires PWM-capable pins for in1/in2 on both sides"
			}
		#endif
	} else if (_driverType == DriverType::TB6612) {
		#if defined(ARDUINO)
			if (!isPwmPin(_pins.leftPwm) || !isPwmPin(_pins.rightPwm)) {
				#warning "TB6612 requires PWM-capable pins for leftPwm/rightPwm"
			}
		#endif
	}
}

bool MotorDriver::begin() {
	resolveDefaults();
	validatePinsCompileTime();
	createMotorController();
	if (!_motor) return false;
	return _motor->begin();
}

void MotorDriver::createMotorController() {
	if (_driverType == DriverType::DRV8833) {
		_motor = new DRV8833MotorController(_pins.leftIn1, _pins.leftIn2, _pins.rightIn1, _pins.rightIn2);
	} else if (_driverType == DriverType::TB6612) {
		_motor = new TB6612MotorController(_pins.stby, _pins.leftIn1, _pins.leftIn2, _pins.leftPwm,
											_pins.rightIn1, _pins.rightIn2, _pins.rightPwm);
	}
}

void MotorDriver::ensureDifferential() {
	if (_mode == ControlMode::Differential && !_diffDrive && _motor) {
		_diffDrive = new DifferentialDriveController(*_motor, BYBYTE_WHEEL_SEPARATION_M, BYBYTE_WHEEL_RADIUS_M, BYBYTE_MAX_PWM);
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
	Twist t; t.linearX = linearX; t.angularZ = angularZ; setTargetVelocity(t);
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
