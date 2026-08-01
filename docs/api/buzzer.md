# `ByByte::Buzzer`

> Non-blocking hardware-tone buzzer supporting single tones (with optional
> duration) and built-in **pattern sequences**, all driven from `update()`.
> Built-in tone tables live in flash (`PROGMEM`) to save SRAM.
>
> **Header:** `src/Buzzer.h`
> **Namespace:** `ByByte`
> **Kind:** Concrete class

`Buzzer` wraps a passive buzzer (or piezo) on a single digital pin and uses
the Arduino core's `::tone()` / `::noTone()` for PWM generation. It is
**non-blocking**: instead of `delay()`, the caller starts a tone or pattern
and then calls `update()` from `loop()`. A finite-duration single tone stops
itself when its deadline elapses; a multi-step pattern steps through its
frequency/duration tables one entry at a time as `update()` fires.

There are three modes of operation:

| Mode | Started by | Stopped by |
|---|---|---|
| **Sustained tone** | `tone(f, 0)` (duration `0`) | explicit `noTone()`, or starting a new tone/pattern |
| **Timed tone** | `tone(f, d)` (`d > 0`) | `update()` when `d` ms elapse, or `noTone()` |
| **Pattern sequence** | any `patternXxx()` | `update()` after the last step (+ repeats), or `noTone()` |

---

## Synopsis

```cpp
namespace ByByte {

class Buzzer {
public:
    explicit Buzzer(uint8_t pin = BYBYTE_HORN_PIN);

    void begin();

    void tone(uint16_t freqHz, uint16_t durationMs = 0);
    void noTone();
    void update();

    void patternCarHorn(uint16_t repeat = 2, uint16_t baseHz = 440);
    void patternSiren(uint16_t repeat = 2);
    void patternR2D2();
    void patternClick();
    void patternHappy();
    void patternSad();
    void patternSurprise();
    void patternDisconnect();
    void patternButton();
};

} // namespace ByByte
```

---

## Configuration

The default pin comes from the active platform config via
`configs/ByByteConfig.h`. It is overridable with `#define` before including
the header, or per instance at construction.

| Build target | Macro | Default pin |
|---|---|---|
| Nano (`BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO`) | `BYBYTE_HORN_PIN` | `11` |
| Mega (`BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA`) | `BYBYTE_HORN_PIN` | `45` |

---

## Public interface

### `explicit Buzzer(uint8_t pin = BYBYTE_HORN_PIN)`

Construct a buzzer bound to `pin`.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `pin` | `uint8_t` | `BYBYTE_HORN_PIN` | Digital output driving the buzzer / piezo. |

The constructor is non-blocking and performs no I/O; it only stores the pin
and zeroes all sequencing state.

### `void begin()`

Configure the pin and silence the output. Internally:

1. `pinMode(pin, OUTPUT)`.
2. `stopPwm()` — `::noTone(pin)` then `digitalWrite(pin, LOW)`.

**Returns:** nothing. Call exactly once (typically in `setup()`).

### `void tone(uint16_t freqHz, uint16_t durationMs = 0)`

Start a single tone.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `freqHz`    | `uint16_t` | — *(required)* | Tone frequency in Hz. `0` ⇒ silence (will stop PWM). |
| `durationMs`| `uint16_t` | `0` | Duration in ms. **`0` ⇒ sustained** (no auto-stop). Any value `> 0` arms a one-shot deadline checked by `update()`. |

Behavior:
- Clears any active pattern.
- Calls `::tone(pin, freqHz)` (or `::noTone()` if `freqHz == 0`).
- If `durationMs > 0`, records a stop deadline `millis() + durationMs`.
- If `durationMs == 0`, the tone sustains until `noTone()` or another
  tone/pattern is started.

> Name collision caveat: `Buzzer::tone` is a **member** that wraps the global
  `::tone()`. There is no ADC/duty parameter — Arduino's two-argument
  `tone(pin, freq)` form is used.

**Returns:** nothing.

### `void noTone()`

Immediately silence the buzzer and cancel any pattern.

