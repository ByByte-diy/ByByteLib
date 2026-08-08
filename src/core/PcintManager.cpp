#include "PcintManager.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)

#if defined(__AVR_ATmega2560__)
// Group 0 -> Port B, Group 1 -> Port J, Group 2 -> Port K
#define PIN_REG0  PINB
#define PIN_REG1  PINJ
#define PIN_REG2  PINK
#define DDR_REG1  DDRJ
#define PORT_REG1 PORTJ
#define DDR_REG2  DDRK
#define PORT_REG2 PORTK
#else
// ATmega328P/168 (Nano, Uno): Group 0 -> Port B, Group 1 -> Port C, Group 2 -> Port D
#define PIN_REG0  PINB
#define PIN_REG1  PINC
#define PIN_REG2  PIND
#define DDR_REG1  DDRC
#define PORT_REG1 PORTC
#define DDR_REG2  DDRD
#define PORT_REG2 PORTD
#endif

namespace ByByte {

	static inline uint8_t readGroup0() {
		return PIN_REG0;
	}

	static inline uint8_t readGroup1() {
		return PIN_REG1;
	}

	static inline uint8_t readGroup2() {
		return PIN_REG2;
	}

	void (*PcintManager::_handlers0[8])() = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	void (*PcintManager::_handlers1[8])() = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	void (*PcintManager::_handlers2[8])() = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	void (*PcintManager::_portHook0)(uint8_t) = nullptr;
	void (*PcintManager::_portHook1)(uint8_t) = nullptr;
	void (*PcintManager::_portHook2)(uint8_t) = nullptr;
	uint8_t PcintManager::_last0 = 0;
	uint8_t PcintManager::_last1 = 0;
	uint8_t PcintManager::_last2 = 0;
	bool PcintManager::_initialized = false;

	void PcintManager::init() {
		cli();
		_last0 = readGroup0();
		_last1 = readGroup1();
		_last2 = readGroup2();
		_initialized = true;
		sei();
	}

	void PcintManager::ensureInitialized() {
		if (!_initialized) { init(); }
	}

	bool PcintManager::mapToGroupAndBit(uint8_t pcintNumber, uint8_t& groupIdx, uint8_t& bitIdx) {
		if (pcintNumber <= 7) { groupIdx = 0; bitIdx = pcintNumber; return true; }
		if (pcintNumber >= 8 && pcintNumber <= 15) { groupIdx = 1; bitIdx = pcintNumber - 8; return true; }
		if (pcintNumber >= 16 && pcintNumber <= 23) { groupIdx = 2; bitIdx = pcintNumber - 16; return true; }
		return false;
	}

	void PcintManager::updateGroupEnable(uint8_t groupIdx) {
		cli();
		if (groupIdx == 0) {
			if (PCMSK0) PCICR |= (1 << PCIE0); else PCICR &= ~(1 << PCIE0);
		} else if (groupIdx == 1) {
			if (PCMSK1) PCICR |= (1 << PCIE1); else PCICR &= ~(1 << PCIE1);
		} else if (groupIdx == 2) {
			if (PCMSK2) PCICR |= (1 << PCIE2); else PCICR &= ~(1 << PCIE2);
		}
		sei();
	}

	bool PcintManager::setMaskBit(uint8_t pcintNumber, bool enable) {
		uint8_t groupIdx, bitIdx;
		if (!mapToGroupAndBit(pcintNumber, groupIdx, bitIdx)) return false;
		cli();
		if (groupIdx == 0) {
			if (enable) PCMSK0 |= (1 << bitIdx); else PCMSK0 &= ~(1 << bitIdx);
		} else if (groupIdx == 1) {
			if (enable) PCMSK1 |= (1 << bitIdx); else PCMSK1 &= ~(1 << bitIdx);
		} else if (groupIdx == 2) {
			if (enable) PCMSK2 |= (1 << bitIdx); else PCMSK2 &= ~(1 << bitIdx);
		}
		sei();
		updateGroupEnable(groupIdx);
		return true;
	}

	bool PcintManager::configurePcint(uint8_t pcintNumber, bool enablePullup) {
		ensureInitialized();
		uint8_t groupIdx, bitIdx;
		if (!mapToGroupAndBit(pcintNumber, groupIdx, bitIdx)) return false;
		if (groupIdx == 0) {
			uint8_t pin = bitIdx;
			if (enablePullup) { DDRB &= ~(1 << pin); PORTB |= (1 << pin); } else { DDRB &= ~(1 << pin); }
			_last0 = readGroup0();
		} else if (groupIdx == 1) {
			uint8_t pin = bitIdx;
			if (enablePullup) { DDR_REG1 &= ~(1 << pin); PORT_REG1 |= (1 << pin); } else { DDR_REG1 &= ~(1 << pin); }
			_last1 = readGroup1();
		} else if (groupIdx == 2) {
			uint8_t pin = bitIdx;
			if (enablePullup) { DDR_REG2 &= ~(1 << pin); PORT_REG2 |= (1 << pin); } else { DDR_REG2 &= ~(1 << pin); }
			_last2 = readGroup2();
		}
		return setMaskBit(pcintNumber, true);
	}

