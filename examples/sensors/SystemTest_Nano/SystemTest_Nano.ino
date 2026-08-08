/*
 * ByByteLib - ByByteNano Peripheral Test (Arduino Nano)
 *
 * Purpose:
 * - Test every peripheral available on the ByByteNano kit from a single sketch
 * - Receive one-character commands over USB Serial and exercise one feature each
 *
 * Peripherals covered (Nano defaults from ByByteConfig):
 *   - Motors    DRV8833  right D5/D6, left D9/D10  (max PWM 220)
 *   - Buzzer    horn D11 (free functions, no class/singleton)
 *   - Side IR   left A6, right A7, power D4
 *   - LDR       A5
 *   - IR remote RX D8  (test mode 'j' — uses Arduino-IRremote directly)
 *   - WS2812    D7, 2 LEDs
 *   - Bluetooth HC-0x on NeoSWSerial D2(RX)/D3(TX)  (external PCINT)
 *
 * Conflict-free shared-PCINT architecture (Nano):
 *   - PcintManager (ByByteLib) is the SINGLE owner of the PCINT0/1/2 vectors.
 *   - Bluetooth uses NeoSWSerial built with NEOSWSERIAL_EXTERNAL_PCINT, so it
 *     emits NO ISR; PcintManager routes the RX pin's byte edges back into
 *     NeoSWSerial::rxISR(port) via a per-group raw-port hook.
 *   - NeoSWSerial at 16 MHz reads Timer0's free-running counter (no Timer2
 *     action), so it never conflicts with the buzzer's tone() (Timer2) or the
 *     D9/D10 motor PWM.
 *   => No vector clash, no PCINT ownership fight, no timer fight. Bluetooth +
 *      motors + buzzer coexist stably.
 *
 * --- Timer2 handoff for the IR test (IMPORTANT) -----------------------------
 * On AVR the Arduino core `tone()` (used by the buzzer) and IRremote's
 * `IrReceiver` BOTH use Timer2, so they cannot run at the same time. This sketch
 * therefore does NOT start IR reception at boot — the buzzer owns Timer2 by
 * default. The 'j' command enters the IR receive-test mode by
 *   1) silencing the buzzer (buzzerNoTone) and,
 *   2) only then calling IrReceiver.begin() (which takes Timer2).
 * While in IR test mode, buzzer commands are refused (send 'x' to leave IR mode
 * first). On exit (one frame received, cancelled by 'x'), IrReceiver.stop()
 * releases Timer2 and the buzzer is usable again.
 * IRremote's IrReceiver uses NO pin-change interrupt — it only touches Timer2 —
 * so it does not conflict with PcintManager/NeoSWSerial on the PCINT vectors.
 *
 * Not covered on Nano (by design):
 *   - Sonar (SonarPrecise / Sonar) — TimerManager on Nano reconfigures Timer1,
 *     which the left motor channel (D9/D10) uses for PWM. Battery/LM35 are
 *     Mega-only (their ADC pins A0/A1 are line sensors on Nano).
 *
 * Commands (case-sensitive):
 *   Motors:    F  forward         B  backward        L  turn left       R  turn right
 *              S  stop            +  speed +20       -  speed -20
 *   Buzzer:    z  short beep      h  car horn        r  R2D2            e  siren
 *              x  buzzer stop / cancel IR-test mode
 *   Sensors:   i  side IR         l  LDR             s Sonar (measure & print)
 *   IR test:   j  wait for ONE IR command (decode + raw dump), then return to
 *                 default. 'x' cancels IR-test mode. Buzzer is unavailable
 *                 while IR-test is active (Timer2 conflict).
 *   LEDs:      w  headlights toggle on/off          o  all LEDs off
 *   Bluetooth: t  ping + print name & baud
 *   Help:      ?  print this menu
 *
 * Module-only usage:
 * - Pulls in the ByByte modules it needs (see platformio.ini) + Adafruit
 *   NeoPixel + Arduino-IRremote. Nothing else is compiled/linked.
 */

