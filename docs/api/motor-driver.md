# `ByByte::MotorDriver`

> Self-contained motor module.
>
> **Header:** `src/MotorDriver.h`
> **Namespace:** `ByByte`
> **Kind:** Concrete class

`MotorDriver` is the primary public API for brushed DC motor control. It
auto-selects the correct driver chip — TB6612 on Mega, DRV8833 on Nano /
other platforms — at compile time and owns the backend by value (no heap
allocation). Pulling in `<MotorDriver.h>` does **not** bring any other
ByByte module (Bluetooth, sensors, timers, PCINT, …), keeping a project
free of unrelated ISR / hardware conflicts.

The driver offers two modes that share a single instance:

| Mode | Entry point | Behaviour |
|---|---|---|
| **Direct** | `setMotorSpeeds(left, right)` | Per-wheel signed PWM `-255..255`. Switches the instance to Direct mode. |
| **Differential** | `setTargetVelocity(…)` + `update()` | Twist kinematics (linearX / angularZ). Switches to Differential mode; `update()` applies the kinematic math and forwards PWM to the backend. |

Convenience moves (`forward`, `backward`, `left`, `right`, `turnLeft`,
`turnRight`, `stop`) all operate in Direct mode and serve as readable
shortcuts for common one-shot commands.

---

## Public interface

```cpp
namespace ByByte {

enum class DriverType  { DRV8833, TB6612 };
enum class ControlMode { Direct, Differential };

struct Twist {
    float linearX;
    float angularZ;
};

struct MotorPins {
    uint8_t leftIn1  = 0, leftIn2  = 0;
    uint8_t rightIn1 = 0, rightIn2 = 0;
    uint8_t leftPwm  = 0, rightPwm = 0, stby = 0;

    static MotorPins drv8833(uint8_t l1, uint8_t l2,
                             uint8_t r1, uint8_t r2);

    static MotorPins tb6612(uint8_t lIn1, uint8_t lIn2, uint8_t lPwm,
                            uint8_t rIn1, uint8_t rIn2, uint8_t rPwm,
                            uint8_t standby);
};

class MotorDriver {
public:
    explicit MotorDriver(ControlMode mode = ControlMode::Direct);
    MotorDriver(const MotorPins& pins, ControlMode mode = ControlMode::Direct);
    ~MotorDriver() = default;

    MotorDriver(const MotorDriver&)            = delete;
    MotorDriver& operator=(const MotorDriver&) = delete;

    DriverType driverType() const noexcept;
    bool begin();

    void setMotorSpeeds(int16_t left, int16_t right);
    void setTargetVelocity(const Twist& cmd);
    void setTargetVelocity(float linearX, float angularZ);
    void update();

    void forward(int16_t speed = 100);
    void backward(int16_t speed = 100);
    void left(int16_t speed = 100);
    void right(int16_t speed = 100);
    void turnLeft(int16_t speed = 100);
    void turnRight(int16_t speed = 100);
    void stop();
};

} // namespace ByByte
```

---

### Supporting types

#### `Twist`

| Field | Type | Meaning |
|---|---|---|
| `linearX`  | `float` | Linear velocity (m/s or arbitrary units). |
| `angularZ` | `float` | Angular velocity (rad/s or arbitrary units). |

Defined in `src/core/Types.h`. Used as the input to `setTargetVelocity()` in
Differential mode.

#### `MotorPins`

Union pin-bundle covering both driver families.

| Field | Driver | Meaning |
|---|---|---|
| `leftIn1`, `leftIn2`   | Both  | Left motor direction / PWM pins. |
| `rightIn1`, `rightIn2` | Both  | Right motor direction / PWM pins. |
| `leftPwm`, `rightPwm`  | TB6612 only | Dedicated PWM pin per channel. |
| `stby`                 | TB6612 only | Standby line (active HIGH). |

Factory static methods:

| Method | Parameters | Returns |
|---|---|---|
| `drv8833(l1,l2,r1,r2)`      | Four `uint8_t` in-pins | `MotorPins` with DRV8833 pinout. |
| `tb6612(lIn1,lIn2,lPwm, rIn1,rIn2,rPwm, standby)` | Seven `uint8_t` pins | `MotorPins` with TB6612 pinout. |

