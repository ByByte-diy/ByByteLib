#include "DRV8833MotorController.h"
#include "MotorUtils.h"

namespace ByByte {

DRV8833MotorController::DRV8833MotorController(uint8_t leftIn1, uint8_t leftIn2,
                                               uint8_t rightIn1, uint8_t rightIn2)
	: _lIn1(leftIn1), _lIn2(leftIn2), _rIn1(rightIn1), _rIn2(rightIn2) {}

bool DRV8833MotorController::begin() {
	pinMode(_lIn1, OUTPUT);
	pinMode(_lIn2, OUTPUT);
	pinMode(_rIn1, OUTPUT);
	pinMode(_rIn2, OUTPUT);
	return true;
}

void DRV8833MotorController::driveHBridgePwm(uint8_t in1, uint8_t in2, int16_t value) {
	writeHBridgePwm(in1, in2, value);
}

void DRV8833MotorController::setMotorSpeeds(int16_t left, int16_t right) {
	driveHBridgePwm(_lIn1, _lIn2, left);
	driveHBridgePwm(_rIn1, _rIn2, right);
}

} // namespace ByByte
