#ifndef BYBYTE_MOTOR_DRIVER_H
#define BYBYTE_MOTOR_DRIVER_H

#include <Arduino.h>
#include "Types.h"
#include "MotorController.h"
#include "DifferentialDriveController.h"

namespace ByByte {

enum class DriverType { DRV8833, TB6612 };

enum class ControlMode { Direct, Differential };

/** Pin bundle: DRV8833 uses in1/in2 per side; TB6612 uses dir pins + PWM + STBY. */
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
	static MotorPins tb6612(uint8_t lIn1, uint8_t lIn2, uint8_t lPwm, uint8_t rIn1, uint8_t rIn2, uint8_t rPwm, uint8_t standby) {
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

class MotorDriver {
public:
	/** Driver auto-detected from build target: TB6612 on Mega, DRV8833 on Nano (and unknown → DRV8833). */
	explicit MotorDriver(ControlMode mode = ControlMode::Direct);
	MotorDriver(const MotorPins& pins, ControlMode mode = ControlMode::Direct);

	explicit MotorDriver(DriverType driver, ControlMode mode = ControlMode::Direct);
	MotorDriver(DriverType driver, const MotorPins& pins, ControlMode mode = ControlMode::Direct);

	~MotorDriver();

	MotorDriver(const MotorDriver&) = delete;
	MotorDriver& operator=(const MotorDriver&) = delete;

	bool begin();

	void setMotorSpeeds(int16_t left, int16_t right);
	void setTargetVelocity(const Twist& cmd);
	void setTargetVelocity(float linearX, float angularZ);
	void update();

	void forward(int16_t speed = 100);
	void backward(int16_t speed = 100);
	void left(int16_t speed = 100);
	void right(int16_t speed = 100);
	void turnLeft(int16_t speed = 100);
	void turnRight(int16_t speed = 100);
	void stop();

private:
	MotorController* _motor;
	DifferentialDriveController* _diffDrive;
	Twist _target;
	ControlMode _mode;
	DriverType _driverType;
	MotorPins _pins;

	void resolveDefaults();
	bool resolvedPinsOk() const;
	bool pwmPinsOk() const;
	void createMotorController();
	void ensureDifferential();
};

} // namespace ByByte

#endif // BYBYTE_MOTOR_DRIVER_H
