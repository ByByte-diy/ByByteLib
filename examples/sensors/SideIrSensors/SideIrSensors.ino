/*
 * ByByteLib - Side IR Wall Sensors Example
 * 
 * This example demonstrates how to use side IR sensors for wall detection
 * in maze navigation. The sensors use power gating to save battery.
 * 
 * Hardware:
 * - IR Transmitter: SFH4545
 * - IR Receiver: TEFT4300
 * - Power control transistor on digital pin
 * - Analog readings from left/right sensors
 * 
 * Pin mapping (auto-detected):
 * - Nano: Left=A6, Right=A7, Power=D4
 * - Mega: Left=A14, Right=A15, Power=D22
 */

#include <ByByteLib.h>

using namespace ByByte;

// Create sensor instance with auto-detected pins
SideIrSensors irSensors;

void setup() {
	Serial.begin(115200);
	Serial.println(F("=== ByByteLib Side IR Wall Sensors Demo ==="));
	Serial.println();
	
	// Initialize sensors
	irSensors.begin();
	Serial.println(F("IR sensors initialized"));
	
	// Calibration phase
	Serial.println(F("Starting auto-calibration..."));
	Serial.println(F("Place robot at typical wall distances and empty space"));
	Serial.println(F("Calibration will complete in 3 seconds..."));
	
	// Auto-calibrate over 3 seconds (150 samples at 20ms intervals)
	irSensors.autoCalibrate(150, 20);
	
	Serial.println(F("Calibration complete!"));
	Serial.println();
	
	// Display calibration results
	Serial.print(F("Left sensor range: "));
	Serial.print(irSensors.leftMin());
	Serial.print(F(" - "));
	Serial.println(irSensors.leftMax());
	
	Serial.print(F("Right sensor range: "));
	Serial.print(irSensors.rightMin());
	Serial.print(F(" - "));
	Serial.println(irSensors.rightMax());
	Serial.println();
	
	Serial.println(F("Normalized readings (0-1000):"));
	Serial.println(F("Left\tRight\tWalls"));
	Serial.println(F("----\t-----\t-----"));
}

void loop() {
	uint16_t leftNorm, rightNorm;
	
	// Read normalized sensor values
	irSensors.readNormalized(leftNorm, rightNorm);
	
	// Display readings
	Serial.print(leftNorm);
	Serial.print(F("\t"));
	Serial.print(rightNorm);
	Serial.print(F("\t"));
	
	// Simple wall detection (threshold depends on calibration/environment)
	const uint16_t WALL_THRESHOLD = 500; // Increase if walls are farther/less reflective
	bool wallLeft = leftNorm > WALL_THRESHOLD;
	bool wallRight = rightNorm > WALL_THRESHOLD;
	
	if (wallLeft && wallRight) {
		Serial.println(F("Walls L+R"));
	} else if (wallLeft && !wallRight) {
		Serial.println(F("Wall Left"));
	} else if (!wallLeft && wallRight) {
		Serial.println(F("Wall Right"));
	} else {
		Serial.println(F("No walls"));
	}
	
	delay(100); // Update every 100ms
}
