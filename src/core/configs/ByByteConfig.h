#ifndef BYBYTE_CONFIG_H
#define BYBYTE_CONFIG_H

#include "PlatformDetect.h"

// Configure default update rate in Hz
#ifndef BYBYTE_DEFAULT_UPDATE_HZ
#define BYBYTE_DEFAULT_UPDATE_HZ 50
#endif

// Geometry and control defaults per platform
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO
	// Nano + DRV8833
#ifndef BYBYTE_WHEEL_SEPARATION_M
#define BYBYTE_WHEEL_SEPARATION_M 0.11f
#endif
#ifndef BYBYTE_WHEEL_RADIUS_M
#define BYBYTE_WHEEL_RADIUS_M 0.045f
#endif
#ifndef BYBYTE_MAX_PWM
#define BYBYTE_MAX_PWM 220
#endif
// Pins: Motor1=right (D5,D6), Motor2=left (D9,D10)
#ifndef BYBYTE_NANO_RIGHT_IN1
#define BYBYTE_NANO_RIGHT_IN1 5
#endif
#ifndef BYBYTE_NANO_RIGHT_IN2
#define BYBYTE_NANO_RIGHT_IN2 6
#endif
#ifndef BYBYTE_NANO_LEFT_IN1
#define BYBYTE_NANO_LEFT_IN1 10
#endif
#ifndef BYBYTE_NANO_LEFT_IN2
#define BYBYTE_NANO_LEFT_IN2 9
#endif
// Side IR sensors
#ifndef BYBYTE_IR_LEFT_PIN
#define BYBYTE_IR_LEFT_PIN A6
#endif
#ifndef BYBYTE_IR_RIGHT_PIN
#define BYBYTE_IR_RIGHT_PIN A7
#endif
#ifndef BYBYTE_IR_POWER_PIN
#define BYBYTE_IR_POWER_PIN 4
#endif
// LDR sensor (mandatory on Nano)
#ifndef BYBYTE_LDR_ADC_PIN
#define BYBYTE_LDR_ADC_PIN A5
#endif
// Optional sensors not present on Nano (define for compatibility)
#ifndef BYBYTE_BAT_ADC_PIN
#define BYBYTE_BAT_ADC_PIN A0
#endif
#ifndef BYBYTE_BAT_VREF_MV
#define BYBYTE_BAT_VREF_MV 5000
#endif
#ifndef BYBYTE_CHRG_PIN
#define BYBYTE_CHRG_PIN 0
#endif
#ifndef BYBYTE_TMP_ADC_PIN
#define BYBYTE_TMP_ADC_PIN A1
#endif
// Bluetooth (Nano): SoftwareSerial D2(RX), D3(TX), always powered
#ifndef BYBYTE_BT_SW_RX_PIN
#define BYBYTE_BT_SW_RX_PIN 2
#endif
#ifndef BYBYTE_BT_SW_TX_PIN
#define BYBYTE_BT_SW_TX_PIN 3
#endif
// WS2812 (Nano): 2 LEDs as headlights (and extra)
#ifndef BYBYTE_WS2812_PIN
#define BYBYTE_WS2812_PIN 7
#endif
#ifndef BYBYTE_WS2812_COUNT
#define BYBYTE_WS2812_COUNT 2
#endif
// Horn (Nano)
#ifndef BYBYTE_HORN_PIN
#define BYBYTE_HORN_PIN 11
#endif
// Line sensors (Nano): 5 digital sensors on A0..A4
#ifndef BYBYTE_LINE_COUNT
#define BYBYTE_LINE_COUNT 5
#endif
#ifndef BYBYTE_LINE_IS_DIGITAL
#define BYBYTE_LINE_IS_DIGITAL 1
#endif
#ifndef BYBYTE_LINE_PIN_0
#define BYBYTE_LINE_PIN_0 A0 // leftmost
#endif
#ifndef BYBYTE_LINE_PIN_1
#define BYBYTE_LINE_PIN_1 A1
#endif
#ifndef BYBYTE_LINE_PIN_2
#define BYBYTE_LINE_PIN_2 A2 // center
#endif
#ifndef BYBYTE_LINE_PIN_3
#define BYBYTE_LINE_PIN_3 A3
#endif
#ifndef BYBYTE_LINE_PIN_4
#define BYBYTE_LINE_PIN_4 A4 // rightmost
#endif
#elif BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA

