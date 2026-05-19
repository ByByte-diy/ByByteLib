#ifndef BYBYTE_IR_RECEIVER_H
#define BYBYTE_IR_RECEIVER_H

#include <Arduino.h>
#include "configs/ByByteConfig.h"
#include "PcintManager.h"

namespace ByByte {

enum class IrProtocol { Auto, NEC, RC5, RC6 };

struct IrFrame {
	uint16_t address;
	uint16_t command;
	bool repeat;
	IrProtocol proto;
};

class IrReceiver {
public:
	IrReceiver(IrProtocol proto = IrProtocol::Auto);
	bool begin(uint8_t rxPin = BYBYTE_IR_RX_PIN);
	bool available() const;
	IrFrame read();

private:
	static void onEdge();
	static IrReceiver* _self;
	uint8_t _rxPin;
	volatile bool _hasFrame;
	volatile IrFrame _frame;
	IrProtocol _proto;
	// Common state
	volatile uint32_t _lastMicros;
	// NEC state
	volatile bool _necInFrame;
	volatile uint8_t _necBitIndex;
	volatile uint32_t _necRaw;
	// RC5/RC6 state (Manchester)
	volatile bool _manInFrame;
	volatile uint8_t _manBitIndex;
	volatile uint32_t _manRaw;
	volatile bool _lastLevel;

	bool attachPcintForPin(uint8_t pin);
	void handleEdge();
	void handleNec(uint32_t dur);
	void handleRc5(uint32_t dur, bool level);
	void handleRc6(uint32_t dur, bool level);
};

} // namespace ByByte

#endif // BYBYTE_IR_RECEIVER_H
