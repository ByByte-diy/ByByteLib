# `ByByte::MotorController` 

> Abstract brushed dual-motor backend.
>
> **Header:** `src/motor/MotorController.h`
> **Namespace:** `ByByte`
> **Kind:** Abstract class (pure-virtual interface)

> ⚠ **Primary public API is `MotorDriver`** — `MotorController` is the
> abstract backend for custom driver implementations. Most users should use
> `MotorDriver` (see [motor-driver.md](motor-driver.md)) which auto-selects
> the correct driver chip and provides Direct + Differential modes.

`MotorController` is the polymorphic backend behind brushed DC motor driver
implementations (e.g. DRV8833, TB6612). It abstracts a **pair** of motors —
identified as *left* and *right* — behind a uniform
`setMotorSpeeds(left, right)` command so that higher-level components
(e.g. `DifferentialDriveController`, `MotorDriver`) can drive any supported
H-bridge chip without depending on its wiring specifics.

The contract is intentionally minimal:

| # | Method | Purpose |
|---|---|---|
| 1 | `begin()` | Configure GPIO and bring the driver online. |
| 2 | `setMotorSpeeds(left, right)` | Command both motors in PWM units. |
| 3 | *Destructor* | Virtual, so derived implementations delete cleanly through the base. |
| 4 | *Copy operations* | **Deleted** — instances are non-copyable singletons of their hardware. |

---

## Public interface

```cpp
namespace ByByte {

class MotorController {
public:
    MotorController() = default;
    virtual ~MotorController() = default;

    virtual bool begin() = 0;
    virtual void setMotorSpeeds(int16_t left, int16_t right) = 0;

    MotorController(const MotorController&)            = delete;
    MotorController& operator=(const MotorController&) = delete;
};

} // namespace ByByte
```

### `MotorController() = default`

Default constructor. The base type holds no state; concrete subclasses store
their pin assignments and are expected to accept them through their own
constructors.

### `virtual ~MotorController() = default`

Virtual destructor. Permits `delete` through a `MotorController*` so that
owning code (e.g. `MotorDriver`) can heap-allocate a derived controller and
clean it up without slicing.

### `virtual bool begin() = 0`

**Returns:** `true` when the driver was initialized successfully; `false`
otherwise (e.g. invalid/unallocated pins, PWM-capability check failure).

Initializes the physical layer: configures the driver's pins as `OUTPUT`,
drives any enable/standby line to the active state, and readies the chip to
receive `setMotorSpeeds()` commands. Must be called once after construction
before any speed command is issued.

### `virtual void setMotorSpeeds(int16_t left, int16_t right) = 0`

| Parameter | Type | Range | Meaning |
|---|---|---|---|
| `left`  | `int16_t` | `-255 .. 255` | Signed PWM duty for the **left** motor. Negative → reverse. |
| `right` | `int16_t` | `-255 .. 255` | Signed PWM duty for the **right** motor. Negative → reverse. |

**Returns:** nothing.

Applies the requested signed duty cycle to both motors atomically. The sign
encodes direction; the magnitude encodes speed. Implementations are expected
to clamp out-of-range values to the effective `[-255, 255]` band (see
`MotorUtils.h::clampPwm`). Calling before `begin()` returns is unspecified.

### Copy operations — deleted

```cpp
MotorController(const MotorController&)            = delete;
MotorController& operator=(const MotorController&) = delete;
```

`MotorController` is non-copyable. A controller instance owns the binding to
specific hardware pins, so copying would alias the same physical driver with
two C++ objects — which is unsafe. Move is not provided either; pass instances
by reference (as `DifferentialDriveController` does) or own them by pointer.

---

## Lifecycle

```
construct(pin args) ──► begin() ──► setMotorSpeeds(...) (repeat) ──► destroy (virtual dtor)
```

1. **Construct** a concrete subclass with its pin arguments.
2. **`begin()`** once to configure pins / enable the bridge; check the boolean return.
3. **`setMotorSpeeds(left, right)`** as often as required to drive the robot.
4. The owning scope destroys the instance; the virtual destructor releases resources.

