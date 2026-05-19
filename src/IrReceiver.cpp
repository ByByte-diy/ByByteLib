#include "IrReceiver.h"

namespace ByByte {

IrReceiver* IrReceiver::_self = nullptr;

IrReceiver::IrReceiver(IrProtocol proto)
	: _rxPin(BYBYTE_IR_RX_PIN), _hasFrame(false), _proto(proto),
	  _lastMicros(0), _necInFrame(false), _necBitIndex(0), _necRaw(0),
	  _manInFrame(false), _manBitIndex(0), _manRaw(0), _lastLevel(true) {
	_frame.address = 0; _frame.command = 0; _frame.repeat = false; _frame.proto = IrProtocol::Auto;
}

static uint8_t pinToPcint(uint8_t pin) {
	#if defined(__AVR_ATmega2560__)
		if (pin == 14) return 10; // PJ1 -> PCINT10
	#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
		if (pin == 8) return 0; // D8 -> PCINT0
	#endif
	return 0xFF;
}

bool IrReceiver::attachPcintForPin(uint8_t pin) {
	uint8_t pcint = pinToPcint(pin);
	if (pcint == 0xFF) return false;
	return PcintManager::subscribe(pcint, &IrReceiver::onEdge, true);
}

bool IrReceiver::begin(uint8_t rxPin) {
	_rxPin = rxPin;
	_self = this;
	pinMode(_rxPin, INPUT_PULLUP);
	_lastLevel = (digitalRead(_rxPin) != 0);
	return attachPcintForPin(_rxPin);
}

bool IrReceiver::available() const { return _hasFrame; }

IrFrame IrReceiver::read() {
	IrFrame out;
	out.address = _frame.address;
	out.command = _frame.command;
	out.repeat = _frame.repeat;
	out.proto = _frame.proto;
	_hasFrame = false;
	return out;
}

void IrReceiver::onEdge() {
	if (_self) _self->handleEdge();
}

void IrReceiver::handleEdge() {
	uint32_t now = micros();
	uint32_t dur = now - _lastMicros;
	_lastMicros = now;
	bool level = (digitalRead(_rxPin) != 0);

	if (_proto == IrProtocol::NEC || _proto == IrProtocol::Auto) handleNec(dur);
	if (_hasFrame) return;
	if (_proto == IrProtocol::RC5 || _proto == IrProtocol::Auto) handleRc5(dur, level);
	if (_hasFrame) return;
	if (_proto == IrProtocol::RC6 || _proto == IrProtocol::Auto) handleRc6(dur, level);
}

void IrReceiver::handleNec(uint32_t dur) {
	const uint32_t T_PULSE = 560;
	const uint32_t T_ONE_SPACE = 1690;
	const uint32_t T_ZERO_SPACE = 560;
	const uint32_t T_LEAD_PULSE = 9000;
	const uint32_t T_LEAD_SPACE = 4500;
	const uint32_t T_REPEAT_SPACE = 2250;
	const uint32_t T_TOL = 300;

	if (!_necInFrame) {
		if (dur > (T_LEAD_PULSE - 2000) && dur < (T_LEAD_PULSE + 2000)) {
			_necInFrame = true; _necBitIndex = 0; _necRaw = 0; return;
		}
	} else {
		if ((dur > (T_LEAD_SPACE - 1000)) && (dur < (T_LEAD_SPACE + 1000)) && _necBitIndex == 0) return;
		if (dur > (T_ZERO_SPACE - T_TOL) && dur < (T_ZERO_SPACE + T_TOL)) {
			_necRaw <<= 1; _necBitIndex++;
		} else if (dur > (T_ONE_SPACE - 500) && dur < (T_ONE_SPACE + 500)) {
			_necRaw = (_necRaw << 1) | 1U; _necBitIndex++;
		} else if (dur > (T_REPEAT_SPACE - 500) && dur < (T_REPEAT_SPACE + 500) && _necBitIndex == 0) {
			_frame.repeat = true; _frame.proto = IrProtocol::NEC; _hasFrame = true; _necInFrame=false; return;
		} else {
			_necInFrame=false; _necBitIndex=0; _necRaw=0; return;
		}
		if (_necBitIndex >= 32) {
			uint32_t d = _necRaw;
			uint8_t addr = (d >> 24) & 0xFF;
			uint8_t naddr = (d >> 16) & 0xFF;
			uint8_t cmd = (d >> 8) & 0xFF;
			uint8_t ncmd = d & 0xFF;
			if ((uint8_t)~addr == naddr && (uint8_t)~cmd == ncmd) {
				_frame.address = addr; _frame.command = cmd; _frame.repeat = false; _frame.proto = IrProtocol::NEC; _hasFrame=true;
			}
			_necInFrame=false; _necBitIndex=0; _necRaw=0;
		}
	}
}

// RC-5: Manchester-coded, bit time ~1778us, half-bit ~889us, 14 bits starting with two start bits '1'
void IrReceiver::handleRc5(uint32_t dur, bool level) {
	const uint32_t HALF = 889; const uint32_t TOL = 400; // tolerance band
	// Track edges and accumulate half-bit periods
	static uint8_t halfs = 0;
	static uint8_t curBit = 1; // RC5 starts at '1'
	static bool last = _lastLevel;
	_lastLevel = level;
	if (dur < (HALF - TOL) || dur > (HALF + TOL)) return; // not a half-bit
	// Alternate levels each half-bit; latch bits every other half-bit
	halfs++;
	if (halfs & 1) {
		// First half of the bit interval: bit value from transition direction
		curBit = last ? 0 : 1; // high->low = 0, low->high = 1
	} else {
		// Second half: commit bit into shift register
		if (!_manInFrame) { _manInFrame = true; _manBitIndex = 0; _manRaw = 0; }
		_manRaw = (_manRaw << 1) | (curBit & 1);
		_manBitIndex++;
		if (_manBitIndex >= 14) {
			// RC5: S1,S2,T, addr5, cmd6
			uint16_t raw = (uint16_t)(_manRaw & 0x3FFF);
			uint8_t toggle = (raw >> 11) & 0x01;
			uint8_t addr = (raw >> 6) & 0x1F;
			uint8_t cmd = raw & 0x3F;
			_frame.address = addr; _frame.command = cmd; _frame.repeat = (toggle != 0); _frame.proto = IrProtocol::RC5; _hasFrame = true;
			_manInFrame=false; _manBitIndex=0; _manRaw=0; halfs=0;
		}
	}
}

// RC-6 (Mode 0): Manchester, leader 2.667ms, ~20 bits; simplified decode akin to RC-5 path
void IrReceiver::handleRc6(uint32_t dur, bool level) {
	const uint32_t HALF = 444; const uint32_t TOL = 250; // quarter of 1.778ms period
	static uint8_t halfs = 0; static uint8_t curBit = 1; static bool last = _lastLevel;
	_lastLevel = level;
	if (dur < (HALF - TOL) || dur > (HALF + TOL)) return;
	halfs++;
	if (halfs & 1) { curBit = last ? 0 : 1; }
	else {
		if (!_manInFrame) { _manInFrame=true; _manBitIndex=0; _manRaw=0; }
		_manRaw = (_manRaw << 1) | (curBit & 1);
		_manBitIndex++;
		if (_manBitIndex >= 20) {
			uint32_t d = _manRaw & 0xFFFFF;
			uint8_t addr = (d >> 12) & 0xFF; // approximate
			uint8_t cmd = d & 0x7F;
			_frame.address = addr; _frame.command = cmd; _frame.repeat = false; _frame.proto = IrProtocol::RC6; _hasFrame = true;
			_manInFrame=false; _manBitIndex=0; _manRaw=0; halfs=0;
		}
	}
}

} // namespace ByByte
