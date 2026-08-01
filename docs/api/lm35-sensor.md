# `ByByte::Lm35Sensor`

> Analog temperature sensor reading an **LM35** (10 mV / °C) on a single ADC
> pin, returning Celsius or Fahrenheit.
>
> **Header:** `src/Lm35Sensor.h`
> **Namespace:** `ByByte`
> **Kind:** Concrete class (header-only, all methods inline)

`Lm35Sensor` measures ambient temperature via an LM35 sensor wired as a
voltage source feeding an AVR analog input. Output is temperature directly
(Celsius), using the LM35's native **10 mV / °C** transfer function. The
analog reference and the millivolt-per-count scale are selected per target MCU
so that the arithmetic stays correct across Nano (DEFAULT, ~5 V reference)
and Mega (INTERNAL1V1, ~1.1 V reference).

The class is value-typed and trivially constructible; it stores only its ADC
pin.

---

## Synopsis

```cpp
namespace ByByte {

class Lm35Sensor {
public:
    Lm35Sensor(uint8_t adcPin = BYBYTE_TMP_ADC_PIN);
    void begin();

    float readCelsius();
    float readFahrenheit();
};

} // namespace ByByte
```

---

## Configuration

The default ADC pin is supplied by the active platform config via
`configs/ByByteConfig.h`. It is overridable with `#define` before including
the header, or per-instance at construction.

| Build target | Macro | Default pin |
|---|---|---|
| Mega (`BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA`) | `BYBYTE_TMP_ADC_PIN` | `A1` |

For `BYBYTE_PLATFORM_NANO` this sensor not installed. But for compatibility use `A1` port. If we used the library for another platform, please define `BYBYTE_TMP_ADC_PIN` for you own device analog pin.

Pass a custom `adcPin` to the constructor to override the default per instance.

### Per-target analog reference

The conversion math depends on the active analog reference, selected by
`readCelsius()` itself based on the MCU:

| MCU | `analogReference(...)` | Effective full-scale | Counts → mV scale |
|---|---|---|---|
| `__AVR_ATmega2560__` (Mega) | `INTERNAL1V1` | ~1100 mV | `raw * 1100.0 / 1023.0` mV |
| All others (Nano/Uno/…) | `DEFAULT` | ~5000 mV | `raw * 5000.0 / 1023.0` mV |

Using the internal 1.1 V reference on the Mega improves resolution for the
LM35's small output range (10 mV / °C ⇒ at 100 °C the sensor outputs ~1.0 V,
well within 1.1 V full-scale).

---

## Public interface

### `Lm35Sensor(uint8_t adcPin = BYBYTE_TMP_ADC_PIN)`

Construct a sensor bound to `adcPin`.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `adcPin` | `uint8_t` | `BYBYTE_TMP_ADC_PIN` | Analog input connected to the LM35 output. |

The constructor is non-blocking and performs no I/O; it only records the pin.

### `void begin()`

No-op. Provided for API symmetry with the rest of the library (every sensor
exposes `begin()`). Safe to call from `setup()`.

**Returns:** nothing.

### `float readCelsius()`

| Return | Unit | Typical range | Meaning |
|---|---|---|---|
| `float` | °C | ~2 °C .. ~110 °C | Temperature in degrees Celsius. |

Reads the LM35 voltage and converts it to temperature. Internally:

1. Selects the per-target analog reference (`INTERNAL1V1` on Mega, `DEFAULT`
   otherwise).
2. Waits 5 ms for the reference to settle.
3. Performs a **discard read** to prime the ADC sample-and-hold, then takes
   the **second** `analogRead()` as the stable sample.
4. Converts the ADC count to millivolts using the per-target scale (see the
   configuration table above).
5. Divides by the LM35's **10 mV / °C** sensitivity to obtain °C.

Negative return values are not representable with the LM35 (its output is
ground-referenced for positive temperatures only).

### `float readFahrenheit()`

| Return | Unit | Meaning |
|---|---|---|
| `float` | °F | Temperature in degrees Fahrenheit. |

Convenience wrapper: calls `readCelsius()` and applies

```
°F = °C * 9.0 / 5.0 + 32.0
```

This performs its own ADC read (via `readCelsius()`), so it is independent of
any prior `readCelsius()` call — do not assume cached results.

---

## Conversion math

For a stable ADC count `raw`:

```
ref   = __AVR_ATmega2560__ ? 1100 mV : 5000 mV
mv    = raw * ref / 1023.0
celsius = mv / 10.0          # 10 mV / °C
fahrenheit = celsius * 9.0/5.0 + 32.0
```

The `10 mV / °C` factor is the LM35's nominal transfer function (from the
TI LM35 datasheet).

---

## Lifecycle

```
construct(adcPin) ──► begin() (optional no-op) ──► readCelsius() / readFahrenheit() (repeat on demand)
```

1. **Construct** with the analog pin (defaults to `BYBYTE_TMP_ADC_PIN`).
2. **`begin()`** (optional no-op, for API symmetry).
3. **Read** — call `readCelsius()` or `readFahrenheit()` whenever fresh
   temperature is required. There is no caching; each call performs its own
   ADC conversion (≈ 5 ms settle + two reads).
4. Held by value; no cleanup needed.

---

## Usage notes

- **Reference switching:** `readCelsius()` (and therefore `readFahrenheit()`)
  re-selects the analog reference on **every** call — `INTERNAL1V1` on Mega,
  `DEFAULT` elsewhere. If the same sketch uses other sensors expecting a
  particular reference, the reference will be changed after each LM35 read;
  re-establish the needed reference before reading those sensors.
- **Blocking:** each read delays ~5 ms for the reference to settle, plus two
  ADC conversions (~104 µs each on a Nano). Avoid calling inside a tight
  real-time control loop.
- **Multi-instance / single ADC:** multiple `Lm35Sensor` instances are fine,
  but they share the AVR's single ADC MUX; analog reads are serialized by the
  hardware. Each instance is independent.
- **Resolution:** on the Mega's 1.1 V reference, one ADC count ≈ 1.07 mV ≈
  0.11 °C. On a Nano's 5 V reference, one ADC count ≈ 4.89 mV ≈ 0.49 °C.
- **Self-heating:** the LM35 draws ~60 µA; negligible self-heating, but allow
  the sensor to thermally settle in still air for accurate absolute readings.
- **No public pin accessor:** `_adcPin` is private; the pin is fixed at
  construction.

---

## File map

| File | Role for `Lm35Sensor` |
|---|---|
| `src/Lm35Sensor.h` | Defines the class (header-only, all inline). |
| `src/configs/ByByteConfig.h` | Provides the `BYBYTE_TMP_ADC_PIN` default (`A1` on both Nano and Mega). |
| `src/configs/PlatformDetect.h` | Selects the `BYBYTE_PLATFORM_ID` used by `ByByteConfig.h`; the read path keys off `__AVR_ATmega2560__` directly. |