> `MotorPins` is defined in `src/motor/MotorTypes.h`.

#### `ControlMode`

| Value | Meaning |
|---|---|
| `Direct`       | `setMotorSpeeds()` drives per-wheel PWM; `update()` is a no-op. |
| `Differential` | `setTargetVelocity(…)` records a Twist; `update()` runs kinematics. |

#### `DriverType`

| Value | Meaning |
|---|---|
| `DRV8833` | DRV8833 dual-PWM H-bridge (Nano / unknown platform). |
| `TB6612`  | TB6612 DIR + PWM + STBY bridge (Mega). |

Returned by `driverType()` for runtime introspection.

---

### `MotorDriver(ControlMode mode = ControlMode::Direct)`

Default constructor. Uses pin defaults from `ByByteConfig.h` for the active
platform (see [Configuration](#configuration)). The optional `mode` parameter
selects the initial control mode.

### `MotorDriver(const MotorPins& pins, ControlMode mode = ControlMode::Direct)`

Explicit-pins constructor. Any field left at `0` in `pins` is filled from the
platform defaults. Pin validation happens in `begin()`.

### `~MotorDriver() = default`

Default destructor. The backend lives by value; no manual cleanup is required.

### Copy operations — deleted

```cpp
MotorDriver(const MotorDriver&)            = delete;
MotorDriver& operator=(const MotorDriver&) = delete;
```

`MotorDriver` is non-copyable. Each instance owns the binding to specific
hardware pins; copying would alias the same physical driver with two C++
objects, which is unsafe.

---

### `DriverType driverType() const noexcept`

**Returns:** the `DriverType` enum value for the backend selected at compile
time (`DRV8833` or `TB6612`). Useful for runtime branching on
hardware-specific behaviour.

---

### `bool begin()`

**Returns:** `true` when all pins are valid and the driver was initialized
successfully; `false` otherwise.

Configures the driver pins as `OUTPUT`, validates that PWM-capable pins are
actually PWM-feasible via `isPwmPin()`, asserts standby HIGH (TB6612), and
readies the chip for speed commands. Must be called once after construction
and before any movement command.

---

### `void setMotorSpeeds(int16_t left, int16_t right)`

| Parameter | Type | Range | Meaning |
|---|---|---|---|
| `left`  | `int16_t` | `-255..255` | Signed PWM duty for the **left** motor. Negative → reverse. |
| `right` | `int16_t` | `-255..255` | Signed PWM duty for the **right** motor. Negative → reverse. |

**Returns:** nothing.

Switches the instance to **Direct** mode and immediately applies the
requested signed duty cycle to both motors. The sign encodes direction;
the magnitude encodes speed. Values are clamped to `[-255, 255]` (see
`MotorUtils.h::clampPwm`). Calling before `begin()` returns is unspecified.

---

### `void setTargetVelocity(const Twist& cmd)`
### `void setTargetVelocity(float linearX, float angularZ)`

| Overload | Parameters | Type |
|---|---|---|
| Struct form  | `cmd.linearX`, `cmd.angularZ` | `const Twist&` |
| Scalar form | `linearX`, `angularZ`         | `float, float` |

**Returns:** nothing.

Records a differential-drive target and switches the instance to
**Differential** mode. The actual PWM values are computed and applied on the
next call to `update()`.

---

### `void update()`

**Returns:** nothing.

When in **Differential** mode, computes left and right wheel velocities from
the stored `Twist` target (using wheel separation, wheel radius, and max PWM
from platform defaults) and forwards the resulting PWM values to the backend.
In Direct mode, this is a no-op — the last `setMotorSpeeds()` values persist.

---

### Convenience moves

All convenience methods switch to **Direct** mode and issue a single
`setMotorSpeeds()` call.

| Method | Effect | Default speed |
|---|---|---|
| `forward(speed)`  | Both wheels forward  | `100` |
| `backward(speed)` | Both wheels backward | `100` |
| `left(speed)`     | Left wheel backward, right forward (turn in place) | `100` |
| `right(speed)`    | Right wheel backward, left forward (turn in place) | `100` |
| `turnLeft(speed)` | Left turn (right wheel forward, left stationary) | `100` |
| `turnRight(speed)`| Right turn (left wheel forward, right stationary) | `100` |
| `stop()`          | Both wheels 0 (coast) | — |

`speed` is clamped to `[-255, 255]`.

---

## Configuration

Platform defaults are defined in `src/core/configs/ByByteConfig.h` and may be
overridden with preprocessor defines before including the header.

| Platform | Driver | Key pins |
|---|---|---|
| Nano | DRV8833 | Left: IN1=D10, IN2=D9; Right: IN1=D5, IN2=D6 |
| Mega | TB6612 | STBY=49, Left: IN1=47, IN2=48, PWM=46; Right: IN1=43, IN2=42, PWM=44 |

Geometry and PWM limits:

| Define | Nano | Mega | Meaning |
|---|---|---|---|
| `BYBYTE_MAX_PWM`            | `220`   | `255`   | Maximum PWM magnitude applied. |
| `BYBYTE_WHEEL_SEPARATION_M` | `0.11f` | `0.11f` | Distance between wheel centres (metres). |
| `BYBYTE_WHEEL_RADIUS_M`     | `0.045f`| `0.045f`| Wheel radius (metres). |

> Override any of these with `#define BYBYTE_MAX_PWM 200` (etc.) **before**
> `#include <MotorDriver.h>` to adjust limits without editing the library.

---

## Lifecycle

```
construct([pins], [mode]) ──► begin() ──► setMotorSpeeds / setTargetVelocity+update (repeat) ──► destroy
```

1. **Construct** with default pins or an explicit `MotorPins` bundle.
2. **`begin()`** once to configure pins and enable the bridge; check the boolean return.
3. **Command** the motors via `setMotorSpeeds()` (Direct) or
   `setTargetVelocity()` + `update()` (Differential). Convenience moves are
   available as Direct-mode shortcuts.
4. The owning scope destroys the instance; the backend (held by value) and
   `DifferentialDriveController` are cleaned up automatically.

---

## Usage notes

- **PWM range:** `[-255, 255]`. The effective maximum is clamped to
  `BYBYTE_MAX_PWM` (220 on Nano, 255 on Mega) in the kinematics layer.
  Negative values reverse direction.
- **Platform auto-detect:** `BYBYTE_PLATFORM_ID` (from `PlatformDetect.h`)
  selects `TB6612MotorController` on Mega and `DRV8833MotorController`
  otherwise. No user intervention required.
- **Pin validation:** `begin()` checks all PWM-designated pins with
  `isPwmPin()` (from `src/core/PinCapabilities.h`). Returns `false` if any
  required PWM pin lacks hardware PWM support.
- **Backend lives by value:** No heap allocation, no `new`/`delete`. The
  backend and `DifferentialDriveController` are value members.
- **Non-copyable:** `MotorDriver` instances cannot be copied or assigned. Pass
  by reference or own as a value member.
- **Pull isolation:** Including `<MotorDriver.h>` pulls only the motor
  subsystem and core types — no Bluetooth, sensors, or timer ISRs.
- **Non-blocking:** `setMotorSpeeds()` and `update()` return synchronously
  after writing to hardware registers / `analogWrite()`. No loop or delay
  inside the driver.

---

## File map

| File | Role for `MotorDriver` |
|---|---|
| `src/MotorDriver.h` | Declares the class (this document). |
| `src/MotorDriver.cpp` | Implements the class. |
| `src/motor/MotorTypes.h` | `MotorPins`, `ControlMode`, `DriverType` types. |
| `src/motor/MotorController.h` | Abstract backend interface. |
| `src/motor/DRV8833MotorController.h` / `.cpp` | DRV8833 concrete backend. |
| `src/motor/TB6612MotorController.h` / `.cpp` | TB6612 concrete backend. |
| `src/motor/DifferentialDriveController.h` / `.cpp` | Differential kinematics from Twist. |
| `src/motor/MotorUtils.h` | `clampPwm`, `writeHBridgePwm`, `writeTb6612Channel`. |
| `src/core/Types.h` | `Twist` struct. |
| `src/core/PinCapabilities.h` | `isPwmPin()` validation. |
| `src/core/configs/ByByteConfig.h` | Platform pin / geometry defaults. |
| `src/core/configs/PlatformDetect.h` | Platform auto-detection for backend selection. |

For custom motor backends, see the abstract [MotorController](motor-controller.md) interface.