---

## Speed semantics

The interface deals only in **signed PWM units**, not physical velocity. The
sign gives direction relative to the motor's wiring:

| Value | Direction |
|---|---|
| `value > 0`  | Forward |
| `value < 0`  | Reverse |
| `value == 0` | Coast / stop (implementation-defined) |

Mapping to real-world units (m/s, rad/s) happens at a higher level —
`DifferentialDriveController` converts a `Twist` (linearX / angularZ) into left
and right PWM values using wheel geometry, then forwards them to the
`MotorController&` it holds.

---

## Implementations

`MotorController` is the abstract base; shipping public subclasses:

| Class | Header | Bridge | Outline |
|---|---|---|---|
| `DRV8833MotorController` | `src/motor/DRV8833MotorController.h` | DRV8833 (dual-PWM H-bridge) | 4 pins: `leftIn1/In2`, `rightIn1/In2`. `begin()` sets all four `OUTPUT`. `setMotorSpeeds()` drives each pair with complementary PWM via `writeHBridgePwm()`. |
| `TB6612MotorController` | `src/motor/TB6612MotorController.h` | TB6612 (DIR + PWM + STBY) | 7 pins: `stby`, `aIn1/aIn2/aPwm`, `bIn1/bIn2/bPwm`. `begin()` sets `OUTPUT` and asserts `stby` HIGH (out of standby). `setMotorSpeeds()` drives each channel via `writeTb6612Channel()`. |

Both override `begin()` and `setMotorSpeeds()` per the contract above, and both
rely on the free helpers in `MotorUtils.h` (`writeHBridgePwm`,
`writeTb6612Channel`, `clampPwm`) to convert a signed value into the
hardware-specific pin levels.

> Only the `MotorController` contract is documented here. The concrete
> subclasses are summarized because the base header describes them as its
> intended implementations; document them separately if full reference is
> needed.

---

## How it is consumed

`MotorController` is meant to be used through its abstract interface:

- **`DifferentialDriveController`** holds a `MotorController&` and calls
  `setMotorSpeeds(pwmLeft, pwmRight)` from its `update()` step.
- **`MotorDriver`** owns a concrete backend (`TB6612MotorController` or
  `DRV8833MotorController`) by value, selected at compile time, and forwards
  `setMotorSpeeds()` in Direct mode or feeds a `DifferentialDriveController`
  in Differential mode.

---

## Platform notes

- `begin()` calls `pinMode(..., OUTPUT)` and (TB6612) `digitalWrite(stby, HIGH)`,
  so it requires the Arduino core (`<Arduino.h>`, included by the header).
- PWM feasibility of the chosen pins is the caller's responsibility at this
  layer; `MotorDriver` performs `isPwmPin()` validation before constructing a
  concrete controller.
- Effective PWM range is clamped to `[-255, 255]` by `src/motor/MotorUtils.h::clampPwm`.
- Async/non-blocking behavior is out of scope for this interface — `setMotorSpeeds()`
  returns synchronous I/O to the hardware registers / Arduino `analogWrite()`.

---

## File map

| File | Role for `MotorController` |
|---|---|
| `src/motor/MotorController.h` | Declares the abstract interface (this document). |
| `src/motor/MotorTypes.h` | `MotorPins`, `ControlMode`, `DriverType` types used by the motor subsystem. |
| `src/motor/DRV8833MotorController.h` / `.cpp` | Public DRV8833 implementation. |
| `src/motor/TB6612MotorController.h` / `.cpp` | Public TB6612 implementation. |
| `src/motor/MotorUtils.h` | Free helpers (`writeHBridgePwm`, `writeTb6612Channel`, `clampPwm`) used by the implementations. |
| `src/motor/DifferentialDriveController.h` / `.cpp` | Consumes a `MotorController&` for kinematic control. |
| `src/MotorDriver.h` / `.cpp` | Owns a concrete `MotorController` by value and selects the backend at compile time. |
| `src/core/Types.h` | `Twist` (linearX / angularZ) — the higher-level input converted down to PWM. |