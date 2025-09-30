#ifndef BYBYTE_LIB_H
#define BYBYTE_LIB_H

#include <Arduino.h>
#include "Types.h"
#include "ByByteConfig.h"
#include "MotorController.h"
#include "DifferentialDriveController.h"
#include "PlatformDetect.h"
#include "PcintManager.h"
#include "MotorDriver.h"
#include "SideIrSensors.h"
#include "IrReceiver.h"
#include "Servo.h"
#include "ServoManager.h"
#include "BatterySensor.h"
#include "Lm35Sensor.h"
#include "LdrSensor.h"
#include "Bluetooth.h"
#include "Buzzer.h"
#include "Sonar.h"
#include "SonarPrecise.h"

namespace ByByte {

class ByByteLib {
public:
	static const char* version();
};

} // namespace ByByte

#endif // BYBYTE_LIB_H
