#ifndef BYBYTE_SERVO_H
#define BYBYTE_SERVO_H

#include <Arduino.h>
#include "configs/PlatformDetect.h"

namespace ByByte {

class Servo {
public:
	Servo();
	bool attach(uint8_t pin, uint16_t minUs = 500, uint16_t maxUs = 2500, uint8_t frameHz = 50);
	void detach();
	void write(uint8_t angle);
	void writeMicroseconds(uint16_t us);
	bool attached() const;

	// Continuous (360°) mode support
	void setContinuous(bool enable, uint16_t stopUs = 1500, uint16_t deadbandUs = 20);
	void writeSpeed(int8_t speedPercent); // -100..100

private:
	int8_t _channel;
	uint8_t _pin;
	bool _continuous;
	uint16_t _stopUs;
	uint16_t _deadbandUs;
	uint16_t _minUs;
	uint16_t _maxUs;
};

} // namespace ByByte

#endif // BYBYTE_SERVO_H
