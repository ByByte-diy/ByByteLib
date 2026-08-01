# `ByByte::SideIrSensors`

> Pair of side-facing analog IR sensors (left + right) with **power-gated
> sampling**, averaging, optional auto-calibration, and a normalized
> `0..1000` output.
>
> **Header:** `src/SideIrSensors.h`
> **Namespace:** `ByByte`
> **Kind:** Concrete class (header-only, all methods inline)

`SideIrSensors` drives **two** IR photoreflectors mounted on the sides of a
robot. It controls their illumination via a single shared **power pin**
(emitters on during a reading, off otherwise — to save current and reduce
ambient coupling), reads both analog channels with multi-sample averaging,
and exposes both **raw** and **normalized** values. An optional
auto-calibration loop learns the observed min/max for each side; normalization
then maps each side's reading to a `0..1000` fraction of that range, with a
polarity flag (`invertNormalized`) so the curve can read "*closer → higher*".

---

## Synopsis

```cpp
namespace ByByte {

class SideIrSensors {
public:
    SideIrSensors(uint8_t powerPin      = BYBYTE_IR_POWER_PIN,
                  uint8_t leftAnalogPin  = BYBYTE_IR_LEFT_PIN,
                  uint8_t rightAnalogPin  = BYBYTE_IR_RIGHT_PIN,
                  bool   invertNormalized = true);

    void begin();

    void sample(uint16_t& leftRaw, uint16_t& rightRaw,
                uint16_t settleMs = 5, uint8_t samples = 4);

    void calibrateStep();
    void autoCalibrate(uint16_t steps = 50, uint16_t delayMs = 20);

    void readNormalized(uint16_t& leftNorm, uint16_t& rightNorm);

    // Calibration bounds (raw ADC counts)
    uint16_t leftMin()  const;
    uint16_t leftMax()  const;
    uint16_t rightMin() const;
    uint16_t rightMax() const;

    void setInvert(bool invert);
    bool invert() const;
};

} // namespace ByByte
```

---

## Configuration

Defaults come from the active platform config via `configs/ByByteConfig.h`.
All are overridable with `#define` before including the header, or per
instance at construction.

| Macro | Nano | Mega | Purpose |
|---|---|---|---|
| `BYBYTE_IR_POWER_PIN` | `4`    | `22`    | Digital pin gating the IR emitters (**HIGH** = on). |
| `BYBYTE_IR_LEFT_PIN`  | `A6`   | `A14`   | Analog input for the **left** IR sensor. |
| `BYBYTE_IR_RIGHT_PIN` | `A7`   | `A15`   | Analog input for the **right** IR sensor. |

> **Pins must be analog-capable** for the left/right channels (they use
> `analogRead()`). The power pin is a standard digital output.

---

## Public interface

### `SideIrSensors(uint8_t powerPin = BYBYTE_IR_POWER_PIN, uint8_t leftAnalogPin = BYBYTE_IR_LEFT_PIN, uint8_t rightAnalogPin = BYBYTE_IR_RIGHT_PIN, bool invertNormalized = true)`

Construct the sensor pair bound to a power pin and two analog channels.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `powerPin` | `uint8_t` | `BYBYTE_IR_POWER_PIN`  | Digital output gating both IR emitters. |
| `leftAnalogPin`  | `uint8_t` | `BYBYTE_IR_LEFT_PIN`  | Analog input for the left sensor. |
| `rightAnalogPin` | `uint8_t` | `BYBYTE_IR_RIGHT_PIN` | Analog input for the right sensor. |
| `invertNormalized` | `bool` | `true` | Polarity of normalized output (see `readNormalized()`). |

Initial state: calibration ranges are empty-by-convention
(`_leftMin = _rightMin = 1023`, `_leftMax = _rightMax = 0`), so
`readNormalized()` returns `0` until at least one `calibrateStep()` /
`autoCalibrate()` call observes the sensors.

### `void begin()`

Configure the power pin and switch the emitters off.

- `pinMode(powerPin, OUTPUT)`
- `digitalWrite(powerPin, LOW)` — emitters **off** by default to save power.

The analog pins need no `pinMode` on AVR (analog inputs are input by
default), so nothing is done for them.

**Returns:** nothing. Call once in `setup()`.

### `void sample(uint16_t& leftRaw, uint16_t& rightRaw, uint16_t settleMs = 5, uint8_t samples = 4)`

