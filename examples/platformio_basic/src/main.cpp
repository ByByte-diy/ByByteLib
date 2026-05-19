/*
 * ByByteLib - PlatformIO Basic Example
 *
 * Minimal sketch: ByByteNano kit bundles MotorDriver with DRV8833 defaults
 * from configs when building for Nano (see platformio.ini board).
 *
 * Swap to ByByteMega for Mega builds (TB6612 defaults).
 */
#include <Arduino.h>
#include <ByByteLib.h>
#include <HardwareSerial.h>

using namespace ByByte;

ByByteNano platform;

void setup() {
	Serial.begin(9600);
	Serial1.begin(115200);
	pinMode(29, OUTPUT);
	digitalWrite(29, HIGH);
	Serial.println("Starting...");
	platform.beginMotors();
	// platform.beginBluetooth();
}

void loop() {
	while (Serial1.available()) {
		Serial.write(Serial1.read());
	}
	while (Serial.available()) {
		Serial1.write(Serial.read());
	}
}