#include <ByByteCore.h>
#include <MotorDriver.h>
#include <Buzzer.h>
#include <SideIrSensors.h>
#include <LdrSensor.h>
#include <Bluetooth.h>
#include <Sonar.h>
#include <Adafruit_NeoPixel.h>

 // --- IR receive (Arduino-IRremote) ------------------------------------------
 // Macros MUST be defined before #include <IRremote.hpp>. The default ByByteNano
 // IR remote receiver pin is D8.
#define IR_RECEIVE_PIN      8
#define NO_LED_FEEDBACK_CODE                  // we have no feedback LED wired
#include <IRremote.hpp>

using namespace ByByte;

// --- Peripheral instances (pins auto-selected from ByByteConfig for Nano) ----
MotorDriver    motors;                 // DRV8833, right D5/D6, left D9/D10
Bluetooth      bluetooth;              // NeoSWSerial D2/D3 (external PCINT)
SideIrSensors  sideIr;                 // A6/A7, power D4
LdrSensor      ldr(BYBYTE_LDR_ADC_PIN, true);
Sonar          sonar;                   // default pins from ByByteConfig

Adafruit_NeoPixel strip(BYBYTE_WS2812_COUNT, BYBYTE_WS2812_PIN, NEO_GRB + NEO_KHZ800);

// --- State -------------------------------------------------------------------
static int16_t  g_speed = 100;       // current motor speed (1..BYBYTE_MAX_PWM)
static bool     g_headlights = false;   // WS2812 headlight state
static bool     g_irReceive = false;    // IR receive-test mode (Timer2 owned by IRremote)
static bool     g_sonarMeasure = false; // sonar measurement mode

static const int16_t SPEED_STEP = 20;

// --- Help --------------------------------------------------------------------
static void printHelp() {
	Serial.println(F("=== ByByteNano Peripheral Test ==="));
	Serial.println(F("Motors: F/B/L/R move, S stop, +/- speed"));
	Serial.println(F("Buzzer: z beep, h horn, r R2D2, e siren, x stop"));
	Serial.println(F("Sensors: i sideIR, l LDR, s sonar"));
	Serial.println(F("IR test: j wait for one IR cmd (raw+decode), x cancels"));
	Serial.println(F("         (buzzer unavailable while IR-test active)"));
	Serial.println(F("LEDs: w headlights toggle, o off"));
	Serial.println(F("Bluetooth: t ping/name"));
	Serial.println(F("Help: ?"));
	Serial.print(F("Speed: ")); Serial.println(g_speed);
}

// --- Motors ------------------------------------------------------------------
static void cmdMotors(char c) {
	switch (c) {
	case 'F': motors.forward(g_speed);    Serial.println(F("MOT forward"));  break;
	case 'B': motors.backward(g_speed);   Serial.println(F("MOT backward")); break;
	case 'L': motors.turnLeft(g_speed);   Serial.println(F("MOT turnLeft")); break;
	case 'R': motors.turnRight(g_speed);  Serial.println(F("MOT turnRight"));break;
	case 'S': motors.stop();              Serial.println(F("MOT stop"));     break;
	}
}

static void cmdSpeed(char c) {
	if (c == '+') g_speed += SPEED_STEP;
	else          g_speed -= SPEED_STEP;
	if (g_speed < 1) g_speed = 1;
	if (g_speed > BYBYTE_MAX_PWM) g_speed = BYBYTE_MAX_PWM;
	Serial.print(F("Speed set ")); Serial.println(g_speed);
	Serial.println(F("(send a move command to apply)"));
}

// --- Buzzer ------------------------------------------------------------------
static void cmdBuzzer(char c) {
	switch (c) {
	case 'z': buzzerPatternButton();  Serial.println(F("BUZ beep"));   break;
	case 'h': buzzerPatternCarHorn(); Serial.println(F("BUZ carHorn"));break;
	case 'r': buzzerPatternR2D2();    Serial.println(F("BUZ R2D2"));   break;
	case 'e': buzzerPatternSiren();    Serial.println(F("BUZ siren"));  break;
	case 'x': buzzerNoTone();         Serial.println(F("BUZ stop"));   break;
	}
}

