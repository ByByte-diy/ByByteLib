#ifndef BYBYTE_SONAR_PRECISE_H
#define BYBYTE_SONAR_PRECISE_H

#include <Arduino.h>
#include "TimerManager.h"
#include "configs/PlatformDetect.h"

namespace ByByte {

class SonarPrecise {
public:
	SonarPrecise(uint8_t trigPin, uint8_t echoPin, uint16_t maxDistanceCm = 400);
	
	bool begin();
	void end();
	
	// Non-blocking distance reading
	uint16_t readCm();
	
	// Diagnostic methods
	uint8_t getState() const { return (uint8_t)_state; }
	uint32_t getLastTriggerUs() const { return _lastTriggerUs; }
	
	// Static callbacks for TimerManager
	static void on1usTick();
	static void on1msTick();
	
	// Filtering methods
	uint16_t getFilteredDistance();

private:
	enum State {
		IDLE,
		TRIGGERING,
		WAITING_ECHO,
		MEASURING
	};
	
	uint8_t _trigPin;
	uint8_t _echoPin;
	uint16_t _maxDistanceCm;
	uint32_t _maxTimeoutUs;
	
	volatile State _state;
	volatile uint32_t _lastTriggerUs;
	volatile uint32_t _echoRiseUs;
	volatile uint16_t _lastCm;
	
	// Filtering for stability
	uint16_t _readings[5];
	uint8_t _readingIndex;
	uint8_t _readingCount;
	
	// Static management for multiple instances
	static SonarPrecise* _sonarInstances[4];
	static uint8_t _instanceCount;
	
	// Timer subscription handlers
	static void timer1usHandler();
	static void timer1msHandler();
	
	void trigger();
	void handleEchoRise();
	void handleEchoFall();
	uint16_t calculateDistance(uint32_t echoTimeUs);
	
public:
	// Factory method
	static SonarPrecise* create(uint8_t trigPin, uint8_t echoPin, uint16_t maxDistanceCm);
};

} // namespace ByByte

#endif // BYBYTE_SONAR_PRECISE_H
