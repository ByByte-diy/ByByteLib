#ifndef BYBYTE_BLUETOOTH_H
#define BYBYTE_BLUETOOTH_H

#include <Arduino.h>
#include "core/ByByteCore.h"

#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
#else
// Pointer member only; full type in Bluetooth.cpp so headers stay parseable without Arduino library paths.
class NeoSWSerial;
#endif

namespace ByByte {

enum class BtModuleType { Unknown, HC02, HC05, HC06, HC08_BLE, HC42_BLE };

/**
 * Platform Bluetooth bridge: Mega uses BYBYTE_BT_UART + optional power pin;
 * other boards use SoftwareSerial on the pins from bb::pins / ByByteConfig.
 */
class Bluetooth {
public:
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	Bluetooth(HardwareSerial& uart = BYBYTE_BT_UART,
	          bb::pins::by_byte_mega::BtMegaBluetoothPins layout = bb::pins::by_byte_mega::defaultBluetoothMega());
#else
	explicit Bluetooth(bb::pins::by_byte_nano::BtSoftwareSerialPins pins =
	                       bb::pins::by_byte_nano::defaultBluetoothSoftwareSerialPins());
#endif

	~Bluetooth();

	Bluetooth(const Bluetooth&) = delete;
	Bluetooth& operator=(const Bluetooth&) = delete;

	// Start UART/SoftwareSerial at desiredBaud, apply power on Mega. Returns immediately; poll isReady().
	bool begin(uint32_t desiredBaud = 9600);

	void startReadinessCheck();
	bool isReady(); // Non-blocking; may ping when checking
	bool isChecking() const { return _isChecking; }

	// Stream-like API over the module link
	int available();
	int read();
	size_t readBytes(uint8_t* buffer, size_t length);
	size_t write(uint8_t b);
	size_t write(const uint8_t* data, size_t length);
	void flush();

	// Mega: toggles Bluetooth supply via power pin. Nano/others: no-op.
	void powerOn();
	void powerOff();

	// Classic module AT commands; return true when response contains expect string / OK
	bool rename(const char* newName);
	bool getName(String& outName);
	bool reset();
	bool setPin(const char* pin4digits);
	bool restoreDefault();

	// BLE modules (best-effort; often same AT+NAME path as classic)
	bool setBleName(const char* newName);
	bool getBleName(String& outName);

	// Send AT and look for OK within timeoutMs
	bool ping(uint16_t timeoutMs = 500);

	BtModuleType moduleType() const { return _type; }
	uint32_t baud() const { return _baud; }

private:
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	HardwareSerial* _serial;
	uint8_t _powerPin;
#else
	bb::pins::by_byte_nano::BtSoftwareSerialPins _pins;
	NeoSWSerial* _uart; // heap-allocated in ctor; see ~Bluetooth() in .cpp
#endif
	uint32_t _baud;
	BtModuleType _type;
	bool _isReady;
	bool _isChecking;
	uint8_t _checkAttempts;
	unsigned long _lastCheckTime;
	/** After one failed ping, try common bauds once (see probeAtAcrossCommonBauds). */
	bool _baudProbeDone;

	void closeUart();
	void openUart(uint32_t baud);

	/** Quick scan of common UART rates if default ping fails (still same line rate for HC-0x data/AT). */
	bool probeAtAcrossCommonBauds(uint16_t replyTimeoutMs);

	bool enterAtMode(uint32_t& detectedBaud);
	bool detectModuleType();
	bool setModuleBaud(uint32_t newBaud);
	bool sendAT(const __FlashStringHelper* cmd, const char* expectOk, String* out = nullptr, uint16_t timeoutMs = 500);
	bool sendAT(const char* cmd, const char* expectOk, String* out = nullptr, uint16_t timeoutMs = 500);
	bool queryFirstMatch(const char* const* cmds, size_t n, const char* expectOk, String* out = nullptr,
	                     uint16_t timeoutMs = 500);
};

} // namespace ByByte

#endif // BYBYTE_BLUETOOTH_H
