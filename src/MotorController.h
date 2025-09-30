#ifndef BYBYTE_MOTOR_CONTROLLER_H
#define BYBYTE_MOTOR_CONTROLLER_H

#include <Arduino.h>
#include "Types.h"

namespace ByByte {

class MotorController {
public:
	virtual ~MotorController() {}
	virtual bool begin() = 0;
	virtual void setMotorSpeeds(int16_t left, int16_t right) = 0;
};

} // namespace ByByte

#endif // BYBYTE_MOTOR_CONTROLLER_H