- Cancels the timed-tone deadline and clears all sequence state.
- Calls `::noTone(pin)` and drives the pin `LOW`.

**Returns:** nothing.

### `void update()`

**Poll from `loop()`** — this is the non-blocking engine. It advances both
timed tones and pattern sequences:

1. **Timed tone:** if a deadline is armed `_untilMs != 0` and
   `millis() >= _untilMs`, stop PWM and clear the deadline.
2. **Pattern sequence:** if a sequence is active and `millis() >= _seqNextMs`:
   - If the index passed the end, either decrement the repeat counter and
     restart (`_seqRepeat > 1`) or stop and clear the sequence.
   - Otherwise read the next frequency `f` and duration `d` from the
     sequence tables, advance the index, set `_seqNextMs = now + d`, and start
     PWM at `f`.

Reading from the sequence tables is storage-transparent: built-in patterns
live in `PROGMEM` and are fetched with `pgm_read_word()`; runtime-built
patterns live in RAM. The caller does not need to know which.

**Returns:** nothing. Idempotent when nothing is active.

### `void patternCarHorn(uint16_t repeat = 2, uint16_t baseHz = 440)`

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `repeat`  | `uint16_t` | `2`  | Number of times to play the two-beep motif. |
| `baseHz`  | `uint16_t` | `440` | Base pitch (Hz) of the first beep; the second beep is `baseHz + 40`. |

Two-beep "car horn" motif: `baseHz` → pause → `baseHz + 40`, each beep
350 ms with a 120 ms gap. Because the pitch depends on the runtime `baseHz`
argument, the sequence is built into RAM buffers (`_tmpFreq` / `_tmpDur`)
— it is not one of the flash-resident tables.

**Returns:** nothing. Then call `update()` repeatedly.

### `void patternSiren(uint16_t repeat = 2)`

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `repeat` | `uint16_t` | `2` | Number of full siren passes. |

Intermittent alarm: 8-element on/off table (1000 Hz beeps alternating with
200 ms silences), total 1600 ms per pass. Sequence lives in flash
(`SIREN_FREQ` / `SIREN_DUR` in `PROGMEM`).

**Returns:** nothing.

### `void patternR2D2()`

Play a 20-step R2D2-style chirp sequence: quick upsweep, pause, down-chirp,
pause, rising ladder, pause, fall. Durations are 35–80 ms for snappy bleeps.
Sequence lives in flash (`R2D2_FREQ` / `R2D2_DUR` in `PROGMEM`). Played once.

**Returns:** nothing.

### `void patternClick()`

Single short key-click: 3000 Hz for 30 ms. Sequence lives in flash
(`CLICK_FREQ` / `CLICK_DUR` in `PROGMEM`). Played once.

**Returns:** nothing.

### `void patternHappy()`

OttoDIY-inspired rising three-tone cue: 800 / 1000 / 1300 Hz (120/120/180 ms).
Built in RAM.

### `void patternSad()`

Falling two-tone with pause: 700 ms @ 700 Hz, 100 ms silence, 300 ms @ 500 Hz.
Built in RAM.

### `void patternSurprise()`

Quick up-down: 1200 / 1600 / 1000 Hz (100/120/140 ms). Built in RAM.

### `void patternDisconnect()`

Two descending beeps: 900 Hz (180 ms), pause 100 ms, 700 Hz (220 ms). Built in RAM.

### `void patternButton()`

Short double click: two 30 ms 2500 Hz clicks with a 40 ms gap. Built in RAM.

---

## Built-in tone tables (flash, `PROGMEM`)

The fixed-pattern frequency/duration tables are stored in program memory
so they cost **0 bytes of SRAM**. They are NOT part of the public API (they
are file-static in `Buzzer.cpp`), but the audio characteristics of each
pattern are documented here for reference.

