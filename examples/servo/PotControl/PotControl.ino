/*
 * ByByteLib - Servo Pot Control (Arduino Mega)
 *
 * Purpose:
 * - Reads two potentiometers on A6 and A7 and drives two standard servos
 * - Maps 0..1023 -> 0..180 degrees
 *
 * Notes:
 * - Intended for Arduino Mega only.
 * - Servos run via ServoManager (Timer3), independent from motor PWM.
 *
 * Module-only usage:
 * - Pulls in ByByteServo only. The Arduino built-in "Servo" library is ignored
 *   since ByByteServo provides the same <Servo.h> header (class ByByte::Servo).
 */
#include <ByByteLib.h>

using namespace ByByte;

Servo s0, s1;

void setup() {
	Serial.begin(115200);
	if (!s0.attach(BYBYTE_SERVO0_PIN)) {
		Serial.println(F("Failed to attach servo 0"));
	}
	if (!s1.attach(BYBYTE_SERVO1_PIN)) {
		Serial.println(F("Failed to attach servo 1"));
	}
	Serial.print(F("Servo0 on D")); Serial.println(BYBYTE_SERVO0_PIN);
	Serial.print(F("Servo1 on D")); Serial.println(BYBYTE_SERVO1_PIN);
}

void loop() {
	int v0 = analogRead(A6); // 0..1023
	int v1 = analogRead(A7);
	uint8_t a0 = (uint8_t)map(v0, 0, 1023, 0, 180);
	uint8_t a1 = (uint8_t)map(v1, 0, 1023, 0, 180);
	s0.write(a0);
	s1.write(a1);
	Serial.print(F("A6=")); Serial.print(v0);
	Serial.print(F(" -> angle0=")); Serial.print(a0);
	Serial.print(F(" | A7=")); Serial.print(v1);
	Serial.print(F(" -> angle1=")); Serial.println(a1);
	delay(50);
}