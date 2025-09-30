/*
 * ByByteLib - SonarPrecise Example
 * 
 * Demonstrates precise sonar distance measurement using microsecond timing.
 * 
 * Hardware:
 * - HC-SR04 ultrasonic sensor
 * - TRIG pin: D25 (Mega) / D13 (Nano)
 * - ECHO pin: D24 (Mega) / D12 (Nano)
 * 
 * Features:
 * - Non-blocking distance measurement
 * - Microsecond precision timing
 * - Automatic trigger every 100ms
 * - Serial output with diagnostics
 * 
 * Usage:
 * 1. Connect HC-SR04 sensor to specified pins
 * 2. Upload this sketch
 * 3. Open Serial Monitor (115200 baud)
 * 4. Observe distance measurements and diagnostics
 */

#include <ByByteLib.h>

// Sonar configuration
static const uint8_t SONAR_TRIG_PIN = 25;  // Mega: D25, Nano: D13
static const uint8_t SONAR_ECHO_PIN = 24;  // Mega: D24, Nano: D12
static const uint16_t SONAR_MAX_CM = 200;  // Maximum range in cm

// Sonar instance
static ByByte::SonarPrecise* sonar = nullptr;

void setup() {
  Serial.begin(115200);
  Serial.println(F("=== ByByte SonarPrecise Example ==="));
  Serial.println(F("Precise ultrasonic distance measurement"));
  Serial.println();
  
  // Initialize sonar
  sonar = ByByte::SonarPrecise::create(SONAR_TRIG_PIN, SONAR_ECHO_PIN, SONAR_MAX_CM);
  if (sonar) {
    if (sonar->begin()) {
      Serial.println(F("Sonar initialized successfully"));
      Serial.print(F("TRIG pin: D"));
      Serial.println(SONAR_TRIG_PIN);
      Serial.print(F("ECHO pin: D"));
      Serial.println(SONAR_ECHO_PIN);
      Serial.print(F("Max range: "));
      Serial.print(SONAR_MAX_CM);
      Serial.println(F(" cm"));
    } else {
      Serial.println(F("Sonar initialization failed!"));
      delete sonar;
      sonar = nullptr;
    }
  } else {
    Serial.println(F("Sonar creation failed!"));
  }
  
  Serial.println();
  Serial.println(F("Distance measurements:"));
  Serial.println(F("Format: Distance(cm) | State | LastTrigger(us)"));
  Serial.println(F("----------------------------------------"));
}

void loop() {
  if (!sonar) {
    Serial.println(F("Sonar not available"));
    delay(1000);
    return;
  }
  
  // Read distance
  uint16_t distance = sonar->readCm();
  uint8_t state = sonar->getState();
  uint32_t lastTrigger = sonar->getLastTriggerUs();
  
  // Display results
  if (distance == 0) {
    Serial.print(F("No echo     "));
  } else {
    Serial.print(distance);
    Serial.print(F(" cm        "));
  }
  
  Serial.print(F("| State: "));
  Serial.print(state);
  Serial.print(F(" | Trigger: "));
  Serial.print(lastTrigger);
  Serial.println();
  
  // Wait before next reading
  delay(100);
}


