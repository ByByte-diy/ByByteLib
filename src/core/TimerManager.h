#ifndef BYBYTE_TIMER_MANAGER_H
#define BYBYTE_TIMER_MANAGER_H

#include <Arduino.h>

namespace ByByte {

// Timer subscription types
enum class TimerInterval {
	MICROSECOND_1 = 0,    // 1μs
	MILLISECOND_1 = 1,    // 1ms
	MILLISECOND_100 = 2,   // 100ms
	SECOND_1 = 3          // 1s
};

// Timer subscription callback with immediate execution flag
struct TimerSubscription {
	void (*handler)();
	TimerInterval interval;
	bool immediately;  // Execute in ISR context
	volatile uint32_t counter;  // Internal counter for intervals > 1ms
};

class TimerManager {
public:
	// Singleton access
	static TimerManager& getInstance();
	
	// Subscribe to timer with specific interval and execution mode
	bool subscribe(TimerInterval interval, void (*handler)(), bool immediately = false);
	void unsubscribe(void (*handler)());
	
	// Called from ISR
	static void on1usTick();
	static void on1msTick();

private:
	TimerManager(); // Private constructor for singleton
	void startTimer();
	void stopTimer();
	
	static TimerManager* _instance;
	TimerSubscription _subscriptions[12];
	volatile uint8_t _subscriptionCount;
	volatile bool _running;
	volatile bool _has1usSubs;
	volatile bool _has1msSubs;
};

} // namespace ByByte

#endif // BYBYTE_TIMER_MANAGER_H
