# `ByByte::BatterySensor`

> Single-cell Li-ion battery monitor: voltage via ADC, charge-status via a
> digital `CHRG` pin, and a linear state-of-charge estimate.
>
> **Header:** `src/BatterySensor.h`
> **Namespace:** `ByByte`
> **Kind:** Concrete class (header-only, all methods inline)

> ## ⚠ Platform availability — read first
>
> **`BatterySensor` is intended for the ByByte **Mega** platform. On the
> ByByte **Nano** it is a stub.**
>
> The class itself compiles on any AVR target, but the default pins and the
> charge-status wiring only make sense on the Mega:
>
> | Platform | `BYBYTE_BAT_ADC_PIN` | `BYBYTE_BAT_VREF_MV` | `BYBYTE_CHRG_PIN` | Status |
> |---|---|---|---|---|
> | **Mega** | `A0` | `5000` | `15` (CHRG on PJ0 → D15, **LOW = charging**) | ✅ Fully usable |
> | **Nano** | `A0` | `5000` | `0` | ⚠ **Stub** — no dedicated CHRG input on the Nano carrier; the charge-status pin defaults to `0` (Arduino `D0`/`RX`), which is **not** a real charge-status line. |
>
> The Nano defaults are emitted "for compatibility" only (see the comment
> *"Optional sensors not present on Nano"* in `configs/ByByteConfig.h`). Do
> not rely on `isCharging()` on the Nano unless you explicitly wire a real
> charge-status signal and pass its pin to the constructor.
>
> To **disable** the charge-status input on any platform, pass `0xFF` as the
> `chrgPin` argument — `isCharging()` then unconditionally returns `false`
> and `begin()` skips configuring the pin. This is the recommended way to use
> the voltage-only portion of the API on a stub target like the Nano.

---

## Synopsis

```cpp
namespace ByByte {

class BatterySensor {
public:
    BatterySensor(uint8_t adcPin   = BYBYTE_BAT_ADC_PIN,
                  uint16_t vrefMv  = BYBYTE_BAT_VREF_MV,
                  uint8_t chrgPin  = BYBYTE_CHRG_PIN);

    void begin();

    float   readVoltage();                 // volts
    bool    isCharging() const;            // CHRG pin low ⇒ charging
    uint8_t estimateSocPercent(float voltage) const;  // 1S Li-ion, 3.0–4.2 V

    void    setVrefMv(uint16_t mv);        // retune ADC reference voltage
};

} // namespace ByByte
```

---

## Configuration

Defaults come from the active platform config via `configs/ByByteConfig.h`.
All are overridable with `#define` before including the header, or per
instance at construction.

| Macro | Mega | Nano | Meaning |
|---|---|---|---|
| `BYBYTE_BAT_ADC_PIN` | `A0` | `A0` | Analog input on the battery voltage divider. |
| `BYBYTE_BAT_VREF_MV` | `5000` | `5000` | ADC full-scale reference voltage (assumes `DEFAULT` ~5 V). |
| `BYBYTE_CHRG_PIN` | `15` | `0` | Digital CHRG-status input (LOW = charging); pass `0xFF` to disable. |

> **No `0xFF` default:** the config ships a value (`0` on Nano, `15` on Mega)
> rather than a sentinel, so the charge-status input is *enabled* by default.
> On the Nano stub this means `isCharging()` will read `D0`; pass `0xFF`
> explicitly if you do not have a real CHRG line.

---

## Public interface

### `BatterySensor(uint8_t adcPin = BYBYTE_BAT_ADC_PIN, uint16_t vrefMv = BYBYTE_BAT_VREF_MV, uint8_t chrgPin = BYBYTE_CHRG_PIN)`