| Table pair (freq/dur) | Length | Pattern use | Notes |
|---|---|---|---|
| `CAR_FREQ` / `CAR_DUR` | 3 | (template only; `patternCarHorn` rebuilds into RAM with `baseHz`) | `480` here is a fixed stand-in; the runtime version uses `baseHz + 40`. |
| `SIREN_FREQ` / `SIREN_DUR` | 8 | `patternSiren` | 1000 Hz alternating with silence, 200 ms each. |
| `R2D2_FREQ` / `R2D2_DUR` | 20 | `patternR2D2` | Upsweep / chirp / ladder / fall. |
| `CLICK_FREQ` / `CLICK_DUR` | 1 | `patternClick` | 3000 Hz, 30 ms. |

> AVR is a Harvard architecture — flash `PROGMEM` arrays **must not** be read
> with `operator[]`. The implementation fetches every element through an
> internal `readSeq() → pgm_read_word()` helper; users never touch the
> tables directly.

---

## Tone vs. pattern interaction

| Call while … | Effect |
|---|---|
| `tone(f, d)` while a pattern is active | Cancels the pattern; starts a (timed or sustained) tone. |
| `patternXxx()` while a tone is active | Cancels the tone; starts the pattern. |
| `noTone()` while anything is active | Immediately silences and clears all state. |
| `update()` while inactive | No-op. |
| Starting a new tone/pattern | The pin is re-armed; the previous step is abandoned without an explicit stop. |

---

## Lifecycle

```
construct(pin) ──► begin() ──► tone(...) / patternXxx() (start) ──► update() (poll in loop, repeat) ──► noTone() (optional, or auto)
```

1. **Construct** with the output pin (defaults to `BYBYTE_HORN_PIN`).
2. **`begin()`** once — sets pin mode and silences.
3. **Start** a tone (`tone(f, d)`) or a pattern (`patternXxx()`).
4. **`update()`** from `loop()` — advances timed tones and pattern steps; a
   call typically takes microseconds (no `delay()`).
5. A timed tone or finite pattern **stops itself** via `update()`;
   sustained tones and infinite repeats stop via `noTone()`.

---

## Usage notes

- **Non-blocking:** `update()` must be polled frequently (e.g. every loop
  iteration). Pattern step granularity depends on `update()` cadence — a
  20 ms step is only accurate if `update()` runs at least every ~20 ms.
- **PWM source:** uses the Arduino core `::tone()` / `::noTone()`, so tone
  generation relies on the platform's hardware Timer (e.g. Timer2 on AVR).
  This may conflict with other libraries that reconfigure the same timer
  (PWM, IR, etc.). On AVR, `tone()` typically uses Timer2.
- **Pin must be PWM/timer-capable:** `::tone()` works on any digital pin via
  software toggle on AVR, but for low jitter pick a hardware-PWM-capable pin
  where possible. The default `BYBYTE_HORN_PIN` (11 on Nano, 45 on Mega) is
  chosen accordingly.
- **0 Hz:** passing `freqHz == 0` to `tone()` is treated as "silence" — it
  calls `stopPwm()` rather than starting a 0 Hz tone.
- **Flash sequences:** built-in patterns (`patternSiren`, `patternR2D2`,
  `patternClick`) are read from `PROGMEM`; runtime-parameterized patterns
  (`patternCarHorn`, `patternHappy`, …) are rebuilt into small RAM buffers
  on each call (8 entries max).
- **Single instance per pin:** one `Buzzer` owns one pin. Multiple instances
  on different pins are fine, but Arduino's `::tone()` can drive only one
  tone at a time per Timer on AVR — concurrent tones on multiple pins share
  that resource.
- **No public accessors** for `_pin`, sequence state, or `_untilMs`; the
  only introspection is observing whether sound stops via `update()`.
- **Blocking built-ins:** none. Every public method returns immediately.

---

## File map

| File | Role for `Buzzer` |
|---|---|
| `src/Buzzer.h` | Declares the class. |
| `src/Buzzer.cpp` | Implements the class; holds the `PROGMEM` tone tables and the `readSeq()` flash/RAM reader. |
| `src/configs/ByByteConfig.h` | Provides the `BYBYTE_HORN_PIN` default (`11` on Nano, `45` on Mega). |
| `src/configs/PlatformDetect.h` | Selects the `BYBYTE_PLATFORM_ID` that `ByByteConfig.h` branches on. |