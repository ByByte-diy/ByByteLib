/*
 * ByByteLib - System Test for Mega Platform
 * 
 * Comprehensive test of all Mega platform peripherals using menu navigation.
 * 
 * Controls:
 * - BTN1 (D39): Previous test
 * - BTN2 (D40): Next test  
 * - BTN3 (D41): Select/Exit test
 * 
 * Tests:
 * 1. Motors (forward/backward)
 * 2. Line sensors (6 analog sensors)
 * 3. Side IR sensors (wall detection)
 * 4. RGB LEDs (red, green, blue)
 * 5. White LEDs (headlights)
 * 6. Potentiometers (A6, A7)
 * 7. Bluetooth (name, type, baud)
 * 8. Battery (voltage, SoC, charging)
 * 9. Temperature (LM35)
 * 10. Light sensor (LDR)
 * 11. IR receiver (remote codes)
 * 12. Sonar (distance)
 * 13. Tachometers (wheel speed)
 * 14. Shock sensor
 * 15. 12V power control
 * 16. Buzzer (R2D2 sound)
 * 17. Servo (0-180 degrees)
 */

#include <ByByteLib.h>
#include <LiquidCrystal_I2C.h>
#include <QTRSensors.h>
#include <Adafruit_NeoPixel.h>
#include <MPU6050.h>

// Hardware
static LiquidCrystal_I2C lcd(0x27, 16, 2);
static Adafruit_NeoPixel ws2812(BYBYTE_WS2812_COUNT, BYBYTE_WS2812_PIN, NEO_GRB + NEO_KHZ800);
static QTRSensors lineSensors;
static MPU6050 mpu(Wire);
// Sonar (HC-SR04 compatible) on Mega: TRIG=D25, ECHO=D24
static const uint8_t SONAR_TRIG_PIN = 25;
static const uint8_t SONAR_ECHO_PIN = 24;
static const uint16_t SONAR_MAX_CM = 200; // 2 meters max range
// Use precise Sonar with microsecond timing
static ByByte::SonarPrecise* sonar = nullptr;

// Test system
enum TestType {
  TEST_MOTORS = 0,
  TEST_LINE_SENSORS,
  TEST_SIDE_IR,
  TEST_RGB_LEDS,
  TEST_WHITE_LEDS,
  TEST_POTS,
  TEST_BLUETOOTH,
  TEST_BATTERY,
  TEST_TEMPERATURE,
  TEST_LIGHT,
  TEST_IR_RECEIVER,
  TEST_SONAR,
  TEST_TACHOMETERS,
  TEST_SHOCK,
  TEST_12V_POWER,
  TEST_BUZZER,
  TEST_SERVO,
  TEST_COUNT
};

const char* testNames[] = {
  "Motors",
  "Line Sensors", 
  "Side IR",
  "RGB LEDs",
  "White LEDs",
  "Pots",
  "Bluetooth",
  "Battery",
  "Temperature",
  "Light",
  "IR Receiver",
  "Sonar",
  "Tachometers",
  "Shock",
  "12V Power",
  "Buzzer",
  "Servo"
};

// State
int currentTest = 0;
bool inTest = false;
unsigned long testStartTime = 0;
unsigned long lastButtonTime = 0;
bool lastBtn1 = HIGH, lastBtn2 = HIGH, lastBtn3 = HIGH;

// Test-specific state
int motorDirection = 0;
int rgbColor = 0;
int servoAngle = 0;
bool servoDirection = true;
bool power12v = false;
unsigned long powerToggleTime = 0;

// Objects
ByByte::MotorDriver motors(DriverType::TB6612);
ByByte::Bluetooth bt;
ByByte::Buzzer buzzer;
ByByte::BatterySensor battery;
ByByte::Lm35Sensor temperature;
ByByte::LdrSensor light;
ByByte::SideIrSensors sideIr;
ByByte::IrReceiver irReceiver;
ByByte::Servo servo0;

