#include "PcintManager.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#if defined(__AVR_ATmega2560__)

namespace ByByte {

static inline uint8_t readGroup0() {
	return PINB;
}

static inline uint8_t readGroup1() {
	return PINJ;
}

static inline uint8_t readGroup2() {
	return PINK;
}

void (*PcintManager::_handlers0[8])() = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
void (*PcintManager::_handlers1[8])() = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
void (*PcintManager::_handlers2[8])() = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
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
		if (enablePullup) { DDRJ &= ~(1 << pin); PORTJ |= (1 << pin); } else { DDRJ &= ~(1 << pin); }
		_last1 = readGroup1();
	} else if (groupIdx == 2) {
		uint8_t pin = bitIdx;
		if (enablePullup) { DDRK &= ~(1 << pin); PORTK |= (1 << pin); } else { DDRK &= ~(1 << pin); }
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

void PcintManager::unsubscribe(uint8_t pcintNumber) {
	uint8_t groupIdx, bitIdx;
	if (!mapToGroupAndBit(pcintNumber, groupIdx, bitIdx)) return;
	if (groupIdx == 0) _handlers0[bitIdx] = nullptr;
	else if (groupIdx == 1) _handlers1[bitIdx] = nullptr;
	else if (groupIdx == 2) _handlers2[bitIdx] = nullptr;
	setMaskBit(pcintNumber, false);
}

void PcintManager::handleGroup0(uint8_t changedMask, uint8_t current) {
	for (uint8_t i = 0; i < 8; ++i) if (changedMask & (1 << i)) { if (_handlers0[i]) _handlers0[i](); }
	_last0 = current;
}

void PcintManager::handleGroup1(uint8_t changedMask, uint8_t current) {
	for (uint8_t i = 0; i < 8; ++i) if (changedMask & (1 << i)) { if (_handlers1[i]) _handlers1[i](); }
	_last1 = current;
}

void PcintManager::handleGroup2(uint8_t changedMask, uint8_t current) {
	for (uint8_t i = 0; i < 8; ++i) if (changedMask & (1 << i)) { if (_handlers2[i]) _handlers2[i](); }
	_last2 = current;
}

} // namespace ByByte

ISR(PCINT0_vect) {
	using namespace ByByte;
	uint8_t cur = PINB;
	static uint8_t last = cur;
	uint8_t changed = cur ^ last;
	if (changed) PcintManager::handleGroup0(changed, cur);
	last = cur;
}

ISR(PCINT1_vect) {
	using namespace ByByte;
	uint8_t cur = PINJ;
	static uint8_t last = cur;
	uint8_t changed = cur ^ last;
	if (changed) PcintManager::handleGroup1(changed, cur);
	last = cur;
}

ISR(PCINT2_vect) {
	using namespace ByByte;
	uint8_t cur = PINK;
	static uint8_t last = cur;
	uint8_t changed = cur ^ last;
	if (changed) PcintManager::handleGroup2(changed, cur);
	last = cur;
}

#endif // __AVR_ATmega2560__
