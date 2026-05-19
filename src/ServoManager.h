#ifndef BYBYTE_SERVO_MANAGER_H
#define BYBYTE_SERVO_MANAGER_H

#include <Arduino.h>
#include "configs/PlatformDetect.h"

#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA

namespace ByByte {

class ServoManager {
public:
	// Up to 3 channels
	static void begin(uint8_t frameHz = 50); // 50 or 25
	static bool attach(uint8_t channel, uint8_t pin, uint16_t minUs = 1000, uint16_t maxUs = 2000);
	static void detach(uint8_t channel);
	static void writeMicros(uint8_t channel, uint16_t micros);
	static void writeAngle(uint8_t channel, uint8_t degrees);
	static void setFrameRate(uint8_t frameHz); // 50 (20ms) or 25 (40ms)

	// Called from ISR
	static void isrTick();

private:
	struct Channel {
		uint8_t pin;
		volatile uint8_t* port;
		uint8_t bitMask;
		uint16_t pulseTicks;
		uint16_t minUs;
		uint16_t maxUs;
		bool enabled;
	};

	static void ensureStarted();
	static void setupTimer3();
	static void recomputeFrame();

	static Channel _ch[3];
	static volatile uint16_t _frameTicks;
	static volatile uint16_t _tick;
	static uint16_t _tickUs;
	static uint8_t _frameHz;
	static bool _running;
};

} // namespace ByByte

#endif // MEGA

#endif // BYBYTE_SERVO_MANAGER_H
