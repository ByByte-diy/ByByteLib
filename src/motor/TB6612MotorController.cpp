#include "TB6612MotorController.h"
#include "MotorUtils.h"

namespace ByByte {

TB6612MotorController::TB6612MotorController(uint8_t stby,
                                             uint8_t aIn1, uint8_t aIn2, uint8_t aPwm,
                                             uint8_t bIn1, uint8_t bIn2, uint8_t bPwm)
	: _stby(stby),
	  _aIn1(aIn1), _aIn2(aIn2), _aPwm(aPwm),
	  _bIn1(bIn1), _bIn2(bIn2), _bPwm(bPwm) {}

bool TB6612MotorController::begin() {
	pinMode(_stby, OUTPUT);
	pinMode(_aIn1, OUTPUT);
	pinMode(_aIn2, OUTPUT);
	pinMode(_aPwm, OUTPUT);
	pinMode(_bIn1, OUTPUT);
	pinMode(_bIn2, OUTPUT);
	pinMode(_bPwm, OUTPUT);
	digitalWrite(_stby, HIGH);
	return true;
}

void TB6612MotorController::driveChannel(uint8_t in1, uint8_t in2, uint8_t pwmPin, int16_t value) {
	writeTb6612Channel(in1, in2, pwmPin, value);
}

void TB6612MotorController::setMotorSpeeds(int16_t left, int16_t right) {
	driveChannel(_aIn1, _aIn2, _aPwm, left);
	driveChannel(_bIn1, _bIn2, _bPwm, right);
}

} // namespace ByByte