#include <HardwareSerial.h>

	// Mega + TB6612
#ifndef BYBYTE_WHEEL_SEPARATION_M
#define BYBYTE_WHEEL_SEPARATION_M 0.11f
#endif
#ifndef BYBYTE_WHEEL_RADIUS_M
#define BYBYTE_WHEEL_RADIUS_M 0.045f
#endif
#ifndef BYBYTE_MAX_PWM
#define BYBYTE_MAX_PWM 255
#endif
// TB6612 pins
#ifndef BYBYTE_TB6612_STBY
#define BYBYTE_TB6612_STBY 49
#endif
#ifndef BYBYTE_TB6612_LEFT_IN1
#define BYBYTE_TB6612_LEFT_IN1 47
#endif
#ifndef BYBYTE_TB6612_LEFT_IN2
#define BYBYTE_TB6612_LEFT_IN2 48
#endif
#ifndef BYBYTE_TB6612_LEFT_PWM
#define BYBYTE_TB6612_LEFT_PWM 46
#endif
#ifndef BYBYTE_TB6612_RIGHT_IN1
#define BYBYTE_TB6612_RIGHT_IN1 43
#endif
#ifndef BYBYTE_TB6612_RIGHT_IN2
#define BYBYTE_TB6612_RIGHT_IN2 42
#endif
#ifndef BYBYTE_TB6612_RIGHT_PWM
#define BYBYTE_TB6612_RIGHT_PWM 44
#endif
// Encoders (optional)
#ifndef BYBYTE_ENCODER_LEFT_PIN
#define BYBYTE_ENCODER_LEFT_PIN 17
#endif
#ifndef BYBYTE_ENCODER_RIGHT_PIN
#define BYBYTE_ENCODER_RIGHT_PIN 15
#endif
#ifndef BYBYTE_ENCODER_PPR
#define BYBYTE_ENCODER_PPR 12
#endif
// Side IR sensors
#ifndef BYBYTE_IR_LEFT_PIN
#define BYBYTE_IR_LEFT_PIN A14
#endif
#ifndef BYBYTE_IR_RIGHT_PIN
#define BYBYTE_IR_RIGHT_PIN A15
#endif
#ifndef BYBYTE_IR_POWER_PIN
#define BYBYTE_IR_POWER_PIN 22
#endif
// Servo pins mapping: D30/D31/D32
#ifndef BYBYTE_SERVO0_PIN
#define BYBYTE_SERVO0_PIN 30
#endif
#ifndef BYBYTE_SERVO1_PIN
#define BYBYTE_SERVO1_PIN 31
#endif
#ifndef BYBYTE_SERVO2_PIN
#define BYBYTE_SERVO2_PIN 32
#endif
// Optional sensors: Battery/Temp/LDR
#ifndef BYBYTE_BAT_ADC_PIN
#define BYBYTE_BAT_ADC_PIN A0
#endif
#ifndef BYBYTE_BAT_VREF_MV
#define BYBYTE_BAT_VREF_MV 5000
#endif
#ifndef BYBYTE_TMP_ADC_PIN
#define BYBYTE_TMP_ADC_PIN A1
#endif
#ifndef BYBYTE_LDR_ADC_PIN
#define BYBYTE_LDR_ADC_PIN A2
#endif
// Charger status input (CHRG on PJ0 -> D15), LOW = charging
#ifndef BYBYTE_CHRG_PIN
#define BYBYTE_CHRG_PIN 15
#endif
// Line sensors (Mega): 6 analog sensors A8..A13, power on D26
#ifndef BYBYTE_LINE_COUNT
#define BYBYTE_LINE_COUNT 6
#endif
#ifndef BYBYTE_LINE_IS_DIGITAL
#define BYBYTE_LINE_IS_DIGITAL 0
#endif
#ifndef BYBYTE_LINE_PWR_PIN
#define BYBYTE_LINE_PWR_PIN 26
#endif
#ifndef BYBYTE_LINE_PIN_0
#define BYBYTE_LINE_PIN_0 A8 // leftmost
#endif
#ifndef BYBYTE_LINE_PIN_1
#define BYBYTE_LINE_PIN_1 A9
#endif
#ifndef BYBYTE_LINE_PIN_2
#define BYBYTE_LINE_PIN_2 A10
#endif
#ifndef BYBYTE_LINE_PIN_3
#define BYBYTE_LINE_PIN_3 A11
#endif
#ifndef BYBYTE_LINE_PIN_4
#define BYBYTE_LINE_PIN_4 A12
#endif
#ifndef BYBYTE_LINE_PIN_5
#define BYBYTE_LINE_PIN_5 A13 // rightmost
#endif
// Bluetooth (Mega): Serial1 (TX1/RX1) and power control D29
#ifndef BYBYTE_BT_UART
#define BYBYTE_BT_UART Serial1
#endif
#ifndef BYBYTE_BT_PWR_PIN
#define BYBYTE_BT_PWR_PIN 29
#endif
// WS2812 (Mega): 4 LEDs: 0-1 headlights, 2-3 tail
#ifndef BYBYTE_WS2812_PIN
#define BYBYTE_WS2812_PIN 23
#endif
#ifndef BYBYTE_WS2812_COUNT
#define BYBYTE_WS2812_COUNT 4
#endif
// Headlights (Mega) - discrete white LEDs
#ifndef BYBYTE_HEADLIGHT_LEFT_PIN
#define BYBYTE_HEADLIGHT_LEFT_PIN 36
#endif
#ifndef BYBYTE_HEADLIGHT_RIGHT_PIN
#define BYBYTE_HEADLIGHT_RIGHT_PIN 37
#endif
// Horn (Mega)
#ifndef BYBYTE_HORN_PIN
#define BYBYTE_HORN_PIN 45
#endif
// User input buttons (active LOW)
#ifndef BYBYTE_BTN1_PIN
#define BYBYTE_BTN1_PIN 39
#endif
#ifndef BYBYTE_BTN2_PIN
#define BYBYTE_BTN2_PIN 40
#endif
#ifndef BYBYTE_BTN3_PIN
#define BYBYTE_BTN3_PIN 41
#endif
// Additional discrete LEDs
#ifndef BYBYTE_LED_BLUE_PIN
#define BYBYTE_LED_BLUE_PIN 33
#endif
#ifndef BYBYTE_LED_GREEN_PIN
#define BYBYTE_LED_GREEN_PIN 34
#endif
#ifndef BYBYTE_LED_RED_PIN
#define BYBYTE_LED_RED_PIN 35
#endif
// 12V power control
#ifndef BYBYTE_PWR12_EN_PIN
#define BYBYTE_PWR12_EN_PIN 27
#endif
// Capacitive touch button
#ifndef BYBYTE_TOUCH_PIN
#define BYBYTE_TOUCH_PIN 28
#endif
// Shock sensor (PJ5)
#ifndef BYBYTE_SHOCK_PIN
#define BYBYTE_SHOCK_PIN PJ5
#endif
#else
	// Unknown defaults for line sensors
#ifndef BYBYTE_LINE_COUNT
#define BYBYTE_LINE_COUNT 0
#endif
#ifndef BYBYTE_LINE_IS_DIGITAL
#define BYBYTE_LINE_IS_DIGITAL 0
#endif
// Bluetooth (fallback): generic SoftwareSerial pins if not overridden
#ifndef BYBYTE_BT_SW_RX_PIN
#define BYBYTE_BT_SW_RX_PIN 2
#endif
#ifndef BYBYTE_BT_SW_TX_PIN
#define BYBYTE_BT_SW_TX_PIN 3
#endif
#endif

#endif // BYBYTE_CONFIG_H

