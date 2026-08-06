/*
 * ByByteLib - Temperature on LCD (Arduino Mega + LM35 + 1602 I2C)
 *
 * Purpose:
 * - Reads LM35 temperature sensor and prints Celsius & Fahrenheit on LCD
 *
 * Hardware:
 * - Arduino Mega, LM35 on A1 (via board switch), 1602 I2C LCD @0x27
 *
 * Module-only usage:
 * - Pulls in ByByteSensors + external LCD lib.
 */
#include <Lm35Sensor.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

using namespace ByByte;

Lm35Sensor temp;
LiquidCrystal_I2C lcd(0x27, 16, 2); // change address if needed

void setup() {
	lcd.init();
	lcd.backlight();
	temp.begin();
	lcd.setCursor(0,0);
	lcd.print("Temp sensor demo");
}

void loop() {
	float c = temp.readCelsius();
	float f = temp.readFahrenheit();
	lcd.setCursor(0,1);
	lcd.print("C:"); lcd.print(c,1);
	lcd.print("  F:"); lcd.print(f,1);
	delay(500);
}