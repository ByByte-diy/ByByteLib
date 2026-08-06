#ifndef BYBYTE_MOTOR_DRIVER_H
#define BYBYTE_MOTOR_DRIVER_H

#include <Arduino.h>

#include "core/Types.h"
#include "core/ByByteCore.h"
#include "motor/MotorTypes.h"
#include "motor/MotorController.h"
#include "motor/DRV8833MotorController.h"
#include "motor/TB6612MotorController.h"
#include "motor/DifferentialDriveController.h"

namespace ByByte {

/**
 * Self-contained motor module.
 *
 * Pulling in this header does NOT bring any other ByByte module (Bluetooth,
 * sensors, timers, PCINT, ...) — only the motor backend, which is what keeps a
 * project free of unrelated ISR / hardware conflicts.
 *
 * The driver silicon is selected at compile time from PlatformDetect
 * (TB6612 on Mega, DRV8832 otherwise), so the backend lives by value with no
 * heap allocation. Pin defaults come from configs/ByByteConfig.h when the user
 * does not pass an explicit MotorPins bundle.
 *
 * Two control modes share one instance:
 *   - Direct       : setMotorSpeeds(left, right) -> per-wheel PWM (-255..255)
 *   - Differential : setTargetVelocity(linear, angular) + update() -> Twist kinematics
 */
class MotorDriver {
public:
	/** Default pins from ByByteConfig for the active platform. */
	explicit MotorDriver(ControlMode mode = ControlMode::Direct);
	/** Explicit pin bundle (defaults still used for any field left at 0). */
	MotorDriver(const MotorPins& pins, ControlMode mode = ControlMode::Direct);
	~MotorDriver() = default;

	MotorDriver(const MotorDriver&) = delete;
	MotorDriver& operator=(const MotorDriver&) = delete;

	/** Driver selected for this build (for introspection / runtime branching). */
	DriverType driverType() const noexcept;

	/** Configure pins and bring the driver up. Returns false on invalid pins. */
	bool begin();

	/** Direct per-wheel PWM. Switches the instance to Direct mode. */
	void setMotorSpeeds(int16_t left, int16_t right);

	/** Differential command queue. Switches to Differential mode, applied on update(). */
	void setTargetVelocity(const Twist& cmd);
	void setTargetVelocity(float linearX, float angularZ);
	void update();

	// Convenience moves (Direct mode).
	void forward(int16_t speed = 100);
	void backward(int16_t speed = 100);
	void left(int16_t speed = 100);
	void right(int16_t speed = 100);
	void turnLeft(int16_t speed = 100);
	void turnRight(int16_t speed = 100);
	void stop();

private:
	// Declaration order matters: _backend depends on _pins, _diff on _backend.
	MotorPins _pins;
	ControlMode _mode;
	Twist _target;

#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	TB6612MotorController _backend;
#else
	DRV8833MotorController _backend;
#endif

	DifferentialDriveController _diff;
	bool _begun;

	bool resolvedPinsOk() const;
	bool pwmPinsOk() const;
};

} // namespace ByByte

#endif // BYBYTE_MOTOR_DRIVER_H
