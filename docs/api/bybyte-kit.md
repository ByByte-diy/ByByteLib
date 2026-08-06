# `ByByte::ByByteKit` & Kit Abstractions

> Product kit abstractions that bundle pre-wired motor and Bluetooth
> instances with platform-default pins. Also documents aggregate headers
> and the `ByByteLib::version()` method.
>
> **Headers:** `src/ByByteKits.h`, `src/core/configs/ByByteProduct.h`
> **Namespace:** `ByByte`
> **Kind:** Abstract base + concrete kit classes

The kit layer provides a single-line entry point: pick a kit class
matching your board, construct it with default pins, call
`beginMotors()` / `beginBluetooth()`, then use the public `motors` and
`bluetooth` members directly.

Three concrete kits are defined:

| Kit | Board | Motor driver | Bluetooth transport | Power control |
|---|---|---|---|---|
| `ByByteNano`    | Nano / Uno | DRV8833 (dual-PWM) | `SoftwareSerial` (RX=D2, TX=D3) | None (always powered) |
| `ByByteMega`    | Mega 2560  | TB6612 (DIR+PWM+STBY) | `HardwareSerial` (`Serial1`) + power pin (D29) | `HIGH` = powered |
| `ByByteNanoBoy` | Console / sim | None | None | None |

---

## Synopsis

```cpp
namespace ByByte {

enum class PlatformKit : uint8_t {
    Nano = 0,
    Mega = 1,
    NanoBoy = 2,
};

class ByByteKit {
public:
    virtual ~ByByteKit() = default;
    virtual PlatformKit kit() const noexcept = 0;
    virtual const char* name() const noexcept = 0;

protected:
    ByByteKit() = default;
};

class ByByteNano final : public ByByteKit {
public:
    MotorDriver motors;
    Bluetooth   bluetooth;

    ByByteNano() = default;

    PlatformKit kit() const noexcept override;
    const char* name() const noexcept override;

    bool beginMotors();
    bool beginBluetooth(uint32_t baud = 9600);
};

class ByByteMega final : public ByByteKit {
public:
    MotorDriver motors;
    Bluetooth   bluetooth;

    ByByteMega() = default;

    PlatformKit kit() const noexcept override;
    const char* name() const noexcept override;

    bool beginMotors();
    bool beginBluetooth(uint32_t baud = 9600);
};

class ByByteNanoBoy final : public ByByteKit {
public:
    ByByteNanoBoy() = default;

    PlatformKit kit() const noexcept override;
    const char* name() const noexcept override;
};

class ByByteLib {
public:
    static const char* version();
};

} // namespace ByByte
```

---

## Aggregate headers

Three convenience headers let sketches pull in groups of modules with a
single `#include`:

| Header | Scope |
|---|---|
| `<ByByteLib.h>`  | **All modules** — motors, sensors, Bluetooth, IR, servo, sonar, buzzer, and kits. |
| `<ByByteCore.h>` | **Foundation only** — `Types`, `PinCapabilities`, platform detection, `ByByteConfig`, product types (`PlatformKit`, `ByByteKit`). |
| `<ByByteKits.h>` | **Kit abstractions** — `ByByteProduct` (`PlatformKit` + `ByByteKit`), `Bluetooth`, `MotorDriver`. Includes the three concrete kit classes. |

Each per-module public header (`<MotorDriver.h>`, `<Bluetooth.h>`, …)
already pulls the foundation transitively via `<ByByteCore.h>`, so most
sketches only need the specific module header or `<ByByteLib.h>`.

### `ByByteLib::version()`

```cpp
static const char* version();
```

**Returns:** `"0.2.0"`.

String literal; no allocation. Always safe to call before any kit or
module is constructed. Defined in `src/ByByteLib.cpp`.

---

## Types

### `enum class PlatformKit : uint8_t`

Tag identifying the product kit chosen in user code.

| Enumerator | Value | Kit class | Board |
|---|---|---|---|
| `Nano`    | `0` | `ByByteNano`    | Nano / Uno |
| `Mega`    | `1` | `ByByteMega`    | Mega 2560  |
| `NanoBoy` | `2` | `ByByteNanoBoy` | Console / sim |