// --- Sensors -----------------------------------------------------------------
static void cmdSideIr() {
	uint16_t leftRaw, rightRaw;
	sideIr.sample(leftRaw, rightRaw);
	Serial.print(F("SideIR raw L=")); Serial.print(leftRaw);
	Serial.print(F(" R=")); Serial.println(rightRaw);
}

static void cmdLdr() {
	uint16_t raw = ldr.readRaw();
	uint16_t norm = ldr.readNormalized(); // 0..1000 (1000 = dark with invert)
	Serial.print(F("LDR raw=")); Serial.print(raw);
	Serial.print(F(" norm=")); Serial.println(norm);
}

static void cmdSonarMeasure() {
	if (g_sonarMeasure) {
		Serial.println(F("Sonar mode already active (x to cancel)"));
		return;
	}
	sonar.begin();
	g_sonarMeasure = true;
	Serial.println(F("Sonar mode ON"));
	Serial.println(F("Send 'x' to return to main menu"));
}

static void exitSonarMeasure() {
	sonar.end();
	g_sonarMeasure = false;
	Serial.println(F("Sonar mode OFF"));
	printHelp();
}

static void handleSonarMeasure() {
	uint16_t distance = sonar.readCm();
	if (distance == 0) {
		Serial.println(F("Sonar: no echo"));
	} else {
		Serial.print(F("Sonar: ")); Serial.print(distance); Serial.println(F(" cm"));
	}
	delay(150);
}

// --- IR receive-test mode ----------------------------------------------------
// Enters IR receive-test mode: takes Timer2 from the buzzer, starts IRremote,
// and waits (non-blocking, loop-polled) for ONE frame. The frame is printed to
// Serial (decoded fields + raw timing dump), then IR is stopped and Timer2 is
// released back to the buzzer. Sending 'x' cancels without waiting for a frame.
static void cmdIrReceive() {
	if (g_irReceive) {                       // already active — ignore re-entry
		Serial.println(F("IR-test already active (x to cancel)"));
		return;
	}
	// 1) Silence the buzzer FIRST so it no longer touches Timer2.
	buzzerNoTone();
	// 2) Start IRremote — takes over Timer2 for receive timing.
	IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
	g_irReceive = true;
	Serial.println(F("IR-test ON (Timer2 taken from buzzer)"));
	Serial.println(F("Point remote at D8 RX and press a key. 'x' to cancel."));
}

// Consume one decoded IR frame (called from loop while g_irReceive is true).
static void handleIrFrame() {
	Serial.println(F("------ IR frame received ------"));

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

	Serial.println(F("--- raw timing dump (50 us ticks) ---"));
	IrReceiver.printIRResultRawFormatted(&Serial, true);
	Serial.println(F("--------------------------------"));
}

// Leave IR-test mode: stop IRremote, release Timer2, restore buzzer capability.
static void exitIrReceive() {
	IrReceiver.stop();                       // release Timer2
	// No explicit buzzer re-init needed: tone() re-configures Timer2 on next use.
	g_irReceive = false;
	Serial.println(F("IR-test OFF (Timer2 released to buzzer) -> default mode"));
}

// --- WS2812 ------------------------------------------------------------------
static void applyHeadlights() {
	uint32_t front = g_headlights ? strip.Color(255, 255, 200) : strip.Color(0, 0, 0);
	strip.setPixelColor(0, front);
	if (BYBYTE_WS2812_COUNT > 1) strip.setPixelColor(1, front);
	strip.show();
}

