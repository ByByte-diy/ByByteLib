#ifndef BYBYTE_TYPES_H
#define BYBYTE_TYPES_H

#include <Arduino.h>

namespace ByByte {

struct Twist {
	float linearX;   // m/s or arbitrary units
	float angularZ;  // rad/s or arbitrary units
};

} // namespace ByByte

#endif // BYBYTE_TYPES_H

