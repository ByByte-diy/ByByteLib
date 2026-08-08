#ifndef BYBYTE_PIN_CAPABILITIES_H
#define BYBYTE_PIN_CAPABILITIES_H

#include <Arduino.h>

// Return whether a pin supports PWM on the active platform
static inline constexpr bool isPwmPin(uint8_t pin) {
	#if defined(ARDUINO_AVR_NANO) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
		// Nano/UNO PWM pins: 3,5,6,9,10,11
		return (pin == 3) || (pin == 5) || (pin == 6) || (pin == 9) || (pin == 10) || (pin == 11);
	#elif defined(ARDUINO_AVR_MEGA2560) || defined(__AVR_ATmega2560__)
		// Mega PWM pins (common set): 2-13 and 44,45,46
		return (pin == 2) || (pin == 3) || (pin == 4) || (pin == 5) || (pin == 6) || (pin == 7) ||
			   (pin == 8) || (pin == 9) || (pin == 10) || (pin == 11) || (pin == 12) || (pin == 13) ||
			   (pin == 44) || (pin == 45) || (pin == 46);
	#else
		// Unknown platform: assume true to avoid false negatives
		return true;
	#endif
}

#endif // BYBYTE_PIN_CAPABILITIES_H
