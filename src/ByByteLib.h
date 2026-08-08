#ifndef BYBYTE_LIB_H
#define BYBYTE_LIB_H

#include <Arduino.h>
#include "core/Types.h"
#include "core/configs/ByByteConfig.h"
#include "core/configs/PlatformDetect.h"
#include "core/configs/ByByteProduct.h"
#include "core/PcintManager.h"
#include "MotorDriver.h"
#include "ByByteKits.h"
#include "SideIrSensors.h"
#include "Servo.h"
#include "ServoManager.h"
#include "BatterySensor.h"
#include "Lm35Sensor.h"
#include "LdrSensor.h"
#include "Bluetooth.h"
#include "Buzzer.h"
#include "Sonar.h"

namespace ByByte {

	class ByByteLib {
	public:
		static const char* version();
	};

} // namespace ByByte

#endif // BYBYTE_LIB_H
