/*
 * ByByteLib - Bluetooth Car Control (Mega/Nano)
 *
 * Controls the robot via single-character commands over Bluetooth.
 * Mega uses Serial1 and optional pins for lights/horn/extra; Nano uses SoftwareSerial.
 * Unsupported hardware on Nano is silently ignored.
 *
 * Commands:
 *  F/B/L/R - forward/back/left/right
 *  G/I/H/J - forward-left / forward-right / back-left / back-right
 *  S - stop; D - stop all (also turns off outputs)
 *  W/w - front light on/off; U/u - back light on/off
 *  V/v - horn on/off; X/x - extra on/off
 *  1..9 -> speed 0..90%; q -> 100%
 */

#include <ByByteLib.h>
#include <Adafruit_NeoPixel.h>

using namespace ByByte;

Bluetooth bt;
MotorDriver motor; // auto-detected driver/pins

// Use pins from ByByteConfig
static const uint8_t PIN_WS2812 = BYBYTE_WS2812_PIN;
static const uint16_t NUM_WS2812 = BYBYTE_WS2812_COUNT;
#if defined(BYBYTE_HEADLIGHT_LEFT_PIN)
static const uint8_t PIN_WHITE1 = BYBYTE_HEADLIGHT_LEFT_PIN; // Mega
#else
static const uint8_t PIN_WHITE1 = 0; // Nano: not present
#endif
#if defined(BYBYTE_HEADLIGHT_RIGHT_PIN)
static const uint8_t PIN_WHITE2 = BYBYTE_HEADLIGHT_RIGHT_PIN; // Mega
#else
static const uint8_t PIN_WHITE2 = 0; // Nano: not present
#endif
static const uint8_t PIN_HORN = BYBYTE_HORN_PIN;              // D45 Mega, D11 Nano
static Buzzer buzzer;

static inline void safePinMode(uint8_t pin, uint8_t mode) { if (pin) pinMode(pin, mode); }
static inline void safeWrite(uint8_t pin, uint8_t val) { if (pin) digitalWrite(pin, val); }

static Adafruit_NeoPixel strip(NUM_WS2812, PIN_WS2812, NEO_GRB + NEO_KHZ800);
static inline void wsBegin() { if (NUM_WS2812) { strip.begin(); strip.show(); } }
static inline void wsFront(bool on) {
  if (!NUM_WS2812) return;
  uint32_t front = on ? strip.Color(255,255,200) : strip.Color(0,0,0);
  strip.setPixelColor(0, front);
  if (NUM_WS2812 > 1) strip.setPixelColor(1, front);
  strip.show();
}
static inline void wsBack(bool on) {
  if (!NUM_WS2812) return;
  if (NUM_WS2812 < 4) return; // only Mega has tails
  uint32_t tail = on ? strip.Color(80,0,0) : strip.Color(0,0,0);
  strip.setPixelColor(2, tail);
  strip.setPixelColor(3, tail);
  strip.show();
}
// Extra blinking state (1s period: 500ms on/off)
static bool g_extraBlinkEnabled = false;
static bool g_extraOnPhase = false;
static unsigned long g_extraLastMs = 0;

static inline void wsExtraSetEnabled(bool enable) {
  g_extraBlinkEnabled = enable;
  if (!NUM_WS2812) return;
  if (!enable) {
    // Turn off extra immediately
    for (uint16_t i=0;i<NUM_WS2812;i++) strip.setPixelColor(i, 0);
    strip.show();
  }
}

static inline void wsTickBlinkers() {
  if (!g_extraBlinkEnabled || !NUM_WS2812) return;
  unsigned long now = millis();
  if (now - g_extraLastMs < 500) return; // 500ms phase
  g_extraLastMs = now;
  g_extraOnPhase = !g_extraOnPhase;
  uint32_t amber = g_extraOnPhase ? strip.Color(255,80,0) : strip.Color(0,0,0);
  for (uint16_t i=0;i<NUM_WS2812;i++) strip.setPixelColor(i, amber);
  strip.show();
}

static uint8_t speedPct = 50; // 0..100

static void handleCmd(char c) {
  // speed
  if (c >= '1' && c <= '9') { speedPct = (uint8_t)((c - '1') * 10); return; }
  if (c == 'q') { speedPct = 100; return; }

  // outputs
  if (c == 'W') { safeWrite(PIN_WHITE1, HIGH); safeWrite(PIN_WHITE2, HIGH); wsFront(true); return; }
  if (c == 'w') { safeWrite(PIN_WHITE1, LOW);  safeWrite(PIN_WHITE2, LOW);  wsFront(false); return; }
  if (c == 'U') { wsBack(true); return; }
  if (c == 'u') { wsBack(false); return; }
  if (c == 'V') { buzzer.patternCarHorn(2); return; }
  if (c == 'v') { buzzer.noTone(); return; }
  if (c == 'X') { wsExtraSetEnabled(true);  return; }
  if (c == 'x') { wsExtraSetEnabled(false); return; }

  // motion
  const int16_t s = (int16_t)((BYBYTE_MAX_PWM * (long)speedPct) / 100L);
  switch (c) {
    case 'F': motor.setMotorSpeeds(s, s); break;
    case 'B': motor.setMotorSpeeds(-s, -s); break;
    case 'L': motor.setMotorSpeeds(-s, s); break;
    case 'R': motor.setMotorSpeeds(s, -s); break;
    case 'G': motor.setMotorSpeeds(s/2, s); break;
    case 'I': motor.setMotorSpeeds(s, s/2); break;
    case 'H': motor.setMotorSpeeds(-s/2, -s); break;
    case 'J': motor.setMotorSpeeds(-s, -s/2); break;
    case 'S': motor.stop(); break;
    case 'D':
      motor.stop();
      safeWrite(PIN_WHITE1, LOW); safeWrite(PIN_WHITE2, LOW);
      buzzer.noTone();
      wsFront(false); wsBack(false); wsExtraSetEnabled(false);
      break;
    default: break;
  }
}

void setup() {
  Serial.begin(9600);
  motor.begin();
  bt.begin(9600);
  safePinMode(PIN_HORN, OUTPUT);
  safePinMode(PIN_WHITE1, OUTPUT);
  safePinMode(PIN_WHITE2, OUTPUT);
  wsBegin();
  buzzer.begin();
}

void loop() {
  // Establish readiness lazily
  (void)bt.isReady();

  while (bt.available()) handleCmd((char)bt.read());
  // periodic tasks (blinkers)
  wsTickBlinkers();
  buzzer.update();
}


