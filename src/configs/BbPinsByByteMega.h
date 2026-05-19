#ifndef BB_PINS_BYBYTE_MEGA_H
#define BB_PINS_BYBYTE_MEGA_H

#include <stdint.h>

#include "ByByteConfig.h"

// Pin map draft for ByByteMega — values mirror TB6612 defaults in ByByteConfig.h (Mega).

namespace bb {
namespace pins {
namespace by_byte_mega {

struct TB6612ChannelPins {
	uint8_t ain1;
	uint8_t ain2;
	uint8_t pwm;
};

struct TB6612DualMotorsPins {
	TB6612ChannelPins left;
	TB6612ChannelPins right;
	uint8_t stby;
};

/** Mega: UART is BYBYTE_BT_UART (Serial1); power gating on a digital pin. */
struct BtMegaBluetoothPins {
	uint8_t powerPin;
};

#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
inline constexpr TB6612DualMotorsPins defaultMegaMotorsTb6612() {
	return TB6612DualMotorsPins{
	  { BYBYTE_TB6612_LEFT_IN1, BYBYTE_TB6612_LEFT_IN2, BYBYTE_TB6612_LEFT_PWM },
	  { BYBYTE_TB6612_RIGHT_IN1, BYBYTE_TB6612_RIGHT_IN2, BYBYTE_TB6612_RIGHT_PWM },
	  BYBYTE_TB6612_STBY,
	};
}

inline constexpr BtMegaBluetoothPins defaultBluetoothMega() {
	return { BYBYTE_BT_PWR_PIN };
}
#endif // BYBYTE_PLATFORM_MEGA

} // namespace by_byte_mega
} // namespace pins
} // namespace bb

#endif // BB_PINS_BYBYTE_MEGA_H
