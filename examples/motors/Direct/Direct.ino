/*
 * ByByteLib - Motors: Direct Control
 *
 * Purpose:
 * - Shows how to drive motors directly with setMotorSpeeds(left,right)
 * - Positive = forward, negative = reverse, range ~ -255..255
 *
 * Works on:
 * - Arduino Nano (DRV8833) and Mega (TB6612), pins auto-detected
 *
 * Usage:
 * - Upload and the robot will move forward, turn, backward, turn, then stop
 */
#include <ByByteLib.h>

using namespace ByByte;

MotorDriver motor;

void setup() {
	motor.begin();
}

void loop() {
	// Direct motor control
	motor.setMotorSpeeds(80, 80);  // forward
	delay(1000);

	motor.setMotorSpeeds(-100, 100);  // turn left
	delay(800);

	motor.setMotorSpeeds(-80, -80);  // backward
	delay(1000);

	motor.setMotorSpeeds(100, -100);  // turn right
	delay(800);

	motor.stop();
	delay(500);
}
