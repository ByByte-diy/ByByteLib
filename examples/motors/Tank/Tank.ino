/*
 * ByByteLib - Motors: Tank (Differential) Control
 *
 * Purpose:
 * - Demonstrates differential control using setTargetVelocity(linear, angular)
 * - MotorDriver internally converts v,w into left/right PWM
 *
 * Works on:
 * - Arduino Nano and Mega (driver/pins auto-detected)
 *
 * Usage:
 * - Upload and observe forward/turn/backward/turn/stop sequence
 */
#include <ByByteLib.h>

using namespace ByByte;

MotorDriver motor;

void setup() {
	motor.begin();
}

void loop() {
	// Tank-style differential control
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
