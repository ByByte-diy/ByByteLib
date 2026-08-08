/*
 * ByByteLib - Line Follow (Mega) with LCD indicators
 *
 * Purpose:
 * - Line following using QTR analog sensors and P-control
 * - Displays per-sensor detection blocks on 1602 I2C LCD (bottom row)
 * - Uses Differential control: setTargetVelocity(v,w) + update()
 *
 * Hardware:
 * - Mega: 6 analog line sensors (A8..A13) + power pin (D26)
 * - 1602 I2C LCD at 0x27 (adjust if needed)
 *
 * Module-only usage:
 * - Pulls in ByByteMotor (for differential drive) plus external QTR + LCD libs.
 */
#include <MotorDriver.h>
#include <ByByteCore.h>
#include <QTRSensors.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

using namespace ByByte;

MotorDriver motor(ControlMode::Differential);
QTRSensors qtr;
LiquidCrystal_I2C lcd(0x27, 16, 2);

static const uint8_t qtrPins[] = { BYBYTE_LINE_PIN_0, BYBYTE_LINE_PIN_1, BYBYTE_LINE_PIN_2,
	BYBYTE_LINE_PIN_3, BYBYTE_LINE_PIN_4, BYBYTE_LINE_PIN_5 };
const uint8_t SENSOR_COUNT = sizeof(qtrPins) / sizeof(qtrPins[0]);

// Use signed types to avoid unsigned wrap in math
static inline int16_t clampI16(int32_t v, int16_t lo, int16_t hi) {
	if (v < lo) return lo;
	if (v > hi) return hi;
	return (int16_t)v;
}

const int16_t MAX_PWM = (int16_t)BYBYTE_MAX_PWM;     // typically 255
const int16_t MAX_SPEED = MAX_PWM / 2;               // cap for this example
const int16_t BASE_SPEED = MAX_PWM / 3;              // base forward speed
const float Kp = 0.35f;                              // proportional gain

void setup() {
	Serial.begin(115200);
	// Power line sensors
	pinMode(BYBYTE_LINE_PWR_PIN, OUTPUT);
	digitalWrite(BYBYTE_LINE_PWR_PIN, HIGH);
	delay(10);
	// QTR setup
	qtr.setTypeAnalog();
	qtr.setSensorPins(qtrPins, SENSOR_COUNT);
	// LCD
	lcd.init();
	lcd.backlight();
	lcd.setCursor(0,0);
	lcd.print("Line Follow MEGA");
	// Motors
	motor.begin();
	// Calibrate sensors briefly
	for (uint16_t i=0;i<200;i++) { qtr.calibrate(); delay(5); }
}

void showIndicators(const uint16_t* vals) {
	lcd.setCursor(0,1);
	for (uint8_t i=0;i<SENSOR_COUNT; i++) {
		bool onLine = vals[i] > 500; // calibrated 0..1000
		lcd.print(onLine ? (char)255 : '_');
		if (i < SENSOR_COUNT-1) lcd.print(' ');
	}
	for (uint8_t k = SENSOR_COUNT*2; k < 16; k++) lcd.print(' ');
}

void loop() {
	uint16_t sensorValues[SENSOR_COUNT];
	uint16_t position = qtr.readLineBlack(sensorValues);
	uint16_t center = (SENSOR_COUNT - 1) * 500;
	int32_t error = (int32_t)position - (int32_t)center;

	// PWM-like turn correction from sensor error
	int16_t w_pwm = (int16_t)(Kp * (float)error);
	if (w_pwm > MAX_SPEED) w_pwm = MAX_SPEED; else if (w_pwm < -MAX_SPEED) w_pwm = -MAX_SPEED;

	// Optional: slow down if line is likely lost
	uint32_t sum = 0; for (uint8_t i=0;i<SENSOR_COUNT;i++) sum += sensorValues[i];
	int16_t base_pwm = (sum < 800) ? (BASE_SPEED / 3) : BASE_SPEED;

	// Map desired PWM-like base/turn to physical v,w so diff controller yields intended PWM
	const float radius = BYBYTE_WHEEL_RADIUS_M;               // meters
	const float halfL = BYBYTE_WHEEL_SEPARATION_M * 0.5f;     // meters
	float v = (float)base_pwm * radius;                       // v_PWM / invR
	float w = (float)w_pwm * radius / halfL;                  // w_PWM / (halfL*invR)

	motor.setTargetVelocity(v, w);
	motor.update();

	showIndicators(sensorValues);
	delay(10);
}