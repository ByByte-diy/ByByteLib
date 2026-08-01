# `ByByte::LdrSensor`

> Light-dependent-resistor (LDR) analog sensor with auto-calibration and
> normalized output.
>
> **Header:** `src/LdrSensor.h`
> **Namespace:** `ByByte`
> **Kind:** Concrete class (header-only, all methods inline)

`LdrSensor` reads ambient light from a single analog LDR input. It exposes a
**raw** ADC reading, a **normalized** `0..1000` reading derived from a
calibration range, and a small **auto-calibration** loop that walks the
observed min/max bounds. The optional `invert` flag flips the normalized
curve so that, for example, *darker → higher value*.

The class is value-typed and trivially constructible; it stores only its ADC
pin, the `invert` flag, and two calibration bounds (`_min`, `_max`).

---

## Synopsis

```cpp
namespace ByByte {

class LdrSensor {
public:
    LdrSensor(uint8_t adcPin = BYBYTE_LDR_ADC_PIN, bool invert = false);
    void begin();

    uint16_t readRaw();
    uint16_t readNormalized();

    void autoCalibrate(uint16_t steps = 50, uint16_t delayMs = 20);
    void calibrateStep();

    // (no public accessors for _min/_max are provided)
};

} // namespace ByByte
```

---

## Configuration

The default ADC pin comes from the active platform config via
`configs/ByByteConfig.h`. It is overridable with `#define` before including
the header.

| Build target | Macro | Default pin |
|---|---|---|
| Nano (`BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_NANO`) | `BYBYTE_LDR_ADC_PIN` | `A5` |
| Mega (`BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA`) | `BYBYTE_LDR_ADC_PIN` | `A2` |

Pass a custom `adcPin` to the constructor to override the default per instance.

---

## Public interface

### `LdrSensor(uint8_t adcPin = BYBYTE_LDR_ADC_PIN, bool invert = false)`

Construct a sensor bound to `adcPin`.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `adcPin`  | `uint8_t` | `BYBYTE_LDR_ADC_PIN` | Analog input reading the LDR voltage divider. |
| `invert` | `bool`    | `false` | If `true`, reverse the normalized output so *darker → higher*. |

Initial state: calibration range is empty-by-convention (`_min = 1023`,
`_max = 0`), which makes `readNormalized()` return `0` until at least one
`calibrateStep()` / `autoCalibrate()` call observes a reading.

### `void begin()`

No-op. Provided for API symmetry with the rest of the library (every sensor
exposes `begin()`). Safe to call from `setup()`.

**Returns:** nothing.

### `uint16_t readRaw()`

| Return | Range | Meaning |
|---|---|---|
| `uint16_t` | `0 .. 1023` | Raw Arduino ADC count on the configured pin. |

Reads the LDR voltage divider. Internally:

1. selects the Arduino `DEFAULT` analog reference (`analogReference(DEFAULT)`),
2. waits 2 ms for the reference to settle,
3. performs a **discard read** to prime the ADC S&H,
4. returns the **second** `analogRead()` (the stable sample).

Using only this method does **not** update the calibration bounds.

### `uint16_t readNormalized()`

| Return | Range | Meaning |
|---|---|---|
| `uint16_t` | `0 .. 1000` | Light level mapped through the calibration range, or `0` if uncalibrated. |

Returns the current light level as a normalized fraction of the calibrated
`[_min, _max]` span, scaled to `1000`. Behavior:

- If uncalibrated (`_max <= _min`) → returns `0`.
- The span is guarded to at least `5` ADC counts to avoid numeric flips when
  the observed range is very narrow.
- With `invert == false` (default): a reading at `_max` → `1000`, at `_min`
  → `0` (brighter → higher).
- With `invert == true`: a reading at `_max` → `0`, at `_min` → `1000`
  (darker → higher) — useful when the LDR sits in a pull-up divider where
  more light lowers the voltage.

This method calls `readRaw()` internally, so it discards-and-re-samples on
its own — no separate `readRaw()` needed before it.

### `void calibrateStep()`

Updates the internal `_min` / `_max` bounds with one fresh `readRaw()` sample.
Call repeatedly while moving the sensor through the expected light range so
the bounds converge on the true min/max. Cheap and non-blocking aside from
the ADC settle delay inside `readRaw()`.

**Returns:** nothing.

### `void autoCalibrate(uint16_t steps = 50, uint16_t delayMs = 20)`

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `steps`   | `uint16_t` | `50`  | Number of samples to take. |
| `delayMs` | `uint16_t` | `20`  | Delay between samples, in milliseconds. |

Convenience wrapper: calls `calibrateStep()` then `delay(delayMs)` in a loop
for `steps` iterations. This is a **blocking** call lasting roughly
`steps * (settle + delayMs)` ms (≈ `steps * ~22 ms` with the default 2 ms ADC
settle). Drive the robot while it runs so the bounds encompass the full
expected light range.

**Returns:** nothing.

> Bounds are only widened, never narrowed, within the lifetime of an
> instance. To re-train from scratch, destroy and reconstruct `LdrSensor`.
> (There is no public reset accessors — `_min`/`_max` are private.)

---

## Normalization math

For a raw reading `v` and bounds `[_min, _max]`:

```
span = max(_max - _min, 5)
num  = invert ? (_max - v) : (v - _min)
num  = clamp(num, 0, span)
out  = (num * 1000) / span
```

The output is intentionally scaled to `1000` (not `1023`) so it reads as a
*per-mille* light fraction that is independent of the ADC resolution.

---

## Lifecycle

```
construct(adcPin, invert) ──► autoCalibrate() (or calibrateStep() x N) ──► readNormalized() / readRaw() (repeat)
```

1. **Construct** with the analog pin and the desired polarity.
2. **`begin()`** (optional no-op, for API symmetry).
3. **Calibrate** — call `autoCalibrate()` once in `setup()` while sweeping the
   light conditions, or call `calibrateStep()` over time.
4. **Read** — call `readNormalized()` in `loop()` for a stable 0..1000 value,
   or `readRaw()` for the raw ADC count.
5. Held by value; no cleanup needed.

---

## Usage notes

- **Reference:** `readRaw()` hard-codes `analogReference(DEFAULT)`. If other
  sensors in the same sketch select a different analog reference, call the LDR
  read last or be aware the first sample after a reference change may be
  settling — the discard read in `readRaw()` mitigates this.
- **Blocking:** `readRaw()` delays 2 ms; `autoCalibrate()` blocks for the
  configured `steps * delayMs`. Avoid calling these inside a tight real-time
  control loop.
- **Multi-instance:** multiple `LdrSensor` instances are fine, but they share
  the AVR's single ADC MUX; concurrent analog reads are serialized by the
  hardware anyway.
- **Uncalibrated safety:** before any calibration sample, `readNormalized()`
  returns `0` rather than a misleading mid-range value.
- **No public bounds accessors:** `_min`/`_max` are private; the only way to
  set them is through `calibrateStep()` / `autoCalibrate()`.

---

## File map

| File | Role for `LdrSensor` |
|---|---|
| `src/LdrSensor.h` | Defines the class (header-only, all inline). |
| `src/configs/ByByteConfig.h` | Provides the `BYBYTE_LDR_ADC_PIN` default (`A5` on Nano, `A2` on Mega). |
| `src/configs/PlatformDetect.h` | Selects the `BYBYTE_PLATFORM_ID` used by `ByByteConfig.h`. |