Construct a sensor bound to `adcPin` with a `vrefMv` full-scale reference and
an optional `chrgPin` charge-status input.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `adcPin`  | `uint8_t`  | `BYBYTE_BAT_ADC_PIN` | Analog input on the battery voltage divider. |
| `vrefMv`  | `uint16_t` | `BYBYTE_BAT_VREF_MV`  | ADC reference voltage in millivolts (assumes `DEFAULT` ~5 V; see `readVoltage()`). |
| `chrgPin` | `uint8_t`  | `BYBYTE_CHRG_PIN`     | Digital CHRG-status input. **`0xFF` ⇒ disabled** (`isCharging()` returns `false`, `begin()` skips `pinMode`). |

The constructor is non-blocking and performs no I/O; it only records the
configuration.

### `void begin()`

Initializes the charge-status input **only**.

If `chrgPin != 0xFF`, configures `chrgPin` as `INPUT_PULLUP` (the CHRG line is
active-LOW, so the internal pull-up gives a defined idle level). The ADC pin
needs no `pinMode` on AVR (analog inputs are input by default), so nothing is
done for `adcPin`.

**Returns:** nothing.

### `float readVoltage()`

| Return | Unit | Range | Meaning |
|---|---|---|---|
| `float` | volts | ~0 .. ~`vrefMv/1000` | Battery voltage at the ADC pin, expressed in volts. |

Reads the battery voltage divider and converts the ADC count to volts.
Internally:

1. Assumes the Arduino `DEFAULT` analog reference (~5 V) — it does **not**
   call `analogReference()`.
2. Performs a **discard read** (`analogRead()` once, result thrown away) to
   prime the ADC sample-and-hold.
3. Takes the **second** `analogRead()` as the stable sample.
4. Converts: `volts = vrefMv * raw / 1023.0 / 1000.0`.

The returned value is the voltage **at the ADC pin**. If your hardware uses a
voltage divider that scales the cell voltage down, divide again by your
divider ratio (or fold it into `vrefMv`) to get the actual cell voltage.
`estimateSocPercent()` expects the **cell** voltage, so apply any scaling
before passing it in.

### `bool isCharging() const`

| Return | Meaning |
|---|---|
| `true`  | `chrgPin` reads `LOW` → battery is charging. |
| `false` | `chrgPin` reads `HIGH` → not charging; **or** charge-status input is disabled (`chrgPin == 0xFF`). |

Reads the digital CHRG-status line. The CHRG pin is active-LOW (typical of
TP4056 / MCP73831 charge controllers), hence `LOW ⇒ charging`. When
`chrgPin == 0xFF` the input is disabled and the method unconditionally
returns `false` without touching any pin.

`const`-qualified; safe to call from a hot loop.

### `uint8_t estimateSocPercent(float voltage) const`

| Parameter | Type | Unit | Meaning |
|---|---|---|---|
| `voltage` | `float` | volts | **Cell** voltage (post-divider, what `readVoltage()` would return if there were no divider). |

| Return | Range | Meaning |
|---|---|---|
| `uint8_t` | `0 .. 100` | Linear state-of-charge estimate for a 1S Li-ion cell. |

A deliberately simple **linear** SoC approximation for a single Li-ion cell
over its useful voltage window:

```
if voltage <  3.0 V → 0   %
if voltage >  4.2 V → 100 %
else               → round((voltage - 3.0) * (100 / 1.2))
```

The 3.0–4.2 V window corresponds to a ~1.2 V span ⇒ slope
`100 % / 1.2 V ≈ 83.3 %/V`. This is a coarse model suitable for a quick
"gauge" readout, **not** a coulomb-counting fuel gauge; real Li-ion discharge
curves are non-linear (flat in the middle, steep at the ends), so expect
this to over-estimate in the 3.7–4.0 V region and under-estimate near 3.2–3.4 V.

`const`-qualified; does not perform I/O. Pass the result of `readVoltage()`
(adjusted for any divider ratio) to get a one-shot SoC snapshot.

### `void setVrefMv(uint16_t mv)`

| Parameter | Type | Purpose |
|---|---|---|
| `mv` | `uint16_t` | New ADC reference voltage in millivolts used by `readVoltage()`. |