	bool PcintManager::subscribe(uint8_t pcintNumber, void (*handler)(), bool enablePullup) {
		if (!configurePcint(pcintNumber, enablePullup)) return false;
		uint8_t groupIdx, bitIdx;
		mapToGroupAndBit(pcintNumber, groupIdx, bitIdx);
		if (groupIdx == 0) _handlers0[bitIdx] = handler;
		else if (groupIdx == 1) _handlers1[bitIdx] = handler;
		else if (groupIdx == 2) _handlers2[bitIdx] = handler;
		return true;
	}

	bool PcintManager::subscribePin(uint8_t arduinoPin, void (*handler)(), bool enablePullup) {
		// Arduino-core pin map: PCIE bit index (group 0..2) and mask bit (bit 0..7).
		volatile uint8_t* pcmsk = digitalPinToPCMSK(arduinoPin);
		if (!pcmsk) return false;                 // not PCINT-capable
		uint8_t groupIdx = digitalPinToPCICRbit(arduinoPin); // 0..2
		uint8_t bitIdx   = digitalPinToPCMSKbit(arduinoPin);  // 0..7
		uint8_t pcintNumber = (uint8_t)(groupIdx * 8 + bitIdx);
		return subscribe(pcintNumber, handler, enablePullup);
	}

	void PcintManager::unsubscribe(uint8_t pcintNumber) {
		uint8_t groupIdx, bitIdx;
		if (!mapToGroupAndBit(pcintNumber, groupIdx, bitIdx)) return;
		if (groupIdx == 0) _handlers0[bitIdx] = nullptr;
		else if (groupIdx == 1) _handlers1[bitIdx] = nullptr;
		else if (groupIdx == 2) _handlers2[bitIdx] = nullptr;
		setMaskBit(pcintNumber, false);
	}

	void PcintManager::handleGroup0(uint8_t changedMask, uint8_t current) {
		if (_portHook0) _portHook0(current);   // e.g. NeoSWSerial::rxISR(PINB/PINJ)
		for (uint8_t i = 0; i < 8; ++i) if (changedMask & (1 << i)) { if (_handlers0[i]) _handlers0[i](); }
		_last0 = current;
	}

	void PcintManager::handleGroup1(uint8_t changedMask, uint8_t current) {
		if (_portHook1) _portHook1(current);
		for (uint8_t i = 0; i < 8; ++i) if (changedMask & (1 << i)) { if (_handlers1[i]) _handlers1[i](); }
		_last1 = current;
	}

	void PcintManager::handleGroup2(uint8_t changedMask, uint8_t current) {
		if (_portHook2) _portHook2(current);
		for (uint8_t i = 0; i < 8; ++i) if (changedMask & (1 << i)) { if (_handlers2[i]) _handlers2[i](); }
		_last2 = current;
	}

	void PcintManager::setPortHook(uint8_t groupIdx, void (*handler)(uint8_t)) {
		cli();
		if (groupIdx == 0) _portHook0 = handler;
		else if (groupIdx == 1) _portHook1 = handler;
		else if (groupIdx == 2) _portHook2 = handler;
		sei();
	}

	void PcintManager::clearPortHook(uint8_t groupIdx) {
		setPortHook(groupIdx, nullptr);
	}

} // namespace ByByte

// PCINT vector ownership:
//   PcintManager is the single owner of the Pin Change Interrupt vectors on
//   every supported AVR board (ATmega2560 and ATmega328P/168). Bluetooth on the
//   Nano uses NeoSWSerial compiled with NEOSWSERIAL_EXTERNAL_PCINT, so it does
//   NOT emit any ISR(PCINTx_vect) and instead receives byte edges through
//   PcintManager's per-group raw-port hook (setPortHook -> NeoSWSerial::rxISR).
//   The classic SoftwareSerial library is no longer linked anywhere, so there
//   is no vector clash.
//
//   Opt out (e.g. if a third party links its own PCINT ISRs) by defining
//   BYBYTE_DISABLE_PCINT_VECTORS before this TU is compiled.
#if !defined(BYBYTE_DISABLE_PCINT_VECTORS)

ISR(PCINT0_vect) {
	using namespace ByByte;
	uint8_t cur = PIN_REG0;
	static uint8_t last = cur;
	uint8_t changed = cur ^ last;
	if (changed) PcintManager::handleGroup0(changed, cur);
	last = cur;
}

ISR(PCINT1_vect) {
	using namespace ByByte;
	uint8_t cur = PIN_REG1;
	static uint8_t last = cur;
	uint8_t changed = cur ^ last;
	if (changed) PcintManager::handleGroup1(changed, cur);
	last = cur;
}

ISR(PCINT2_vect) {
	using namespace ByByte;
	uint8_t cur = PIN_REG2;
	static uint8_t last = cur;
	uint8_t changed = cur ^ last;
	if (changed) PcintManager::handleGroup2(changed, cur);
	last = cur;
}

#endif // !BYBYTE_DISABLE_PCINT_VECTORS

#endif // supported boards