void setup() {
  Serial.begin(115200);
  Serial.println(F("=== ByByte Mega System Test ==="));
  
  // Initialize hardware
  Wire.begin();
  Wire.setClock(400000);
  
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  
  // Initialize buttons
  pinMode(BYBYTE_BTN1_PIN, INPUT_PULLUP);
  pinMode(BYBYTE_BTN2_PIN, INPUT_PULLUP);
  pinMode(BYBYTE_BTN3_PIN, INPUT_PULLUP);
  
  // Initialize other components
  motors.begin();
  bt.begin();
  buzzer.begin();
  battery.begin();
  temperature.begin();
  light.begin();
  sideIr.begin();
  irReceiver.begin();
  servo0.attach(BYBYTE_SERVO0_PIN);
  
  // Initialize line sensors
  uint8_t linePins[] = {A8, A9, A10, A11, A12, A13};
  lineSensors.setTypeAnalog();
  lineSensors.setSensorPins(linePins, 6);
  lineSensors.setEmitterPin(BYBYTE_LINE_PWR_PIN);
  
  // Initialize WS2812
  ws2812.begin();
  ws2812.clear();
  ws2812.show();
  
  // Initialize white LEDs
  pinMode(BYBYTE_HEADLIGHT_LEFT_PIN, OUTPUT);
  pinMode(BYBYTE_HEADLIGHT_RIGHT_PIN, OUTPUT);
  
  // Initialize RGB LEDs
  pinMode(BYBYTE_LED_RED_PIN, OUTPUT);
  pinMode(BYBYTE_LED_GREEN_PIN, OUTPUT);
  pinMode(BYBYTE_LED_BLUE_PIN, OUTPUT);
  
  // Init sonar
  sonar = ByByte::SonarPrecise::create(SONAR_TRIG_PIN, SONAR_ECHO_PIN, SONAR_MAX_CM);
  if (sonar) {
    if (!sonar->begin()) {
      Serial.println(F("Sonar init failed!"));
      delete sonar;
      sonar = nullptr;
    } else {
      Serial.println(F("Sonar initialized successfully"));
    }
  } else {
    Serial.println(F("Sonar create failed!"));
  }
  
  // Initialize 12V power control
  pinMode(BYBYTE_PWR12_EN_PIN, OUTPUT);
  digitalWrite(BYBYTE_PWR12_EN_PIN, LOW);
  
  // Initialize shock sensor
  pinMode(BYBYTE_SHOCK_PIN, INPUT_PULLUP);
  
  // Initialize tachometers
  pinMode(BYBYTE_ENCODER_LEFT_PIN, INPUT_PULLUP);
  pinMode(BYBYTE_ENCODER_RIGHT_PIN, INPUT_PULLUP);
  
  // Initialize MPU6050
  mpu.begin();
  
  showMenu();
}

void loop() {
  handleButtons();
  
  if (inTest) {
    runCurrentTest();
  }
  delay(50);
}

void handleButtons() {
  bool btn1 = digitalRead(BYBYTE_BTN1_PIN);
  bool btn2 = digitalRead(BYBYTE_BTN2_PIN);
  bool btn3 = digitalRead(BYBYTE_BTN3_PIN);
  
  unsigned long now = millis();
  
  // Debounce
  if (now - lastButtonTime < 200) return;
  
  // Button 1 - Previous test
  if (btn1 == LOW && lastBtn1 == HIGH) {
    if (!inTest) {
      currentTest = (currentTest - 1 + TEST_COUNT) % TEST_COUNT;
      showMenu();
    }
    lastButtonTime = now;
  }
  
  // Button 2 - Next test
  if (btn2 == LOW && lastBtn2 == HIGH) {
    if (!inTest) {
      currentTest = (currentTest + 1) % TEST_COUNT;
      showMenu();
    }
    lastButtonTime = now;
  }
  
  // Button 3 - Select/Exit
  if (btn3 == LOW && lastBtn3 == HIGH) {
    if (inTest) {
      exitTest();
    } else {
      startTest();
    }
    lastButtonTime = now;
  }
  
  lastBtn1 = btn1;
  lastBtn2 = btn2;
  lastBtn3 = btn3;
}

void showMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Test: "));
  lcd.print(testNames[currentTest]);
  lcd.setCursor(0, 1);
  lcd.print(F("BTN1/2:nav BTN3:sel"));
}

