/*
 * ByByteLib - Line Follow (Arduino Nano, QTR RC sensors)
 *
 * Purpose:
 * - Line following using QTR RC (digital) sensors and P-control
 * - Computes base speed + proportional turn, feeds Differential controller
 *
 * Hardware:
 * - Nano: 5 digital line sensors (A0..A4 via Schmitt triggers)
 * - Pins auto-detected from configs/ByByteConfig
 */
#include <ByByteLib.h>
#include <QTRSensors.h>

using namespace ByByte;

MotorDriver motor(MotorDriver::driverForBuildTarget(), {}, ControlMode::Differential);
QTRSensors qtr;

static const uint8_t qtrPins[] = { BYBYTE_LINE_PIN_0, BYBYTE_LINE_PIN_1, BYBYTE_LINE_PIN_2,
	BYBYTE_LINE_PIN_3, BYBYTE_LINE_PIN_4 };
const uint8_t SENSOR_COUNT = sizeof(qtrPins) / sizeof(qtrPins[0]);

const int16_t MAX_PWM = (int16_t)BYBYTE_MAX_PWM;     // typically 255
const int16_t MAX_SPEED = MAX_PWM / 2;               // cap for this example
const int16_t BASE_SPEED = MAX_PWM / 3;              // base forward speed
const float Kp = 0.35f;                              // proportional gain

void setup() {
	Serial.begin(115200);
	qtr.setTypeRC();
	qtr.setSensorPins(qtrPins, SENSOR_COUNT);
	motor.begin();
	for (uint16_t i=0;i<200;i++) { qtr.calibrate(); delay(5); }
}

void loop() {
	uint16_t sensorValues[SENSOR_COUNT];
	uint16_t position = qtr.readLineBlack(sensorValues);
	uint16_t center = (SENSOR_COUNT - 1) * 500;
	int32_t error = (int32_t)position - (int32_t)center;

	int16_t w_pwm = (int16_t)(Kp * (float)error);
	if (w_pwm > MAX_SPEED) w_pwm = MAX_SPEED; else if (w_pwm < -MAX_SPEED) w_pwm = -MAX_SPEED;

	uint32_t sum = 0; for (uint8_t i=0;i<SENSOR_COUNT;i++) sum += sensorValues[i];
	int16_t base_pwm = (sum < 800) ? (BASE_SPEED / 3) : BASE_SPEED;

	const float radius = BYBYTE_WHEEL_RADIUS_M;               // meters
	const float halfL = BYBYTE_WHEEL_SEPARATION_M * 0.5f;     // meters
	float v = (float)base_pwm * radius;                       // v_PWM / invR
	float w = (float)w_pwm * radius / halfL;                  // w_PWM / (halfL*invR)

	motor.setTargetVelocity(v, w);
	motor.update();
	delay(10);
}
