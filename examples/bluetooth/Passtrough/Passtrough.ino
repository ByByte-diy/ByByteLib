/*
 * ByByteLib - Bluetooth Passthrough (HC-02/05/06/08/42)
 *
 * Purpose:
 * - Initializes Bluetooth module, aligns baud rate, and echoes data
 * - Provides simple AT commands demo: name/address, rename, reset
 *
 * Hardware:
 * - Mega: Serial1 + power pin D29 (auto handled)
 * - Nano: SoftwareSerial on D2(RX), D3(TX)
 */
#include <ByByteLib.h>

using namespace ByByte;

Bluetooth bt;

void setup() {
	Serial.begin(9600);
	Serial.println(F("=== Bluetooth Basic ==="));
	Serial.println(F("Starting BT module..."));
	bt.begin(9600);
	Serial.println(F("Type 'r' to reset, 'n' to rename, any other to echo."));
}

void loop() {
	// Show status once when ready
	static bool showedReady = false;
	if (bt.isReady() && !showedReady) {
		Serial.println(F("BT module ready!"));
		Serial.print(F("Baud: ")); Serial.println(bt.baud());
		String name;
		if (bt.getName(name)) Serial.println(String(F("Name: ")) + name);
		showedReady = true;
	}

	// PC -> BT passthrough
	if (Serial.available()) {
		char c = (char)Serial.read();
		bt.write((uint8_t)c);
	}
	// BT -> PC
	while (bt.available()) {
		int b = bt.read();

		Serial.write((char)b);
	}
}


