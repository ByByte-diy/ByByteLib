/*
 * ByByteLib - Sonar Example
 *
 * Demonstrates simple sonar distance measurement with the default
 * board-specific pin configuration supplied by the library.
 *
 * Usage:
 * 1. Connect an HC-SR04 sensor to the configured pins.
 * 2. Upload this sketch.
 * 3. Open the Serial Monitor at 115200 baud.
 */
#include <Sonar.h>

ByByte::Sonar sonar;

void setup() {
  Serial.begin(9600);
  Serial.println(F("=== ByByte Sonar Example ==="));
  Serial.println(F("Using library-managed default pins"));
  Serial.println();

  sonar.begin();

  Serial.println(F("Sonar initialized successfully"));
  Serial.print(F("TRIG pin: D")); Serial.println(BYBYTE_SONAR_TRIG_PIN);
  Serial.print(F("ECHO pin: D")); Serial.println(BYBYTE_SONAR_ECHO_PIN);
  Serial.print(F("Max range: ")); Serial.print(BYBYTE_SONAR_MAX_CM); Serial.println(F(" cm"));
  Serial.println();
  Serial.println(F("Distance measurements:"));
}

void loop() {
  uint16_t distance = sonar.readCm();

  if (distance == 0) {
    Serial.println(F("No echo"));
  } else {
    Serial.print(distance);
    Serial.println(F(" cm"));
  }

  delay(200);
}