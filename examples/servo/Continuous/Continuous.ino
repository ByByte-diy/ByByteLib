/*
 * ByByteLib - Continuous (360°) Servo Example (Arduino Mega)
 *
 * Purpose:
 * - Controls a continuous rotation servo using a potentiometer on A6
 * - Speed in percent (-100..100), with adjustable stop pulse and deadband
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
#include <Servo.h>

ByByte::Servo sc;

// Tune these for your servo's stop point and deadband
static const uint16_t STOP_US = 1320;
static const uint16_t DEADBAND_US = 20;

void setup() {
	Serial.begin(9600);
	if (!sc.attach(BYBYTE_SERVO0_PIN)) {
		Serial.println(F("Failed to attach continuous servo"));
	}
	sc.setContinuous(true, STOP_US, DEADBAND_US);
}

void loop() {
	int v = analogRead(A6); // 0..1023
	int8_t speed = (int8_t)map(v, 0, 1023, -100, 100);
	sc.writeSpeed(speed);
	Serial.print(F("A6=")); Serial.print(v);
	Serial.print(F(" -> speed%=")); Serial.println(speed);
	delay(50);
}