Retunes the voltage conversion at runtime, e.g. if the analog reference is
changed away from the `DEFAULT` ~5 V that `readVoltage()` assumes.

**Returns:** nothing.

> **No public set accessors** exist for `adcPin` or `chrgPin` — they are
> fixed at construction. `_adcPin`, `_vrefMv`, `_chrgPin` are private.

---

## Conversion math

For a stable ADC count `raw`:

```
volts = (vrefMv * raw / 1023.0) / 1000.0       # at the ADC pin
cell  = volts / dividerRatio                    # if a divider is present
soc   = clamp((cell - 3.0) * (100 / 1.2), 0, 100)
charging = (chrgPin != 0xFF) and (digitalRead(chrgPin) == LOW)
```

The `1023` divisor is the 10-bit AVR ADC full-scale count.

---

## Lifecycle

```
construct(adcPin, vrefMv, chrgPin) ──► begin() ──► readVoltage() / isCharging() / estimateSocPercent() (repeat on demand)
```

1. **Construct** with the battery ADC pin, reference voltage (mV), and an
   optional charge-status pin (use `0xFF` to disable it).
2. **`begin()`** once — configures `chrgPin` as `INPUT_PULLUP` (skipped if
   `0xFF`). No setup needed for the analog pin.
3. **Read** — call `readVoltage()` for volts, `isCharging()` for charge
   status, and `estimateSocPercent(cellVoltage)` for a 0–100 gauge. Each
   `readVoltage()` performs its own ADC conversion (discard + sample).
4. Held by value; no cleanup needed.

---

## Usage notes

- **Mega-targeted, Nano-stub:** on the ByByte Nano the charge-status pin
  defaults to `0` (Arduino `D0`/`RX`), which is **not** a real CHRG line —
  this is compatibility scaffolding only. Either wire a real charge-status
  signal and pass its pin, or pass `0xFF` to disable `isCharging()` and use
  the voltage/SoC path alone.
- **Reference assumed:** `readVoltage()` always assumes the `DEFAULT`
  ~5 V analog reference. It does **not** call `analogReference()`. If another
  sensor in the sketch selects a different reference, either restore
  `analogReference(DEFAULT)` before reading the battery, or call
  `setVrefMv()` with the matching full-scale value.
- **Voltage divider scaling:** the returned voltage is the voltage at the
  ADC pin. If your board scales the cell voltage down (common, to fit a 4.2 V
  cell into a 5 V ADC range), divide by the divider ratio before calling
  `estimateSocPercent()`, or fold the ratio into `vrefMv`.
- **SoC model is coarse:** `estimateSocPercent()` is a *linear* 3.0–4.2 V
  approximation for a 1S Li-ion cell. Treat it as a rough gauge, not a
  precision fuel gauge.
- **Blocking:** each `readVoltage()` performs two ADC conversions (~104 µs
  each on a Nano). Safe to call from a control loop; `isCharging()` and
  `estimateSocPercent()` are non-blocking.
- **Multi-instance / single ADC:** multiple `BatterySensor` instances are
  fine, but they share the AVR's single ADC MUX; reads are serialized by the
  hardware. Each instance is independent.
- **CHRG polarity:** the CHRG pin is active-LOW (typical charge-controller
  behavior); `begin()` enables the internal pull-up so the idle level is
  well-defined.

---

## File map

| File | Role for `BatterySensor` |
|---|---|
| `src/BatterySensor.h` | Defines the class (header-only, all inline). |
| `src/configs/ByByteConfig.h` | Provides `BYBYTE_BAT_ADC_PIN`, `BYBYTE_BAT_VREF_MV`, `BYBYTE_CHRG_PIN` defaults per platform (Mega real wiring; Nano stub). |
| `src/configs/PlatformDetect.h` | Selects the `BYBYTE_PLATFORM_ID` (NANO / MEGA) that `ByByteConfig.h` branches on. |