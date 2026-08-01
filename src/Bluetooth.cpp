// Bluetooth.h keeps a pointer + forward declare of `class SoftwareSerial;` so headers stay
// parseable without Arduino library paths. The full SoftwareSerial type is only needed on
// non-Mega targets (the Mega uses HardwareSerial / Serial1).
//
// CRITICAL on the Mega: <SoftwareSerial.h> must NOT be included in this TU. SoftwareSerial.cpp
// defines ISR(PCINT0_vect) / ISR(PCINT1_vect) / ISR(PCINT2_vect), which are __vector_9/10/11 on
// ATmega2560. ByByteLib's PcintManager.cpp also defines those three ISRs on the Mega (it owns
// Pin Change Interrupts there). Including <SoftwareSerial.h> here makes the Arduino builder link
// SoftwareSerial.cpp.o into the Mega binary, causing "multiple definition of __vector_9/10/11".
// The Mega branch never instantiates SoftwareSerial, so excluding the header resolves the clash.
#include <Arduino.h>
#include "Bluetooth.h"

#if BYBYTE_PLATFORM_ID != BYBYTE_PLATFORM_MEGA
#include <SoftwareSerial.h>
#endif

namespace ByByte {

	// Bauds to try when hunting for AT mode (HC-0x family typical values)
	static const uint32_t kProbeBauds[] = { 9600, 38400, 19200, 57600, 115200, 4800, 2400, 1200 };

	static bool atTrySequence(Stream& s, const __FlashStringHelper* cmdF, const char* cmdC, const char* expect,
		String* out = nullptr, uint16_t timeoutMs = 500);

#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
	Bluetooth::Bluetooth(HardwareSerial& uart, bb::pins::by_byte_mega::BtMegaBluetoothPins layout)
		: _serial(&uart), _powerPin(layout.powerPin), _baud(0), _type(BtModuleType::Unknown), _isReady(false),
		_isChecking(false), _checkAttempts(0), _lastCheckTime(0) {}
#else
	Bluetooth::Bluetooth(bb::pins::by_byte_nano::BtSoftwareSerialPins pins)
		: _pins(pins), _uart(new SoftwareSerial(pins.rx, pins.tx)), _baud(0), _type(BtModuleType::Unknown),
		_isReady(false), _isChecking(false), _checkAttempts(0), _lastCheckTime(0) {}
#endif

	Bluetooth::~Bluetooth() {
#if BYBYTE_PLATFORM_ID != BYBYTE_PLATFORM_MEGA
		// Arduino SoftwareSerial has no virtual destructor; delete is well-defined for this concrete type.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
		delete _uart;
#pragma GCC diagnostic pop
#endif
	}

	void Bluetooth::closeUart() {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		_serial->end();
#else
		_uart->end();
#endif
	}

	void Bluetooth::openUart(uint32_t baud) {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		_serial->begin(baud);
#else
		_uart->begin(static_cast<long>(baud));
#endif
	}

	bool Bluetooth::begin(uint32_t desiredBaud) {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		pinMode(_powerPin, OUTPUT);
		digitalWrite(_powerPin, HIGH); // board default: HIGH powers the module
		delay(300); // allow regulator + module boot before first AT (avoids false "init FAILED")
		Serial.print(F("BT power pin D"));
		Serial.print(_powerPin);
		Serial.println(F(" = HIGH"));
#else
		Serial.print(F("BT SoftwareSerial RX="));
		Serial.print(_pins.rx);
		Serial.print(F(" TX="));
		Serial.println(_pins.tx);
#endif
		closeUart();
		openUart(desiredBaud);
		_baud = desiredBaud;
		startReadinessCheck();
		return true;
	}

	// beginPassive removed as per design: begin() handles stabilization and probing

	int Bluetooth::available() {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		if (!_serial) return 0;
		int n = _serial->available();
#else
		if (!_uart) return 0;
		int n = _uart->available();
#endif
		if (n > 0) {
			_isReady = true;
			_isChecking = false;
			return n;
		}
		isReady();
		return 0;
	}

	int Bluetooth::read() {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		return _serial ? _serial->read() : -1;
#else
		return _uart ? _uart->read() : -1;
#endif
	}

	size_t Bluetooth::readBytes(uint8_t* buffer, size_t length) {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		return _serial ? _serial->readBytes((char*)buffer, length) : 0;
#else
		return _uart ? _uart->readBytes((char*)buffer, length) : 0;
#endif
	}

	size_t Bluetooth::write(uint8_t b) {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		return _serial ? _serial->write(b) : 0;
#else
		return _uart ? _uart->write(b) : 0;
#endif
	}

	size_t Bluetooth::write(const uint8_t* data, size_t length) {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		return _serial ? _serial->write(data, length) : 0;
#else
		return _uart ? _uart->write(data, length) : 0;
#endif
	}

