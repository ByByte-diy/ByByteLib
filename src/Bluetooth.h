#ifndef BYBYTE_BLUETOOTH_H
#define BYBYTE_BLUETOOTH_H

#include <Arduino.h>
#include "ByByteConfig.h"

#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO
#include <SoftwareSerial.h>
#endif

namespace ByByte {

enum class BtModuleType { Unknown, HC02, HC05, HC06, HC08_BLE, HC42_BLE };

class Bluetooth {
public:
    Bluetooth();

    // Initialize module; automatically detects default baud and configures desired baud
    // Returns true on success
    bool begin(uint32_t desiredBaud = 9600);

    // Asynchronous readiness check (non-blocking)
    void startReadinessCheck();
    bool isReady(); // Checks time, pings module, updates flag
    bool isChecking() const { return _isChecking; }

    // Basic stream-like API
    int available();
    int read();
    size_t readBytes(uint8_t* buffer, size_t length);
    size_t write(uint8_t b);
    size_t write(const uint8_t* data, size_t length);
    void flush();

    // Power control (Mega only). On Nano these are no-ops.
    void powerOn();
    void powerOff();

    // AT helpers (return true on success)
    bool rename(const char* newName);
    bool getName(String& outName);
    bool reset();
    bool setPin(const char* pin4digits);
    bool restoreDefault();

    // BLE specific (best-effort; no-op on classic modules)
    bool setBleName(const char* newName);
    bool getBleName(String& outName);

    // Diagnostics: send simple AT and expect OK
    bool ping(uint16_t timeoutMs = 500);

    BtModuleType moduleType() const { return _type; }
    uint32_t baud() const { return _baud; }

private:
#if BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA
    HardwareSerial* _serial;
#else
    SoftwareSerial* _serial;
#endif
    uint32_t _baud;
    BtModuleType _type;
    bool _isReady;
    bool _isChecking;
    uint8_t _checkAttempts;
    unsigned long _lastCheckTime;

    bool enterAtMode(uint32_t& detectedBaud);
    bool detectModuleType();
    bool setModuleBaud(uint32_t newBaud);
    bool sendAT(const __FlashStringHelper* cmd, const char* expectOk, String* out = nullptr, uint16_t timeoutMs = 500);
    bool sendAT(const char* cmd, const char* expectOk, String* out = nullptr, uint16_t timeoutMs = 500);
    bool queryFirstMatch(const char* const* cmds, size_t n, const char* expectOk, String* out = nullptr, uint16_t timeoutMs = 500);
};

} // namespace ByByte

#endif // BYBYTE_BLUETOOTH_H