void startTest() {
  inTest = true;
  testStartTime = millis();
  lcd.clear();
  
  // Reset test-specific state
  motorDirection = 0;
  rgbColor = 0;
  servoAngle = 0;
  servoDirection = true;
  power12v = false;
  powerToggleTime = 0;
  
  // Stop motors and clear LEDs
  motors.stop();
  ws2812.clear();
  ws2812.show();
  digitalWrite(BYBYTE_HEADLIGHT_LEFT_PIN, LOW);
  digitalWrite(BYBYTE_HEADLIGHT_RIGHT_PIN, LOW);
  digitalWrite(BYBYTE_LED_RED_PIN, LOW);
  digitalWrite(BYBYTE_LED_GREEN_PIN, LOW);
  digitalWrite(BYBYTE_LED_BLUE_PIN, LOW);
  
  showTestStart();
}

void exitTest() {
  inTest = false;
  
  // Cleanup
  motors.stop();
  buzzer.noTone();
  servo0.write(90);
  digitalWrite(BYBYTE_PWR12_EN_PIN, LOW);
  ws2812.clear();
  ws2812.show();
  digitalWrite(BYBYTE_HEADLIGHT_LEFT_PIN, LOW);
  digitalWrite(BYBYTE_HEADLIGHT_RIGHT_PIN, LOW);
  digitalWrite(BYBYTE_LED_RED_PIN, LOW);
  digitalWrite(BYBYTE_LED_GREEN_PIN, LOW);
  digitalWrite(BYBYTE_LED_BLUE_PIN, LOW);
  
  showMenu();
}

void showTestStart() {
  lcd.setCursor(0, 0);
  lcd.print(F("Testing: "));
  lcd.print(testNames[currentTest]);
  lcd.setCursor(0, 1);
  lcd.print(F("BTN3: Exit"));
}

void runCurrentTest() {
  switch (currentTest) {
    case TEST_MOTORS: testMotors(); break;
    case TEST_LINE_SENSORS: testLineSensors(); break;
    case TEST_SIDE_IR: testSideIr(); break;
    case TEST_RGB_LEDS: testRgbLeds(); break;
    case TEST_WHITE_LEDS: testWhiteLeds(); break;
    case TEST_POTS: testPots(); break;
    case TEST_BLUETOOTH: testBluetooth(); break;
    case TEST_BATTERY: testBattery(); break;
    case TEST_TEMPERATURE: testTemperature(); break;
    case TEST_LIGHT: testLight(); break;
    case TEST_IR_RECEIVER: testIrReceiver(); break;
    case TEST_SONAR: testSonar(); break;
    case TEST_TACHOMETERS: testTachometers(); break;
    case TEST_SHOCK: testShock(); break;
    case TEST_12V_POWER: test12vPower(); break;
    case TEST_BUZZER: testBuzzer(); break;
    case TEST_SERVO: testServo(); break;
  }
}

void testMotors() {
  unsigned long elapsed = millis() - testStartTime;
  int phase = (elapsed / 2000) % 3; // 0: forward, 1: stop, 2: backward
  
  lcd.setCursor(0, 0);
  lcd.print(F("Motor Test"));
  lcd.setCursor(0, 1);
  
  switch (phase) {
    case 0:
      lcd.print(F("Forward    "));
      motors.forward(100);
      break;
    case 1:
      lcd.print(F("Stop       "));
      motors.stop();
      break;
    case 2:
      lcd.print(F("Backward   "));
      motors.backward(100);
      break;
  }
}

void testLineSensors() {
  uint16_t sensorValues[6];
  lineSensors.readCalibrated(sensorValues);
  
  lcd.setCursor(0, 0);
  lcd.print(F("Line Sensors"));
  lcd.setCursor(0, 1);
  
  // Show sensor states as filled rectangles or underscores
  for (int i = 0; i < 6; i++) {
    lcd.setCursor(10 + i, 1);
    if (sensorValues[i] > 500) {
      lcd.write(0xFF); // Filled rectangle
    } else {
      lcd.print(F("_")); // Underscore
    }
  }
}

void testSideIr() {
  uint16_t left, right;
  sideIr.sample(left, right);
  
  lcd.setCursor(0, 0);
  lcd.print(F("Side IR"));
  lcd.setCursor(0, 1);
  lcd.print(F("L:"));
  lcd.print(left);
  lcd.print(F(" R:"));
  lcd.print(right);
}

