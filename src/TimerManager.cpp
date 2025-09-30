#include "TimerManager.h"

namespace ByByte {

// Static member definitions
TimerManager* TimerManager::_instance = nullptr;

TimerManager::TimerManager() {
	// Initialize all subscriptions
	for (uint8_t i = 0; i < 12; i++) {
		_subscriptions[i].handler = nullptr;
		_subscriptions[i].interval = TimerInterval::MILLISECOND_1;
		_subscriptions[i].immediately = false;
		_subscriptions[i].counter = 0;
	}
	_subscriptionCount = 0;
	_running = false;
	_has1usSubs = false;
	_has1msSubs = false;
	
	// Configure Timer4 registers in constructor
	#if defined(__AVR_ATmega2560__)
		// Mega: Timer4 CTC mode with prescaler 8 (2MHz)
		TCCR4A = 0;                    // CTC mode
		TCCR4B = (1<<WGM42) | (1<<CS41); // CTC mode, prescaler 8 (2MHz)
		TCNT4 = 0;                     // Start from 0
		
		// Set OCR4A for 1ms compare interrupt
		OCR4A = 1999;              // 2MHz/1000 = 2000 ticks = 1ms
		TIFR4 = (1<<OCF4A);        // Clear flag
		
		// Set OCR4B for 1μs compare interrupt  
		OCR4B = 1;                 // 2MHz/2 = 1MHz = 1μs
		TIFR4 = (1<<OCF4B);        // Clear flag
		
		Serial.println(F("TimerManager: Timer4 configured in constructor"));
	#else
		// Nano: Timer1 CTC mode with prescaler 1 (16MHz)
		TCCR1A = 0;                    // CTC mode
		TCCR1B = (1<<WGM12) | (1<<CS10); // CTC mode, prescaler 1 (16MHz)
		TCNT1 = 0;                     // Start from 0
		
		// Set OCR1A for 1ms compare interrupt
		OCR1A = 15999;             // 16MHz/1000 = 16000 ticks = 1ms
		TIFR1 = (1<<OCF1A);        // Clear flag
		
		// Set OCR1B for 1μs compare interrupt
		OCR1B = 15;                // 16MHz/16 = 1MHz = 1μs
		TIFR1 = (1<<OCF1B);        // Clear flag
		
		Serial.println(F("TimerManager: Timer1 configured in constructor"));
	#endif
}

TimerManager& TimerManager::getInstance() {
	if (_instance == nullptr) {
		_instance = new TimerManager();
	}
	return *_instance;
}

bool TimerManager::subscribe(TimerInterval interval, void (*handler)(), bool immediately) {
	if (_subscriptionCount >= 12) {
		Serial.println(F("TimerManager: Too many subscriptions"));
		return false;
	}
	
	// Find empty slot
	for (uint8_t i = 0; i < 12; i++) {
		if (_subscriptions[i].handler == nullptr) {
			_subscriptions[i].handler = handler;
			_subscriptions[i].interval = interval;
			_subscriptions[i].immediately = immediately;
			_subscriptions[i].counter = 0;
			_subscriptionCount++;
			
			Serial.print(F("TimerManager: Subscribed handler "));
			Serial.print(i);
			Serial.print(F(" to interval "));
			Serial.print((int)interval);
			Serial.print(F(", immediately: "));
			Serial.println(immediately);
			
			// Start timer on first subscription
			if (_subscriptionCount == 1) {
				startTimer();
			}
			
			// Update subscription flags
			if (interval == TimerInterval::MICROSECOND_1) {
				_has1usSubs = true;
				Serial.println(F("TimerManager: Set _has1usSubs = true"));
			} else if (interval == TimerInterval::MILLISECOND_1) {
				_has1msSubs = true;
				Serial.println(F("TimerManager: Set _has1msSubs = true"));
			}
			
			// If timer is already running and we added a new type, restart it
			if (_running && _subscriptionCount > 1) {
				Serial.println(F("TimerManager: Restarting timer for new interrupt type"));
				stopTimer();
				startTimer();
			}
			return true;
		}
	}
	
	Serial.println(F("TimerManager: No free slots"));
	return false;
}

void TimerManager::unsubscribe(void (*handler)()) {
	for (uint8_t i = 0; i < 12; i++) {
		if (_subscriptions[i].handler == handler) {
			TimerInterval oldInterval = _subscriptions[i].interval;
			_subscriptions[i].handler = nullptr;
			_subscriptions[i].interval = TimerInterval::MILLISECOND_1;
			_subscriptions[i].immediately = false;
			_subscriptions[i].counter = 0;
			_subscriptionCount--;
			
			Serial.print(F("TimerManager: Unsubscribed handler "));
			Serial.println(i);
			
			// Update subscription flags
			if (oldInterval == TimerInterval::MICROSECOND_1) {
				_has1usSubs = false;
			} else if (oldInterval == TimerInterval::MILLISECOND_1) {
				_has1msSubs = false;
			}
			
			// Stop timer on last subscription
			if (_subscriptionCount == 0) {
				stopTimer();
			}
			break;
		}
	}
}

void TimerManager::startTimer() {
	if (_running) {
		Serial.println(F("TimerManager: Already running"));
		return;
	}
	
	Serial.println(F("TimerManager: Starting timer"));
	Serial.print(F("TimerManager: _has1msSubs = "));
	Serial.print(_has1msSubs);
	Serial.print(F(", _has1usSubs = "));
	Serial.println(_has1usSubs);
	
	#if defined(__AVR_ATmega2560__)
		// Mega: Enable Timer4 interrupts based on subscriptions
		if (_has1msSubs) {
			TIMSK4 |= (1<<OCIE4A);     // Enable 1ms interrupt
			Serial.println(F("Timer4: 1ms interrupt enabled"));
		}
		if (_has1usSubs) {
			TIMSK4 |= (1<<OCIE4B);     // Enable 1μs interrupt
			Serial.println(F("Timer4: 1μs interrupt enabled"));
		}
	#else
		// Nano: Enable Timer1 interrupts based on subscriptions
		if (_has1msSubs) {
			TIMSK1 |= (1<<OCIE1A);     // Enable 1ms interrupt
			Serial.println(F("Timer1: 1ms interrupt enabled"));
		}
		if (_has1usSubs) {
			TIMSK1 |= (1<<OCIE1B);     // Enable 1μs interrupt
			Serial.println(F("Timer1: 1μs interrupt enabled"));
		}
	#endif
	
	_running = true;
}

void TimerManager::stopTimer() {
	if (!_running) {
		Serial.println(F("TimerManager: Already stopped"));
		return;
	}
	
	Serial.println(F("TimerManager: Stopping timer"));
	
	#if defined(__AVR_ATmega2560__)
		// Mega: Disable Timer4 interrupts
		TIMSK4 &= ~(1<<OCIE4A);    // Disable 1ms interrupt
		TIMSK4 &= ~(1<<OCIE4B);    // Disable 1μs interrupt
		Serial.println(F("Timer4: Interrupts disabled"));
	#else
		// Nano: Disable Timer1 interrupts
		TIMSK1 &= ~(1<<OCIE1A);    // Disable 1ms interrupt
		TIMSK1 &= ~(1<<OCIE1B);    // Disable 1μs interrupt
		Serial.println(F("Timer1: Interrupts disabled"));
	#endif
	
	_running = false;
}

void TimerManager::on1usTick() {
	static uint32_t tickCount = 0;
	tickCount++;

	// Debug every 1000 ticks (1ms)
	if (tickCount % 1000 == 0) {
		Serial.print(F("TimerManager: 1μs tick #"));
		Serial.println(tickCount);
	}

	// Process 1μs subscriptions immediately in ISR
	if (_instance) {
		for (uint8_t i = 0; i < 12; i++) {
			if (_instance->_subscriptions[i].handler != nullptr && 
				_instance->_subscriptions[i].interval == TimerInterval::MICROSECOND_1) {
				if (_instance->_subscriptions[i].immediately) {
					_instance->_subscriptions[i].handler();
				}
			}
		}
	}
}

void TimerManager::on1msTick() {
	static uint32_t tickCount = 0;
	tickCount++;

	// Debug every 1000 ticks (1s)
	if (tickCount % 1000 == 0) {
		Serial.print(F("TimerManager: 1ms tick #"));
		Serial.println(tickCount);
	}

	// Process 1ms subscriptions (always call, not just immediately)
	if (_instance) {
		for (uint8_t i = 0; i < 12; i++) {
			if (_instance->_subscriptions[i].handler != nullptr && 
				_instance->_subscriptions[i].interval == TimerInterval::MILLISECOND_1) {
				_instance->_subscriptions[i].handler();
			}
		}
	}
}

} // namespace ByByte

#if defined(__AVR__)
	#if defined(__AVR_ATmega2560__)
		// Mega: Timer4 dual compare interrupts
		ISR(TIMER4_COMPB_vect) { ByByte::TimerManager::on1usTick(); }   // OCR4B: 1μs
		ISR(TIMER4_COMPA_vect) { ByByte::TimerManager::on1msTick(); }   // OCR4A: 1ms
	#else
		// Nano: Timer1 dual compare interrupts
		ISR(TIMER1_COMPB_vect) { ByByte::TimerManager::on1usTick(); }   // OCR1B: 1μs
		ISR(TIMER1_COMPA_vect) { ByByte::TimerManager::on1msTick(); }   // OCR1A: 1ms
	#endif
#endif