	void Bluetooth::flush() {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		if (_serial) _serial->flush();
#else
		if (_uart) _uart->flush();
#endif
	}

	void Bluetooth::powerOn() {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		pinMode(_powerPin, OUTPUT);
		digitalWrite(_powerPin, HIGH);
#endif
	}

	void Bluetooth::powerOff() {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		pinMode(_powerPin, OUTPUT);
		digitalWrite(_powerPin, LOW);
#endif
	}

	bool Bluetooth::rename(const char* newName) {
		char cmd[32];
		snprintf(cmd, sizeof(cmd), "AT+NAME%s", newName);
		return sendAT(cmd, "OK");
	}

	bool Bluetooth::getName(String& outName) {
		const char* cmds[] = { "AT+NAME?", "AT+NAME" };
		return queryFirstMatch(cmds, 2, "+NAME", &outName, 500);
	}

	bool Bluetooth::reset() {
		// HC-05/06: AT+RESET; HC-08/42 BLE: AT+RESET
		return sendAT(F("AT+RESET"), "OK", nullptr, 800);
	}

	void Bluetooth::startReadinessCheck() {
		_isReady = false;
		_isChecking = true;
		_checkAttempts = 0;
		_lastCheckTime = millis();
	}

	bool Bluetooth::isReady() {
		if (_isReady) return true; // If already ready, return true
		if (!_isChecking) return false; // If not checking, return false

		// Check every 1 second
		if (millis() - _lastCheckTime < 1000) return false;

		_lastCheckTime = millis();
		_checkAttempts++;

		// Try to ping the module
		if (ping(500)) {
			_isReady = true;
			_isChecking = false;
			return true;
		}

		// If more than 10 attempts failed, give up
		if (_checkAttempts >= 10) {
			Serial.println(F("BT init FAILED after 10 attempts"));
			_isChecking = false;
			_isReady = false;
		}

		return false;
	}

	bool Bluetooth::ping(uint16_t timeoutMs) {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		if (!_serial) return false;
		return atTrySequence(*_serial, F("AT"), nullptr, "OK", nullptr, timeoutMs);
#else
		if (!_uart) return false;
		return atTrySequence(*_uart, F("AT"), nullptr, "OK", nullptr, timeoutMs);
#endif
	}

	bool Bluetooth::setPin(const char* pin4digits) {
		// HC-05 classic: AT+PSWD=1234 or AT+PIN1234
		char cmd1[24]; snprintf(cmd1, sizeof(cmd1), "AT+PSWD=%s", pin4digits);
		if (sendAT(cmd1, "OK")) return true;
		char cmd2[24]; snprintf(cmd2, sizeof(cmd2), "AT+PIN%s", pin4digits);
		return sendAT(cmd2, "OK");
	}

	bool Bluetooth::restoreDefault() {
		// HC-05: AT+ORGL, HC-06: AT+DEFAULT, BLE: AT+RENEW
		if (sendAT(F("AT+ORGL"), "OK", nullptr, 800)) return true;
		if (sendAT(F("AT+DEFAULT"), "OK", nullptr, 800)) return true;
		return sendAT(F("AT+RENEW"), "OK", nullptr, 800);
	}

	bool Bluetooth::setBleName(const char* newName) {
		// HC-08/42 BLE often accept AT+NAME or AT+NAME= variant
		return rename(newName);
	}

	bool Bluetooth::getBleName(String& outName) {
		return getName(outName);
	}

	static bool atTrySequence(Stream& s, const __FlashStringHelper* cmdF, const char* cmdC, const char* expect, String* out, uint16_t timeoutMs) {
		// Try without CRLF
		while (s.available()) s.read();
		if (cmdF) { s.print(cmdF); } else { s.print(cmdC); }
		unsigned long t0 = millis();
		String resp;
		while (millis() - t0 < timeoutMs) {
			while (s.available()) resp += (char)s.read();
			if (resp.indexOf(expect) >= 0) { if (out) *out = resp; return true; }
			delay(5);
		}
		Serial.println(F("AT try without CRLF failed"));
		Serial.println(resp);
		// Try with CRLF
		while (s.available()) s.read();
		if (cmdF) { s.print(cmdF); s.print("\r\n"); } else { s.print(cmdC); s.print("\r\n"); }
		t0 = millis(); resp = String();
		while (millis() - t0 < timeoutMs) {
			while (s.available()) resp += (char)s.read();
			if (resp.indexOf(expect) >= 0) { if (out) *out = resp; return true; }
			delay(5);
		}
		Serial.println(F("AT try with CRLF failed"));
		Serial.println(resp);
		return false;
	}

