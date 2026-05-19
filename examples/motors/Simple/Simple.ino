/*
 * ByByteLib - Motors: Simple Moves
 *
 * Purpose:
 * - Demonstrates convenience movement methods of MotorDriver
 * - forward/backward/turnLeft/turnRight/stop with fixed speeds and delays
 *
 * Works on:
 * - Arduino Nano and Mega (pins auto-detected from configs/ByByteConfig)
 *
 * Usage:
 * - Upload and observe the robot perform a simple movement routine
 */
#include <ByByteLib.h>

using namespace ByByte;

MotorDriver motor(MotorDriver::driverForBuildTarget());

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
