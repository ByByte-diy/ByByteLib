#ifndef BYBYTE_TYPES_H
#define BYBYTE_TYPES_H

#include <Arduino.h>

namespace ByByte {

struct Twist {
	float linearX;   // m/s or arbitrary units
	float angularZ;  // rad/s or arbitrary units
};

struct MotorSpeeds {
	int16_t left;   // PWM or percent (-255..255)
	int16_t right;  // PWM or percent (-255..255)
};

} // namespace ByByte

#endif // BYBYTE_TYPES_H

