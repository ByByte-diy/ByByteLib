#ifndef BYBYTE_KITS_H
#define BYBYTE_KITS_H

#include "core/configs/ByByteProduct.h"
#include "Bluetooth.h"
#include "MotorDriver.h"

namespace ByByte {

/** ByByte Nano kit — DRV8833 motor driver and HC-0x on SoftwareSerial defaults (when MCU is Nano). */
class ByByteNano final : public ByByteKit {
public:
	MotorDriver motors;
	Bluetooth bluetooth;

	// Driver/pins are auto-selected by PlatformDetect for the compiled board.
	ByByteNano() = default;

	PlatformKit kit() const noexcept override {
		return PlatformKit::Nano;
	}

	const char* name() const noexcept override {
		return "ByByteNano";
	}

	bool beginMotors() {
		return motors.begin();
	}

	/** Start UART on default BT pins from config; poll `bluetooth.isReady()` if needed. */
	bool beginBluetooth(uint32_t baud = 9600) {
		return bluetooth.begin(baud);
	}
};

/** ByByte Mega kit — TB6612 driver and BT on Serial1 + power pin defaults (when MCU is Mega). */
class ByByteMega final : public ByByteKit {
public:
	MotorDriver motors;
	Bluetooth bluetooth;

	// Driver/pins are auto-selected by PlatformDetect for the compiled board.
	ByByteMega() = default;

	PlatformKit kit() const noexcept override {
		return PlatformKit::Mega;
	}

	const char* name() const noexcept override {
		return "ByByteMega";
	}

	bool beginMotors() {
		return motors.begin();
	}

	bool beginBluetooth(uint32_t baud = 9600) {
		return bluetooth.begin(baud);
	}
};

/**
 * NanoBoy console variant — peripheral layout is host/sim-specific; onboard MotorDriver omitted.
 * Extend this class once console/sim I/O bridges are modeled.
 */
class ByByteNanoBoy final : public ByByteKit {
public:
	ByByteNanoBoy() = default;

	PlatformKit kit() const noexcept override {
		return PlatformKit::NanoBoy;
	}

	const char* name() const noexcept override {
		return "ByByteNanoBoy";
	}
};

} // namespace ByByte

#endif // BYBYTE_KITS_H