static void cmdLeds(char c) {
	if (c == 'w') {
		g_headlights = !g_headlights;
		applyHeadlights();
		Serial.print(F("Headlights ")); Serial.println(g_headlights ? F("ON") : F("OFF"));
	} else if (c == 'o') {
		g_headlights = false;
		for (uint16_t i = 0; i < BYBYTE_WS2812_COUNT; i++) strip.setPixelColor(i, 0);
		strip.show();
		Serial.println(F("LEDs off"));
	}
}

// --- Bluetooth ---------------------------------------------------------------
static void cmdBluetooth() {
	Serial.println(F("BT ping..."));
	bool ok = bluetooth.ping(800);
	Serial.print(F("BT ping ")); Serial.println(ok ? F("OK") : F("FAIL"));
	Serial.print(F("BT baud ")); Serial.println(bluetooth.baud());
	String name;
	if (bluetooth.getName(name)) {
		Serial.print(F("BT name ")); Serial.println(name);
	} else {
		Serial.println(F("BT name (no response)"));
	}
}

// --- Dispatch ----------------------------------------------------------------
static void handleCmd(char c) {
	if (g_sonarMeasure) {
		if (c == 'x') {
			exitSonarMeasure();
		} else {
			Serial.println(F("Sonar mode active: send 'x' to return to menu"));
		}
		return;
	}

	// While IR-test is active, only 'x' (cancel) is honored; everything else
	// that would touch Timer2 (buzzer) is refused. Motors/LEDs/sensors are
	// still allowed because they do not touch Timer2.
	if (g_irReceive && (c == 'z' || c == 'h' || c == 'r' || c == 'e')) {
		Serial.println(F("IR-test active: buzzer unavailable (Timer2 owned by IRremote)"));
		Serial.println(F("Send 'x' to cancel IR-test first."));
		return;
	}

	switch (c) {
	case 'F': case 'B': case 'L': case 'R': case 'S': cmdMotors(c); break;
	case '+': case '-':                               cmdSpeed(c);  break;
	case 'z': case 'h': case 'r': case 'e':           cmdBuzzer(c); break;
	case 'x':
		if (g_irReceive) exitIrReceive();
		else             cmdBuzzer('x');
		break;
	case 'i': cmdSideIr();   break;
	case 'l': cmdLdr();       break;
	case 's': cmdSonarMeasure(); break;
	case 'j': cmdIrReceive(); break;
	case 'w': cmdLeds(c);    break;
	case 't': cmdBluetooth(); break;
	case '?': printHelp();    break;
	case '\r': case '\n': break; // ignore line endings
	default:
		Serial.print(F("Unknown cmd '")); Serial.write(c);
		Serial.println(F("' - send '?' for help"));
		break;
	}
}

void setup() {
	Serial.begin(9600);
	while (!Serial) {}

	if (!motors.begin()) Serial.println(F("WARN: motor begin failed"));
	buzzerBegin();           // uses BYBYTE_HORN_PIN; buzzer owns Timer2 by default
	sideIr.begin();
	ldr.begin();

	bluetooth.begin(9600);   // NeoSWSerial D2/D3; PcintManager routes its RX edges

	strip.begin();
	strip.clear();
	strip.show();

	printHelp();
	Serial.println(F("Send a single character command."));
}

void loop() {
	if (Serial.available()) {
		char c = (char)Serial.read();
		handleCmd(c);
	}

	if (g_sonarMeasure) {
		handleSonarMeasure();
	} else if (g_irReceive) {
		if (IrReceiver.decode()) {
			handleIrFrame();
			IrReceiver.resume();        // ready for next (in case 'j' is sent again)
			exitIrReceive();            // one frame done -> release Timer2 to buzzer
		}
		// While IR-test owns Timer2 we MUST NOT call buzzerUpdate() — it would
		// pull Timer2 out from under IRremote. Skip the buzzer tick here.
	} else {
		// Advance the buzzer patterns only when Timer2 is free.
		buzzerUpdate();
	}

	// Apply any pending differential target (no-op in Direct mode).
	motors.update();

	// Advance the bluetooth readiness check (non-blocking).
	(void)bluetooth.isReady();
}