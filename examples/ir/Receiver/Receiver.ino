/*
 * ByByteLib - IR Receiver Diagnostic (Nano / Mega)
 * -------------------------------------------------
 * Standalone IR receive test using the Arduino-IRremote library directly
 * (the `IrReceiver` instance + `decode()` / `resume()` API). It is independent
 * of ByByteLib's modules and PcintManager, so it serves as a clean reference
 * that confirms your IR receiver hardware + remote are seen by IRremote, and
 * prints the decoded protocol / address / command plus the raw timing buffer
 * so you can read your remote's exact codes.
 *
 * IMPORTANT (AVR): IRremote's IrReceiver uses Timer2 for receive timing. Do NOT
 * run the buzzer (tone()) in this same sketch — tone() also owns Timer2 and they
 * conflict. The buzzer is not used here, so this sketch is conflict-free.
 *
 * Wiring (IR receiver module OUT -> Arduino digital pin):
 *   - Nano: connect to D8  (the ByByteNano default IR RX pin)
 *   - Mega: connect to D14 (the ByByteMega  default IR RX pin)
 *   Set IR_RECEIVE_PIN below to match your wiring.
 *
 * Build:  pio run -e nano  |  pio run -e mega
 * Upload, open Serial Monitor @115200, point a remote at the receiver and
 * press any key. Each frame prints:
 *   - decoded protocol / address / command / decodedRawData / isRepeat
 *   - numberOfBits
 *   - the formatted raw timing dump (50 us ticks, Mark + Space alternating)
 */

 // --- IR receive pin ---------------------------------------------------------
 // Change this to match your wiring. Defaults below match the ByByte kits:
 //   Nano -> D8, Mega -> D14.
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#  define IR_RECEIVE_PIN  14
#else
#  define IR_RECEIVE_PIN  8
#endif

// Optional: omit feedback LED code (we have no feedback LED wired).
#define NO_LED_FEEDBACK_CODE

#include <IRremote.hpp>

void setup() {
	Serial.begin(115200);
	while (!Serial) {}

	Serial.println();
	Serial.println(F("=== IR Receiver Diagnostic (IRremote) ==="));
	Serial.print(F("RX pin  = D")); Serial.println(IR_RECEIVE_PIN);
	Serial.println(F("Timer2  = reserved by IRremote (do NOT use buzzer/tone here)"));
	Serial.println(F("Ready. Point your remote at the IR receiver and press a key."));
	Serial.println();

	// Start the IRremote receiver. Second arg = LED feedback (false = disabled).
	IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
}

void loop() {
	if (IrReceiver.decode()) {

		Serial.println(F("------ IR frame received ------"));

		// Decoded summary. IrReceiver fills decodedIRData during decode().
		Serial.print(F("protocol       = "));
		Serial.println(getProtocolString(IrReceiver.decodedIRData.protocol));
		Serial.print(F("address        = 0x"));
		Serial.println(IrReceiver.decodedIRData.address, HEX);
		Serial.print(F("command        = 0x"));
		Serial.println(IrReceiver.decodedIRData.command, HEX);
		Serial.print(F("decodedRawData = 0x"));
		Serial.println((unsigned long)IrReceiver.decodedIRData.decodedRawData, HEX);
		Serial.print(F("numberOfBits   = "));
		Serial.println(IrReceiver.decodedIRData.numberOfBits);
		Serial.print(F("isRepeat       = "));
		Serial.println((IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)
			? F("YES") : F("NO"));

		// IRremote's own raw timing dump (50 us ticks, Mark / Space alternating).
		Serial.println(F("--- raw timing dump (50 us ticks) ---"));
		IrReceiver.printIRResultRawFormatted(&Serial, true);

		Serial.println(F("--------------------------------"));
		Serial.println();

		// Arm the next frame.
		IrReceiver.resume();
	}
}