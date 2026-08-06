#ifndef BB_PINS_NANO_BOY_H
#define BB_PINS_NANO_BOY_H

#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <stdint.h>
#endif

// NanoBoy console: stub only until GPIO / stdin / HID / simulator wiring exists.

namespace bb {
namespace pins {
namespace nano_boy {

constexpr uint8_t kPinUnset = static_cast<uint8_t>(0xFF);

struct NanoBoyPinLayout {
	uint8_t reservedA = kPinUnset;
	uint8_t reservedB = kPinUnset;
};

inline constexpr NanoBoyPinLayout defaults() {
	return {};
}

} // namespace nano_boy
} // namespace pins
} // namespace bb

#endif // BB_PINS_NANO_BOY_H