Power the emitters, take averaged readings, then power them back off.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `leftRaw`  | `uint16_t&` (out) | — | Averaged raw ADC count for the left sensor. |
| `rightRaw` | `uint16_t&` (out) | — | Averaged raw ADC count for the right sensor. |
| `settleMs` | `uint16_t` | `5`  | Delay (ms) after powering the emitters before sampling, to let the IR settle. |
| `samples`  | `uint8_t`  | `4`  | Number of `analogRead()`s summed per channel; result is the integer mean. |

**Returns:** the two averaged raw counts are written to `leftRaw` / `rightRaw`
(in ADC counts, `0..1023` on a 10-bit AVR).

Internally:
1. `digitalWrite(powerPin, HIGH)` — emitters on.
2. `delay(settleMs)` — wait for the IR diodes to stabilize.
3. For `samples` iterations, accumulate `analogRead(leftPin)` and
   `analogRead(rightPin)`.
4. Write the integer means `accL / samples`, `accR / samples`.
5. `digitalWrite(powerPin, LOW)` — emitters off.

> **Blocking:** this call lasts `settleMs` + `samples` × (two ADC
> conversions, ~104 µs each on a Nano). With defaults that is ≈ 5 ms + 4×~0.2 ms
> ≈ 6 ms. It also **does not** update the calibration bounds — call
> `calibrateStep()` for that (which itself calls `sample()`).

### `void calibrateStep()`

Take one fresh `sample()` and **widen** the per-side calibration bounds.

- Reads the current `leftRaw` / `rightRaw`.
- For each side, updates `_min`/`_max` to the running observed extremes.

Use this for **online** calibration — call it from `loop()` while sweeping the
sensor through the expected reflection range. Cheap and non-blocking aside
from the settle/delay inside `sample()`.

**Returns:** nothing. Bounds only ever *widen* within an instance's lifetime.

### `void autoCalibrate(uint16_t steps = 50, uint16_t delayMs = 20)`

Convenience wrapper: calls `calibrateStep()` then `delay(delayMs)` in a loop
for `steps` iterations. This is a **blocking** call lasting roughly
`steps × (settleMs + 2·ADC·默认samples + delayMs)`. With defaults ≈ 50 × ~6 ms
+ 50 × 20 ms ≈ 1.3 s. Drive the robot through the expected reflection range
while it runs so the bounds converge on the true min/max.

**Returns:** nothing.

### `void readNormalized(uint16_t& leftNorm, uint16_t& rightNorm)`

Power-sample both sensors, then map each reading through its calibration
range to a `0..1000` normalized value.

| Parameter | Type | Purpose |
|---|---|---|
| `leftNorm`  | `uint16_t&` (out) | Normalized reading for the left sensor (`0..1000`). |
| `rightNorm` | `uint16_t&` (out) | Normalized reading for the right sensor (`0..1000`). |

**Returns:** both normalized values via the out-parameters.

Behavior:
- Calls `sample(l, r, 5, 4)` internally (defaults).
- If uncalibrated (`vmax <= vmin` for that side) → returns `0` for that side.
- The span is guarded to at least `5` ADC counts to avoid numeric flips when
  the observed range is narrow.
- With `invert == false`: raw at `_max` → `1000`, at `_min` → `0`
  (brighter / closer-in-voltage → higher), i.e. higher-voltage → higher.
- With `invert == true` (**default**): raw at `_max` → `0`, at `_min` → `1000`
  (closer object ⇒ lower sensor voltage ⇒ higher normalized) — the usual
  convention for proximity, since reflective IR dividers lower their output
  voltage when an object is near.

### `uint16_t leftMin() const` / `uint16_t leftMax() const` / `uint16_t rightMin() const` / `uint16_t rightMax() const`

Read-only access to the raw calibration bounds (ADC counts).

| Return | Meaning |
|---|---|
| `uint16_t` | Running observed min or max for the named side. |

Initial values are `1023` (mins) and `0` (maxes) — i.e. an empty range —
until `calibrateStep()` / `autoCalibrate()` widens them. `const`-qualified.

### `void setInvert(bool invert)`

Change the normalized-output polarity at runtime.

| Parameter | Type | Purpose |
|---|---|---|
| `invert` | `bool` | `true` ⇒ closer object → higher value (default); `false` ⇒ higher-voltage → higher value. |

