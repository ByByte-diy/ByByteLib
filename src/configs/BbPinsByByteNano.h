#ifndef BB_PINS_BYBYTE_NANO_H
#define BB_PINS_BYBYTE_NANO_H

#include <stdint.h>

#include "ByByteConfig.h"

// Pin map draft for ByByteNano — values mirror BYBYTE_PLATFORM_NANO defaults in ByByteConfig.h.
// Scoped to Nano builds so Mega firmware does not see undefined macros.
// For full pin macro resolution (e.g. A*) on real hardware, ensure <Arduino.h> is included before
// ByByteConfig in the translation unit (ByByteLib.h does this).

namespace bb {
namespace pins {
namespace by_byte_nano {

struct DRV8833MotorPins {
	uint8_t leftIn1;
	uint8_t leftIn2;
	uint8_t rightIn1;
	uint8_t rightIn2;
};

/** SoftwareSerial HC-0x wiring (Nano default: D2/D3 — see ByByteConfig). */
struct BtSoftwareSerialPins {
	uint8_t rx;
	uint8_t tx;
};

#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO
inline constexpr DRV8833MotorPins defaultMotorsNano() {
	return {
	  BYBYTE_NANO_LEFT_IN1,
	  BYBYTE_NANO_LEFT_IN2,
	  BYBYTE_NANO_RIGHT_IN1,
	  BYBYTE_NANO_RIGHT_IN2,
	};
}
#endif // BYBYTE_PLATFORM_NANO

/** Default HC-0x SoftwareSerial pins for the current MCU (uses ByByteConfig macros). */
inline constexpr BtSoftwareSerialPins defaultBluetoothSoftwareSerialPins() {
	return { BYBYTE_BT_SW_RX_PIN, BYBYTE_BT_SW_TX_PIN };
}

} // namespace by_byte_nano
} // namespace pins
} // namespace bb

#endif // BB_PINS_BYBYTE_NANO_H
