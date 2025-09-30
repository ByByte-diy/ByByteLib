#include "SonarPrecise.h"

namespace ByByte {

SonarPrecise* SonarPrecise::_sonarInstances[4] = {nullptr, nullptr, nullptr, nullptr};
uint8_t SonarPrecise::_instanceCount = 0;

SonarPrecise::SonarPrecise(uint8_t trigPin, uint8_t echoPin, uint16_t maxDistanceCm)
	: _trigPin(trigPin), _echoPin(echoPin), _maxDistanceCm(maxDistanceCm),
	  _maxTimeoutUs(maxDistanceCm * 58), _state(IDLE), _lastTriggerUs(0),
	  _echoRiseUs(0), _lastCm(0), _readingIndex(0), _readingCount(0) {
	// Initialize filter array
	for (uint8_t i = 0; i < 5; i++) {
		_readings[i] = 0;
	}
}

bool SonarPrecise::begin() {
	if (_instanceCount >= 4) {
		return false;
	}
	
	// Configure pins
	pinMode(_trigPin, OUTPUT);
	pinMode(_echoPin, INPUT);
	digitalWrite(_trigPin, LOW);
	
	// Register instance
	for (uint8_t i = 0; i < 4; i++) {
		if (_sonarInstances[i] == nullptr) {
			_sonarInstances[i] = this;
			_instanceCount++;
			break;
		}
	}
	
	// Subscribe to timers
	bool usSub = TimerManager::getInstance().subscribe(TimerInterval::MICROSECOND_1, &timer1usHandler, true);
	bool msSub = TimerManager::getInstance().subscribe(TimerInterval::MILLISECOND_1, &timer1msHandler, false);
	
	return usSub && msSub;
}

void SonarPrecise::end() {
	// Unregister instance
	for (uint8_t i = 0; i < 4; i++) {
		if (_sonarInstances[i] == this) {
			_sonarInstances[i] = nullptr;
			_instanceCount--;
			break;
		}
	}
	
	// Unsubscribe from timers
	TimerManager::getInstance().unsubscribe(&timer1usHandler);
	TimerManager::getInstance().unsubscribe(&timer1msHandler);
}

uint16_t SonarPrecise::readCm() {
	return _lastCm;
}

void SonarPrecise::on1usTick() {
	static uint32_t tickCount = 0;
	tickCount++;
	
	// Process all instances for 1μs precision
	for (uint8_t i = 0; i < 4; i++) {
		if (_sonarInstances[i] != nullptr) {
			SonarPrecise* sonar = _sonarInstances[i];
			
			// Handle echo edge detection with microsecond precision
			uint8_t echoLevel = digitalRead(sonar->_echoPin);
			uint32_t nowUs = micros();
			
			if (sonar->_state == WAITING_ECHO && echoLevel == HIGH) {
				sonar->handleEchoRise();
			} else if (sonar->_state == MEASURING && echoLevel == LOW) {
				sonar->handleEchoFall();
			}
			
			// Check for timeout based on max distance
			if (sonar->_state == WAITING_ECHO || sonar->_state == MEASURING) {
				if ((nowUs - sonar->_lastTriggerUs) > sonar->_maxTimeoutUs) {
					sonar->_state = IDLE;
					sonar->_lastCm = 0;
				}
			}
		}
	}
	
}

void SonarPrecise::on1msTick() {
	// Process all instances for 1ms trigger (every 100ms)
	static uint32_t lastTriggerMs = 0;
	static uint32_t tickCount = 0;
	uint32_t nowMs = millis();
	
	tickCount++;
	
	if (nowMs - lastTriggerMs >= 50) { // Trigger every 50ms
		lastTriggerMs = nowMs;
		
		for (uint8_t i = 0; i < 4; i++) {
			if (_sonarInstances[i] != nullptr) {
				SonarPrecise* sonar = _sonarInstances[i];
				
				if (sonar->_state == IDLE) {
					sonar->trigger();
				}
			}
		}
	}
}

void SonarPrecise::timer1usHandler() {
	on1usTick();
}

void SonarPrecise::timer1msHandler() {
	on1msTick();
}

void SonarPrecise::trigger() {
	// NewPing approach: Send 10μs trigger pulse with precise timing
	digitalWrite(_trigPin, LOW);
	delayMicroseconds(2);  // Ensure clean start
	
	digitalWrite(_trigPin, HIGH);
	delayMicroseconds(10); // 10μs trigger pulse
	digitalWrite(_trigPin, LOW);
	
	_lastTriggerUs = micros();
	_state = WAITING_ECHO;
}

void SonarPrecise::handleEchoRise() {
	_echoRiseUs = micros();
	_state = MEASURING;
}

void SonarPrecise::handleEchoFall() {
	uint32_t echoTimeUs = micros() - _echoRiseUs;
	uint16_t rawDistance = calculateDistance(echoTimeUs);
	
	// Add to filter if valid reading
	if (rawDistance > 0) {
		_readings[_readingIndex] = rawDistance;
		_readingIndex = (_readingIndex + 1) % 5;
		if (_readingCount < 5) _readingCount++;
		
		// Calculate filtered distance (median of last 3 readings)
		uint16_t filteredDistance = getFilteredDistance();
		_lastCm = filteredDistance;
	}
	
	_state = IDLE;
}

uint16_t SonarPrecise::calculateDistance(uint32_t echoTimeUs) {
	// NewPing formula: Distance = (echo time in μs) / 58
	// This gives distance in cm with better precision
	uint32_t distanceCm = echoTimeUs / 58;
	
	// Check if distance is within range (NewPing approach)
	// Minimum distance 2cm to filter noise
	if (distanceCm < 2 || distanceCm > _maxDistanceCm) {
		return 0; // Out of range or too close (noise)
	}
	
	return (uint16_t)distanceCm;
}

uint16_t SonarPrecise::getFilteredDistance() {
	if (_readingCount == 0) return 0;
	
	// Simple median filter of last 3 readings
	uint16_t temp[3];
	uint8_t count = min(_readingCount, (uint8_t)3);
	
	// Copy last readings
	for (uint8_t i = 0; i < count; i++) {
		uint8_t idx = (_readingIndex - 1 - i + 5) % 5;
		temp[i] = _readings[idx];
	}
	
	// Sort (bubble sort for small array)
	for (uint8_t i = 0; i < count - 1; i++) {
		for (uint8_t j = 0; j < count - 1 - i; j++) {
			if (temp[j] > temp[j + 1]) {
				uint16_t swap = temp[j];
				temp[j] = temp[j + 1];
				temp[j + 1] = swap;
			}
		}
	}
	
	// Return median
	return temp[count / 2];
}

SonarPrecise* SonarPrecise::create(uint8_t trigPin, uint8_t echoPin, uint16_t maxDistanceCm) {
	return new SonarPrecise(trigPin, echoPin, maxDistanceCm);
}

} // namespace ByByte