void testRgbLeds() {
  unsigned long elapsed = millis() - testStartTime;
  int phase = (elapsed / 1000) % 3; // 0: red, 1: green, 2: blue
  
  lcd.setCursor(0, 0);
  lcd.print(F("RGB LEDs"));
  lcd.setCursor(0, 1);
  
  // Clear all RGB LEDs
  digitalWrite(BYBYTE_LED_RED_PIN, LOW);
  digitalWrite(BYBYTE_LED_GREEN_PIN, LOW);
  digitalWrite(BYBYTE_LED_BLUE_PIN, LOW);
  
  switch (phase) {
    case 0:
      lcd.print(F("Red        "));
      digitalWrite(BYBYTE_LED_RED_PIN, HIGH);
      break;
    case 1:
      lcd.print(F("Green      "));
      digitalWrite(BYBYTE_LED_GREEN_PIN, HIGH);
      break;
    case 2:
      lcd.print(F("Blue       "));
      digitalWrite(BYBYTE_LED_BLUE_PIN, HIGH);
      break;
  }
}

void testWhiteLeds() {
  unsigned long elapsed = millis() - testStartTime;
  int phase = (elapsed / 1000) % 3; // 0: left, 1: both, 2: right
  
  lcd.setCursor(0, 0);
  lcd.print(F("White LEDs"));
  lcd.setCursor(0, 1);
  
  // Clear both LEDs
  digitalWrite(BYBYTE_HEADLIGHT_LEFT_PIN, LOW);
  digitalWrite(BYBYTE_HEADLIGHT_RIGHT_PIN, LOW);
  
  switch (phase) {
    case 0:
      lcd.print(F("Left       "));
      digitalWrite(BYBYTE_HEADLIGHT_LEFT_PIN, HIGH);
      break;
    case 1:
      lcd.print(F("Both       "));
      digitalWrite(BYBYTE_HEADLIGHT_LEFT_PIN, HIGH);
      digitalWrite(BYBYTE_HEADLIGHT_RIGHT_PIN, HIGH);
      break;
    case 2:
      lcd.print(F("Right      "));
      digitalWrite(BYBYTE_HEADLIGHT_RIGHT_PIN, HIGH);
      break;
  }
}

void testPots() {
  int pot1 = analogRead(A6);
  int pot2 = analogRead(A7);
  
  lcd.setCursor(0, 0);
  lcd.print(F("Pots A6/A7"));
  lcd.setCursor(0, 1);
  
  // Show pot1 as bar graph
  int bar1 = map(pot1, 0, 1023, 0, 7);
  lcd.print(F("A6:"));
  for (int i = 0; i < 8; i++) {
    lcd.setCursor(3 + i, 1);
    if (i < bar1) {
      lcd.write(0xFF);
    } else {
      lcd.print(F("_"));
    }
  }
}

void testBluetooth() {
  lcd.setCursor(0, 0);
  lcd.print(F("Bluetooth"));
  lcd.setCursor(0, 1);
  
  if (bt.isReady()) {
    lcd.print(F("Ready"));
    String name;
    if (bt.getName(name)) {
      lcd.setCursor(6, 1);
      lcd.print(name);
    }
  } else {
    lcd.print(F("Not Ready"));
  }
}

void testBattery() {
  float voltage = battery.readVoltage();
  int soc = battery.estimateSocPercent(voltage);
  bool charging = battery.isCharging();
  
  lcd.setCursor(0, 0);
  lcd.print(F("Battery"));
  lcd.setCursor(0, 1);
  lcd.print(voltage, 1);
  lcd.print(F("V "));
  lcd.print(soc);
  lcd.print(F("%"));
  if (charging) {
    lcd.print(F(" CHG"));
  }
}

void testTemperature() {
  float tempC = temperature.readCelsius();
  float tempF = temperature.readFahrenheit();
  
  lcd.setCursor(0, 0);
  lcd.print(F("Temperature"));
  lcd.setCursor(0, 1);
  lcd.print(tempC, 1);
  lcd.print(F("C "));
  lcd.print(tempF, 1);
  lcd.print(F("F"));
}

