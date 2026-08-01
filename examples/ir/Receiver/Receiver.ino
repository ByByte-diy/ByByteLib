/*
 * ByByteLib - IR Receiver + Motor Control
 *
 * Purpose:
 * - Receive IR commands (Auto: NEC, RC5, RC6) and drive the robot
 * - Maps arrow keys to movement via MotorDriver convenience methods
 *
 * Hardware:
 * - IR RX pin auto-detected (Nano: D8, Mega: PJ1/D14)
 *
 * Usage:
 * - Open Serial Monitor @115200 to see decoded frames
 * - Use a common Arduino remote; adjust key codes if needed
 */
#include <ByByteLib.h>

using namespace ByByte;

IrReceiver ir(IrProtocol::Auto);
MotorDriver motor; // Auto-detected: DRV8833 on Nano / TB6612 on Mega

// Common NEC key codes on cheap remotes (adjust if needed)
// These are typical values; print received frames to learn your remote
const uint8_t KEY_UP = 0x18; // ▲
const uint8_t KEY_DOWN = 0x52; // ▼
const uint8_t KEY_LEFT = 0x08; // ◄
const uint8_t KEY_RIGHT = 0x5A; // ►
const uint8_t KEY_OK = 0x1C; // OK/Enter
const uint8_t KEY_STOP = 0x16; // often labeled as '0' or STOP

void setup() {
	Serial.begin(9600);
	Serial.println(F("=== IR Receiver Demo ==="));
	motor.begin();
	ir.begin();
	Serial.print(F("IR RX pin: ")); Serial.println(BYBYTE_IR_RX_PIN);
}

void handleCommand(const IrFrame& f) {
	// Basic control mapping
	switch (f.command) {
	case KEY_UP:
		motor.forward(120);
		Serial.println(F("CMD: FORWARD"));
		break;
	case KEY_DOWN:
		motor.backward(120);
		Serial.println(F("CMD: BACKWARD"));
		break;
	case KEY_LEFT:
		motor.turnLeft(120);
		Serial.println(F("CMD: TURN LEFT"));
		break;
	case KEY_RIGHT:
		motor.turnRight(120);
		Serial.println(F("CMD: TURN RIGHT"));
		break;
	case KEY_OK:
		motor.setTargetVelocity(0, 0); // switch to differential idle
		motor.stop();
		Serial.println(F("CMD: OK / STOP"));
		break;
	case KEY_STOP:
		motor.stop();
		Serial.println(F("CMD: STOP"));
		break;
	default:
		Serial.print(F("CMD: 0x")); Serial.print(f.command, HEX);
		Serial.print(F(" proto=")); Serial.println((int)f.proto);
		break;
	}
}

void loop() {
	if (ir.available()) {
		IrFrame f = ir.read();
		Serial.print(F("IR: proto=")); Serial.print((int)f.proto);
		Serial.print(F(" addr=0x")); Serial.print(f.address, HEX);
		Serial.print(F(" cmd=0x")); Serial.print(f.command, HEX);
		Serial.print(F(" rep=")); Serial.println(f.repeat ? "Y" : "N");
		if (!f.repeat) handleCommand(f);
	}
	motor.update();
	delay(10);
}
