#ifndef BYBYTE_PRODUCT_H
#define BYBYTE_PRODUCT_H

#include <stdint.h>

namespace ByByte {

/**
 * Product kits are chosen explicitly in code (constructor you pick), not compile-time macros.
 * Board pin defaults stay in configs/ByByteConfig for the MCU target chosen in the IDE /
 * PlatformIO; see ByByteKits.h for bundles that expose MotorDriver wired to defaults.
 */

enum class PlatformKit : uint8_t {
	Nano = 0,
	Mega = 1,
	NanoBoy = 2,
};

/** Base type for an explicitly picked product/platform. */
class ByByteKit {
public:
	virtual ~ByByteKit() = default;

	virtual PlatformKit kit() const noexcept = 0;
	virtual const char* name() const noexcept = 0;

protected:
	ByByteKit() = default;
};

} // namespace ByByte

#endif // BYBYTE_PRODUCT_H