> **Kits are chosen explicitly in code**, not by compile-time macros.
> The board pin defaults are platform-dependent (selected by
> `PlatformDetect`), but which *kit* to instantiate is the author's
> choice.

---

### `class ByByteKit` — abstract base

```cpp
class ByByteKit {
public:
    virtual ~ByByteKit() = default;

    virtual PlatformKit kit() const noexcept = 0;
    virtual const char* name() const noexcept = 0;

protected:
    ByByteKit() = default;
};
```

| Member | Kind | Returns | Purpose |
|---|---|---|---|
| `~ByByteKit()` | Virtual default | — | Polymorphic cleanup. |
| `kit()` | Pure virtual | `PlatformKit` | The kit identity (`Nano`, `Mega`, `NanoBoy`). |
| `name()` | Pure virtual | `const char*` | Human-readable name (`"ByByteNano"`, …). |
| `ByByteKit()` | Protected | — | Cannot be instantiated directly — only subclasses. |

The protected constructor ensures only the concrete kit classes can be
constructed. `ByByteKit` itself is never heap-allocated (concrete kits
are held by value or on the stack), but the virtual destructor keeps the
contract safe for pointer/reference use.

---

## Concrete kits

### `ByByteNano`

```
┌──────────────────────────────────────────┐
│  ByByteNano                              │
│  ├─ motors  : MotorDriver (DRV8833)      │
│  └─ bluetooth : Bluetooth (SoftwareSerial)│
└──────────────────────────────────────────┘
```

```cpp
class ByByteNano final : public ByByteKit {
public:
    MotorDriver motors;
    Bluetooth   bluetooth;
    // ...
};
```

| Member | Type | Description |
|---|---|---|
| `motors`    | `MotorDriver` | DRV8833 dual-PWM H-bridge, default pins from `ByByteConfig`. |
| `bluetooth` | `Bluetooth`   | HC-0x on `SoftwareSerial` (RX=D2, TX=D3). No power control. |

| Method | Returns | Purpose |
|---|---|---|
| `kit()` | `PlatformKit::Nano` | Kit identity tag. |
| `name()` | `"ByByteNano"` | Human-readable kit name. |
| `beginMotors()` | `bool` | `true` when `motors.begin()` succeeds (pin validation passes). |
| `beginBluetooth(baud=9600)` | `bool` | `true` unconditionally — opens the UART and arms the readiness check; poll `bluetooth.isReady()`. |

`ByByteNano` is default-constructible. The `MotorDriver` and `Bluetooth`
members are also default-constructed from platform pin defaults — no
manual wiring needed. See [MotorDriver](motor-driver.md) and
[Bluetooth](bluetooth.md) for the full API on each member.

---

### `ByByteMega`

```
┌──────────────────────────────────────────┐
│  ByByteMega                              │
│  ├─ motors  : MotorDriver (TB6612)       │
│  └─ bluetooth : Bluetooth (Serial1 + PWR)│
└──────────────────────────────────────────┘
```

```cpp
class ByByteMega final : public ByByteKit {
public:
    MotorDriver motors;
    Bluetooth   bluetooth;
    // ...
};
```

| Member | Type | Description |
|---|---|---|
| `motors`    | `MotorDriver` | TB6612 DIR+PWM+STBY bridge, default pins from `ByByteConfig`. |
| `bluetooth` | `Bluetooth`   | HC-0x on `HardwareSerial` (`Serial1`), power gated on D29 (`HIGH` = powered). |

| Method | Returns | Purpose |
|---|---|---|
| `kit()` | `PlatformKit::Mega` | Kit identity tag. |
| `name()` | `"ByByteMega"` | Human-readable kit name. |
| `beginMotors()` | `bool` | `true` when `motors.begin()` succeeds. |
| `beginBluetooth(baud=9600)` | `bool` | `true` unconditionally — powers the module, opens the UART, arms readiness check. |

Same default-constructible pattern as `ByByteNano`. The `Bluetooth`
constructor binds to `Serial1` and the power pin automatically; see
[Bluetooth](bluetooth.md) for details.

---

### `ByByteNanoBoy`

```cpp
class ByByteNanoBoy final : public ByByteKit {
public:
    ByByteNanoBoy() = default;

    PlatformKit kit() const noexcept override;   // → NanoBoy
    const char* name() const noexcept override;  // → "ByByteNanoBoy"
};
```

