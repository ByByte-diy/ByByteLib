#ifndef BYBYTE_TB6612_MOTOR_CONTROLLER_H
#define BYBYTE_TB6612_MOTOR_CONTROLLER_H

#include <Arduino.h>
#include "Types.h"
#include "MotorController.h"

namespace ByByte {

class TB6612MotorController : public MotorController {
public:
	TB6612MotorController(uint8_t stby,
						 uint8_t aIn1, uint8_t aIn2, uint8_t aPwm,
						 uint8_t bIn1, uint8_t bIn2, uint8_t bPwm);
	bool begin() override;
	void setMotorSpeeds(int16_t left, int16_t right) override;
private:
	void driveChannel(uint8_t in1, uint8_t in2, uint8_t pwmPin, int16_t value);
	uint8_t _stby;
	uint8_t _aIn1; uint8_t _aIn2; uint8_t _aPwm;
	uint8_t _bIn1; uint8_t _bIn2; uint8_t _bPwm;
};

} // namespace ByByte

#endif // BYBYTE_TB6612_MOTOR_CONTROLLER_H
