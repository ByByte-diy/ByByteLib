/*
 * ByByteLib - PlatformIO Basic Example
 *
 * Purpose:
 * - Minimal sketch to verify build and basic motor control on PlatformIO
 * - Uses direct control with setMotorSpeeds and delays
 *
 * Works on:
 * - Arduino Nano and Mega (auto-detected driver/pins)
 */
#include <Arduino.h>
#include <ByByteLib.h>

using namespace ByByte;

MotorDriver motor;

void setup() {
	motor.begin();
}

void loop() {
	// Forward
	motor.setMotorSpeeds(80, 80);
	delay(1000);
	// Turn
	motor.setMotorSpeeds(-100, 100);
	delay(800);
	// Stop
	motor.stop();
	delay(500);
}