| Member | Description |
|---|---|
| *(none yet)* | No onboard `MotorDriver` or `Bluetooth` — intended for console / simulation I/O. |

`ByByteNanoBoy` is a stub kit for host / simulator targets where
physical motor and Bluetooth hardware does not exist. Extend this class
with console or simulation I/O bridges as they are modeled.

| Method | Returns | Purpose |
|---|---|---|
| `kit()` | `PlatformKit::NanoBoy` | Kit identity tag. |
| `name()` | `"ByByteNanoBoy"` | Human-readable kit name. |

---

## Lifecycle

```
Pick a kit class → construct (default, all pins from config)
                   → beginMotors()    // returns bool
                   → beginBluetooth([baud])  // returns bool, then poll bluetooth.isReady()
                   → use motors.bluetooth members directly
                   → destroy (held by value — automatic cleanup)
```

1. **Pick the kit** matching your board: `ByByteNano` for Nano,
   `ByByteMega` for Mega, `ByByteNanoBoy` for console/sim.
2. **Construct with defaults** — all pins come from platform config.
   No parameters needed.
3. **`beginMotors()`** — validates PWM pins and enables the driver.
   Returns `true` on success; `false` signals a pin error.
4. **`beginBluetooth([baud])`** — opens the UART and arms the
   non-blocking readiness probe. Returns `true` unconditionally (the
   UART opened). Poll `bluetooth.isReady()` from `loop()` until `true`.
5. **Use members directly** — `motors.setMotorSpeeds(…)`,
   `bluetooth.write(…)`, etc. The kit owns the members by value; no
   pointers, no manual `delete`.
6. **Destroy** — the kit instance scope cleans up both `MotorDriver`
   and `Bluetooth` (including the heap `SoftwareSerial` on Nano).

---

## Usage notes

- **Kits own their members by value** — no heap allocation, no
  `new`/`delete`. The `Bluetooth` member on Nano internally heap-allocates
  a `SoftwareSerial*`, but that is cleaned up by `~Bluetooth()`.
- **Pins are resolved at construction** from `ByByteConfig.h` /
  platform pin maps. Override with `#define` before including the header
  (see [Configuration](#configuration) in the individual module docs).
- **`beginBluetooth()` is not a readiness verdict** — it arms the probe;
  you must poll `bluetooth.isReady()` to confirm the module is answering.
- **`ByByteNanoBoy` has no onboard modules** — it is a stub for console /
  simulation targets. Extend it for sim I/O.
- **Both `motors` and `bluetooth` are non-copyable** (`= delete`d copy
  constructors), so the kit classes are implicitly non-copyable as well.
  Hold by value or pass by reference.
- **`ByByteNano` and `ByByteMega` final** — the kit classes are `final`;
  inherit from `ByByteKit` directly for custom kits.
- **Aggregate includes** — prefer `<ByByteLib.h>` for sketches that use
  multiple modules; use `<ByByteKits.h>` when you only need the kit
  layer + `MotorDriver` + `Bluetooth`; use `<ByByteCore.h>` for
  pure-foundation work (types, config, platform detection).

---

## File map

| File | Role |
|---|---|
| `src/ByByteKits.h` | Declares `ByByteNano`, `ByByteMega`, `ByByteNanoBoy`. |
| `src/core/configs/ByByteProduct.h` | Declares `PlatformKit` enum and `ByByteKit` base. |
| `src/MotorDriver.h` | Motor module (public member of Nano and Mega kits). |
| `src/Bluetooth.h` | Bluetooth module (public member of Nano and Mega kits). |
| `src/ByByteLib.h` | Aggregate header (all modules) + `ByByteLib::version()`. |
| `src/ByByteCore.h` | Foundation aggregate header (public `#include` proxy for `core/ByByteCore.h`). |
| `src/core/ByByteCore.h` | Foundation umbrella: `Types`, `PinCapabilities`, platform detection, configs, product types. |
| `src/ByByteLib.cpp` | Implements `ByByteLib::version()`. |

---

## See also

- [MotorDriver](motor-driver.md) — full API for the `motors` member.
- [Bluetooth](bluetooth.md) — full API for the `bluetooth` member.
- [MotorController](motor-controller.md) — abstract backend (advanced; for custom motor drivers).
