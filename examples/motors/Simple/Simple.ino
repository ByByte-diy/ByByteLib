/*
 * ByByteLib - Motors: Simple Moves
 *
 * Purpose:
 * - Demonstrates convenience movement methods of MotorDriver
 * - forward/backward/turnLeft/turnRight/stop with fixed speeds and delays
 *
 * Works on:
 * - Arduino Nano (DRV8833) and Mega (TB6612), pins auto-detected from ByByteConfig
 *
 * Module-only usage:
 * - Pulls in the ByByteMotor module alone (see platformio.ini).
 */
#include <MotorDriver.h>

using namespace ByByte;

MotorDriver motor;

void setup() {
	motor.begin();
}

void loop() {
	// Simple movement methods
	motor.forward(100);
	delay(1000);

	motor.turnLeft(80);
	delay(800);

	motor.backward(100);
	delay(1000);

	motor.turnRight(80);
	delay(800);

	motor.stop();
	delay(500);
}