void testLight() {
  int lightValue = light.readRaw();
  int normalized = light.readNormalized();
  
  lcd.setCursor(0, 0);
  lcd.print(F("Light Sensor"));
  lcd.setCursor(0, 1);
  lcd.print(lightValue);
  lcd.print(F(" ("));
  lcd.print(normalized);
  lcd.print(F("%)"));
}

void testIrReceiver() {
  lcd.setCursor(0, 0);
  lcd.print(F("IR Receiver"));
  lcd.setCursor(0, 1);
  
  if (irReceiver.available()) {
    ByByte::IrFrame frame = irReceiver.read();
    lcd.print(F("Code: "));
    lcd.print(frame.command, HEX);
  } else {
    lcd.print(F("Waiting..."));
  }
}

// removed local sonarTick; using library Sonar with TimerManager

void testSonar() {
  lcd.setCursor(0, 0);
  lcd.print(F("Sonar D25/D24"));
  lcd.setCursor(0, 1);
  
  if (!sonar) {
    lcd.print(F("Not init    "));
    return;
  }
  
  uint16_t cm = sonar->readCm();
  uint8_t state = sonar->getState();
  uint32_t lastTrigger = sonar->getLastTriggerUs();
  
  if (cm == 0) {
    lcd.print(F("No echo     "));
  } else {
    lcd.print(cm);
    lcd.print(F(" cm        "));
  }
  
  // Debug info to Serial
  Serial.print(F("Sonar: cm="));
  Serial.print(cm);
  Serial.print(F(", state="));
  Serial.print(state);
  Serial.print(F(", lastTrigger="));
  Serial.println(lastTrigger);
}

void testTachometers() {
  static int leftCount = 0, rightCount = 0;
  static unsigned long lastCountTime = 0;
  
  // Slowly move motors
  motors.forward(30);
  
  lcd.setCursor(0, 0);
  lcd.print(F("Tachometers"));
  lcd.setCursor(0, 1);
  lcd.print(F("L:"));
  lcd.print(leftCount);
  lcd.print(F(" R:"));
  lcd.print(rightCount);
  
  // Count pulses (simplified - would need interrupt handling in real implementation)
  if (millis() - lastCountTime > 100) {
    if (digitalRead(BYBYTE_ENCODER_LEFT_PIN) == LOW) leftCount++;
    if (digitalRead(BYBYTE_ENCODER_RIGHT_PIN) == LOW) rightCount++;
    lastCountTime = millis();
  }
}

void testShock() {
  bool shock = digitalRead(BYBYTE_SHOCK_PIN) == LOW;
  
  lcd.setCursor(0, 0);
  lcd.print(F("Shock Sensor"));
  lcd.setCursor(0, 1);
  lcd.print(shock ? F("TRIGGERED") : F("Normal"));
}

void test12vPower() {
  unsigned long now = millis();
  
  if (now - powerToggleTime > 1000) {
    power12v = !power12v;
    digitalWrite(BYBYTE_PWR12_EN_PIN, power12v ? HIGH : LOW);
    powerToggleTime = now;
  }
  
  lcd.setCursor(0, 0);
  lcd.print(F("12V Power"));
  lcd.setCursor(0, 1);
  lcd.print(power12v ? F("ON ") : F("OFF"));
}

void testBuzzer() {
  lcd.setCursor(0, 0);
  lcd.print(F("Buzzer"));
  lcd.setCursor(0, 1);
  lcd.print(F("R2D2 Sound"));
  
  // Play R2D2 sound once
  static bool played = false;
  if (!played) {
    buzzer.patternR2D2();
    played = true;
  }
}

void testServo() {
  unsigned long elapsed = millis() - testStartTime;
  
  lcd.setCursor(0, 0);
  lcd.print(F("Servo Test"));
  lcd.setCursor(0, 1);
  
  if (elapsed < 2000) {
    lcd.print(F("0 degrees"));
    servo0.write(0);
  } else if (elapsed < 4000) {
    lcd.print(F("90 degrees"));
    servo0.write(90);
  } else if (elapsed < 6000) {
    lcd.print(F("180 degrees"));
    servo0.write(180);
  } else {
    lcd.print(F("90 degrees"));
    servo0.write(90);
    testStartTime = millis(); // Reset cycle
  }
}
