/*
 * ByByteLib - Battery on LCD (Arduino Mega + 1602 I2C)
 *
 * Purpose:
 * - Displays battery voltage, estimated state-of-charge, and charging status
 * - Handles "battery disconnected" case with a user-friendly message
 *
 * Hardware:
 * - Arduino Mega, BatterySensor on A0, CHRG on PJ0, 1602 I2C LCD @0x27
 *
 * Usage:
 * - Upload and observe live battery readouts on the LCD
 */
#include <ByByteLib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

using namespace ByByte;

BatterySensor bat;
LiquidCrystal_I2C lcd(0x27, 16, 2); // change address if needed

void setup() {
	lcd.begin();
	lcd.backlight();
	bat.begin();
	lcd.setCursor(0,0);
	lcd.print("Battery monitor");
}

void loop() {
	static bool showedError = false;
	float v = bat.readVoltage();
	if (v < 2.0f) {
		lcd.clear();
		lcd.setCursor(0,0); lcd.print("Enable batt read");
		lcd.setCursor(0,1); lcd.print("Check battery link");
		showedError = true;
		delay(800);
		return;
	}
	if (showedError) { lcd.clear(); showedError = false; }
	uint8_t soc = bat.estimateSocPercent(v);
	bool chg = bat.isCharging();
	lcd.setCursor(0,0);
	lcd.print("V:");
	lcd.print(v,2);
	lcd.print("V      ");
	lcd.setCursor(0,1);
	lcd.print("SoC:");
	if (soc < 100) {
		if (soc < 10) lcd.print("  "); else lcd.print(" ");
	}
	lcd.print(soc);
	lcd.print("% ");
	lcd.setCursor(12,1);
	if (chg) { lcd.print("CHG"); } else { lcd.print("   "); }
	delay(500);
}
