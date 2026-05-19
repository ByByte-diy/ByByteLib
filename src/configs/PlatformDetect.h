#ifndef BYBYTE_PLATFORM_DETECT_H
#define BYBYTE_PLATFORM_DETECT_H

// Numeric platform identifiers for preprocessor conditions
#define BYBYTE_PLATFORM_UNKNOWN 0
#define BYBYTE_PLATFORM_NANO 1
#define BYBYTE_PLATFORM_MEGA 2

// Basic board detection using Arduino predefined macros
#if defined(ARDUINO_AVR_NANO)
	#define BYBYTE_PLATFORM_ID BYBYTE_PLATFORM_NANO
#elif defined(ARDUINO_AVR_MEGA2560) || defined(ARDUINO_AVR_MEGA)
	#define BYBYTE_PLATFORM_ID BYBYTE_PLATFORM_MEGA
#else
	#define BYBYTE_PLATFORM_ID BYBYTE_PLATFORM_UNKNOWN
#endif

#ifdef __cplusplus
#include <stdint.h>
namespace ByByte {
inline uint8_t hardwarePlatformChip() noexcept {
	return static_cast<uint8_t>(BYBYTE_PLATFORM_ID);
}
} // namespace ByByte
#endif

#endif // BYBYTE_PLATFORM_DETECT_H

