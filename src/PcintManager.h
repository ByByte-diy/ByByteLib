#ifndef BYBYTE_PCINT_MANAGER_H
#define BYBYTE_PCINT_MANAGER_H

#include <Arduino.h>

namespace ByByte {

// Global Pin Change Interrupt manager supporting common AVR Arduinos
// - ATmega328P/168 (Arduino Nano, Uno):
//   PCINT0..7 -> Port B, PCINT8..14 -> Port C, PCINT16..23 -> Port D
// - ATmega2560 (Arduino Mega):
//   PCINT0..7 -> Port B, PCINT8..15 -> Port J, PCINT16..23 -> Port K
class PcintManager {
public:
	// Initialize manager (idempotent). Called automatically on first subscribe.
	static void init();
	// Subscribe handler for a PCINT number. Handler called on any edge (change).
	// If enablePullup is true, input with pull-up is configured automatically.
	static bool subscribe(uint8_t pcintNumber, void (*handler)(), bool enablePullup = true);
	static void unsubscribe(uint8_t pcintNumber);

	// Internal ISRs dispatch
	static void handleGroup0(uint8_t changedMask, uint8_t current);
	static void handleGroup1(uint8_t changedMask, uint8_t current);
	static void handleGroup2(uint8_t changedMask, uint8_t current);
private:
	static void ensureInitialized();
	static bool configurePcint(uint8_t pcintNumber, bool enablePullup);
	static bool setMaskBit(uint8_t pcintNumber, bool enable);
	static bool mapToGroupAndBit(uint8_t pcintNumber, uint8_t& groupIdx, uint8_t& bitIdx);
	static void updateGroupEnable(uint8_t groupIdx);

	static void (*_handlers0[8])();
	static void (*_handlers1[8])();
	static void (*_handlers2[8])();
	static uint8_t _last0;
	static uint8_t _last1;
	static uint8_t _last2;
	static bool _initialized;
};

} // namespace ByByte

#endif // BYBYTE_PCINT_MANAGER_H