	bool Bluetooth::enterAtMode(uint32_t& detectedBaud) {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		for (uint8_t i = 0; i < sizeof(kProbeBauds) / sizeof(kProbeBauds[0]); ++i) {
			_serial->end();
			_serial->begin(kProbeBauds[i]);
			delay(80);
			if (atTrySequence(*_serial, F("AT"), nullptr, "OK", nullptr, 600)) {
				detectedBaud = kProbeBauds[i];
				return true;
			}
			if (atTrySequence(*_serial, F("AT+VERSION?"), nullptr, "OK", nullptr, 600)) {
				detectedBaud = kProbeBauds[i];
				return true;
			}
		}
#else
		for (uint8_t i = 0; i < sizeof(kProbeBauds) / sizeof(kProbeBauds[0]); ++i) {
			_uart->end();
			_uart->begin(static_cast<long>(kProbeBauds[i]));
			delay(80);
			if (atTrySequence(*_uart, F("AT"), nullptr, "OK", nullptr, 600)) {
				detectedBaud = kProbeBauds[i];
				return true;
			}
			if (atTrySequence(*_uart, F("AT+VERSION?"), nullptr, "OK", nullptr, 600)) {
				detectedBaud = kProbeBauds[i];
				return true;
			}
		}
#endif
		return false;
	}

	bool Bluetooth::detectModuleType() {
		String resp;
		if (sendAT(F("AT+VERS?"), "+VERS", &resp, 300) || sendAT(F("AT+VERSION?"), "+VERSION", &resp, 300)) {
			if (resp.indexOf(F("HC-05")) >= 0) { _type = BtModuleType::HC05; return true; }
			if (resp.indexOf(F("HC-06")) >= 0) { _type = BtModuleType::HC06; return true; }
			if (resp.indexOf(F("HC-02")) >= 0) { _type = BtModuleType::HC02; return true; }
			if (resp.indexOf(F("HC-08")) >= 0) { _type = BtModuleType::HC08_BLE; return true; }
			if (resp.indexOf(F("HC-42")) >= 0) { _type = BtModuleType::HC42_BLE; return true; }
		}
		// BLE modules sometimes answer to AT+ROLE? or AT+ADDR?
		if (sendAT(F("AT+ROLE?"), "+ROLE", &resp, 300)) {
			if (resp.indexOf(F("ROLE")) >= 0) { _type = BtModuleType::HC42_BLE; return true; }
		}
		_type = BtModuleType::Unknown;
		return false;
	}

	bool Bluetooth::setModuleBaud(uint32_t newBaud) {
		// Many HC-02/06 prefer AT+BAUDx codes; try those first
		struct { uint32_t baud; const char* token; } map[] = {
				{1200, "1"},{2400, "2"},{4800, "3"},{9600, "4"},{19200, "5"},{38400, "6"},{57600, "7"},{115200, "8"}
		};
		for (uint8_t i = 0;i < sizeof(map) / sizeof(map[0]);++i) {
			if (map[i].baud == newBaud) {
				char c2[16]; snprintf(c2, sizeof(c2), "AT+BAUD%s", map[i].token);
				if (sendAT(c2, "OK", nullptr, 800)) return true;
			}
		}
		// HC-05 classic: AT+UART=baud,0,0
		char cmd[32];
		snprintf(cmd, sizeof(cmd), "AT+UART=%lu,0,0", (unsigned long)newBaud);
		if (sendAT(cmd, "OK", nullptr, 800)) return true;
		return false;
	}

	bool Bluetooth::sendAT(const __FlashStringHelper* cmd, const char* expectOk, String* out, uint16_t timeoutMs) {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		if (!_serial) return false;
		auto* rx = _serial;
#else
		if (!_uart) return false;
		auto* rx = _uart;
#endif
		while (rx->available()) rx->read();
		rx->print(cmd);
		rx->print("\r\n");
		unsigned long t0 = millis();
		String resp;
		while (millis() - t0 < timeoutMs) {
			while (rx->available()) resp += (char)rx->read();
			if (resp.indexOf(expectOk) >= 0) {
				if (out) *out = resp;
				return true;
			}
			delay(5);
		}
		if (out) *out = resp;
		return false;
	}

	bool Bluetooth::sendAT(const char* cmd, const char* expectOk, String* out, uint16_t timeoutMs) {
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
		if (!_serial) return false;
		auto* rx = _serial;
#else
		if (!_uart) return false;
		auto* rx = _uart;
#endif
		while (rx->available()) rx->read();
		rx->print(cmd);
		rx->print("\r\n");
		unsigned long t0 = millis();
		String resp;
		while (millis() - t0 < timeoutMs) {
			while (rx->available()) resp += (char)rx->read();
			if (resp.indexOf(expectOk) >= 0) {
				if (out) *out = resp;
				return true;
			}
			delay(5);
		}
		if (out) *out = resp;
		return false;
	}

	bool Bluetooth::queryFirstMatch(const char* const* cmds, size_t n, const char* expectOk, String* out, uint16_t timeoutMs) {
		for (size_t i = 0;i < n;i++) {
			if (sendAT(cmds[i], expectOk, out, timeoutMs)) return true;
		}
		return false;
	}

} // namespace ByByte


