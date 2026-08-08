/*
 * ByByteLib - Motors: Tank (Differential) Control
 *
 * Purpose:
 * - Demonstrates differential control using setTargetVelocity(linear, angular)
 * - MotorDriver internally converts v,w into left/right PWM
 *
 * Works on:
 * - Arduino Nano (DRV8833) and Mega (TB6612)
 * - Driver silicon and pin defaults are selected at compile time from the
 *   target board, so no DriverType or pin bundle is needed here
 *
 * Module-only usage:
 * - This sketch pulls in the ByByteMotor module alone (see platformio.ini),
 *   not the full ByByteLib aggregate. Only the motor .cpp units are compiled
 *   and linked — nothing else (no Bluetooth, timers, PCINT, ...).
 *
 * Build (from this directory):
 * - pio run -e nano   or   pio run -e mega
 */
#include <MotorDriver.h>

using namespace ByByte;

// Default constructor: pins come from ByByteConfig for the compiled board,
// and the driver backend is chosen by PlatformDetect (TB6612 on Mega,
// DRV8833 on Nano). No heap allocation.
MotorDriver motor(ControlMode::Differential);

void setup() {
	Serial.begin(115200);
	if (!motor.begin()) {
		Serial.println(F("MotorDriver: invalid pins — check board target"));
		while (true) {}
	}

	// Driver selected at compile time (DRV8833 or TB6612), for introspection.
	Serial.print(F("Motor driver: "));
	Serial.println(motor.driverType() == DriverType::TB6612 ? F("TB6612") : F("DRV8833"));
}

void loop() {
	// Tank-style differential control.
	//   linearX  -> forward (+) / backward (-) speed
	//   angularZ -> turn left (+) / turn right (-) rate
	// update() applies the queued command to the wheels.
	motor.setTargetVelocity(100, 0);  // forward
	motor.update();
	delay(1000);

	motor.setTargetVelocity(0, 60);   // turn left
	motor.update();
	delay(800);

	motor.setTargetVelocity(-100, 0); // backward
	motor.update();
	delay(1000);

	motor.setTargetVelocity(0, -60);  // turn right
	motor.update();
	delay(800);

	motor.stop();
	delay(500);
}