**Returns:** nothing. Affects subsequent `readNormalized()` calls only.

### `bool invert() const`

| Return | Current polarity flag. |
|---|---|

`const`-qualified; read-only introspection.

---

## Normalization math

For a raw reading `v` and per-side bounds `[_min, _max]`:

```
span = max(_max - _min, 5)                 # guard against tiny ranges
num  = invert ? (_max - v) : (v - _min)
num  = clamp(num, 0, span)
out  = clamp((num * 1000) / span, 0, 1000)
```

The output is scaled to `1000` (per-mille), independent of the ADC
resolution. The minimum-span guard (`5`) prevents noise from producing
flipped/dominated values when the observed range is very narrow (e.g. a
uniform background).

---

## Power-gating model

```
begin()  ─► powerPin LOW (emitters off, default state)
sample() ─► powerPin HIGH ─► delay(settleMs) ─► analogRead×samples×2 ─► powerPin LOW
```

Emitters are on **only** during the settle + sampling window. This:

- cuts average IR current dramatically (duty ≈ sample-time / sample-period),
- reduces ambient-light coupling because the reading is taken promptly after
  the emitters fire,
- but means **every** `sample()` / `readNormalized()` / `calibrateStep()` is
  a timed, blocking pulse — keep the call rate sane (typ. ≤ 100 Hz).

---

## Lifecycle

```
construct(powerPin, leftPin, rightPin, invert) ──► begin() ──► autoCalibrate() (or calibrateStep() × N in loop)
                                              ──► readNormalized()/sample() (poll, repeat)
```

1. **Construct** with the power pin, two analog inputs, and the polarity flag.
2. **`begin()`** once — sets the power pin `OUTPUT` and turns emitters off.
3. **Calibrate** — call `autoCalibrate()` once in `setup()` while sweeping
   the reflection conditions, or call `calibrateStep()` over time in `loop()`.
4. **Read** — call `readNormalized()` in `loop()` for stable `0..1000`
   values, or `sample()` for raw averaged ADC counts.
5. Held by value; no cleanup needed. (Bounds are only widened; reconstruct
   to re-train from scratch.)

---

## Usage notes

- **Power-gated / blocking:** every read pulses the emitters and waits
  `settleMs` — avoid calling `sample()`/`readNormalized()`/`calibrateStep()`
  inside a tight real-time control loop at high rate.
- **Shared power pin:** the two emitters share one power pin, so they are
  always sampled together. You cannot read only the left or only the right —
  `sample()` always returns both.
- **Default polarity is proximity-oriented:** `invertNormalized = true` so
  *closer → higher*, matching typical reflective-IR divider wiring. Use
  `setInvert(false)` if your front-end pulls the ADC voltage the other way.
- **Uncalibrated safety:** before any calibration sample, `readNormalized()`
  returns `0` rather than a misleading mid-range value. The `leftMin/leftMax/
  rightMin/rightMax` accessors expose the institutional empty ranges
  (`1023`/`0`).
- **Bounds only widen:** within one instance's lifetime, `_min`/`_max` are
  monotonic — re-train by destroying and reconstructing.
- **Averaging reduces noise:** `samples=4` trades ~0.8 ms for noise
  reduction; raise it for a noisier electrical environment, lower it for speed.
- **Multi-instance / single ADC:** multiple `SideIrSensors` instances are
  fine, but they share the AVR's single ADC MUX; reads are serialized by the
  hardware. Each instance owns its own pins and bounds.
- **Analog pin capability:** left/right pins must be `analogRead()`-capable
  (`A6/A7` on Nano, `A14/A15` on Mega). The power pin is a plain digital output.
- **No public pin accessors** — `_powerPin`, `_leftPin`, `_rightPin` are
  private; pins are fixed at construction. The `normalize()` helper is private
  static.

---

## File map

| File | Role for `SideIrSensors` |
|---|---|
| `src/SideIrSensors.h` | Defines the class (header-only, all inline). |
| `src/configs/ByByteConfig.h` | Provides `BYBYTE_IR_POWER_PIN`, `BYBYTE_IR_LEFT_PIN`, `BYBYTE_IR_RIGHT_PIN` defaults per platform. |
| `src/configs/PlatformDetect.h` | Selects the `BYBYTE_PLATFORM_ID` (NANO / MEGA) that `ByByteConfig.h` branches on. |