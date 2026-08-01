# `ByByte::Servo`

> Hobby-style RC servo driver wrapping `ServoManager`, supporting
> **angle**, **microsecond**, and **continuous-rotation** (360°) modes.
>
> **Header:** `src/Servo.h`
> **Namespace:** `ByByte`
> **Kind:** Concrete class

`ByByte::Servo` is a thin object-oriented façade over the static
`ServoManager` (which owns the Timer3 ISR and up to 3 hardware channels on
the Mega). It exposes an Arduino-`Servo`-like API — `attach`,
`write(degrees)`, `writeMicroseconds(us)`, `detach`, `attached` — plus an
extra **continuous-rotation** mode selected by `setContinuous()` and driven
through `writeSpeed(percent)`.

> ## Platform availability
>
> `Servo` is functional **only on the ByByte Mega** (`BYBYTE_PLATFORM_ID ==
> BYBYTE_PLATFORM_MEGA`). On the Nano and on unknown platforms, every public
> method either **returns a failure / no-op** and discards its arguments, so
> the class can still be *declared* on any target without compilation errors:
>
> | Method | Mega (active) | Nano / Unknown (no-op) |
> |---|---|---|
> | `attach(...)` | allocates a Timer3 channel from `ServoManager`, returns `true`/`false`. | Returns `false`; arguments discarded. |
> | `detach()` | releases the channel. | No-op. |
> | `write(angle)` | angle or (in continuous mode) speed. | No-op; `angle` discarded. |
> | `writeMicroseconds(us)` | raw µs. | No-op; `us` discarded. |
> | `attached() const` | `true` if a channel is allocated. | Always `false`. |
> | `setContinuous(...)` | stores continuous config. | Stores the config (state only — no hardware). |
> | `writeSpeed(percent)` | commands speed via µs. | No-op; `percent` discarded. |
>
> RC timing on the Mega is produced by `ServoManager` on **Timer3**
> (CTC, 20 kHz tick ⇒ 50 µs per tick, 50 Hz / 20 ms frame), so up to **3
> servo channels** can run concurrently. Channel pins are interpreted by an
> internal port-pin map; the supported servo pins are **D30, D31, D32** (all
> on `PORTC`: PC7 / PC6 / PC5).

---

## Synopsis

```cpp
namespace ByByte {

class Servo {
public:
    Servo();

    bool    attach(uint8_t pin,
                   uint16_t minUs = 500,
                   uint16_t maxUs = 2500,
                   uint8_t  frameHz = 50);
    void    detach();
    void    write(uint8_t angle);            // 0..180 (standard) or speed shortcut (continuous)
    void    writeMicroseconds(uint16_t us);  // raw pulse width in µs
    bool    attached() const;

    // Continuous (360°) mode support
    void    setContinuous(bool enable,
                         uint16_t stopUs = 1500,
                         uint16_t deadbandUs = 20);
    void    writeSpeed(int8_t speedPercent); // -100..100
};

} // namespace ByByte
```

---

## Configuration

`Servo` itself takes **no pin default** — the pin is a required argument to
`attach()`. On the Mega, `ServoManager`'s three channels are pre-seeded with
the `BYBYTE_SERVO0_PIN` / `BYBYTE_SERVO1_PIN` / `BYBYTE_SERVO2_PIN` macros
from `configs/ByByteConfig.h`:

| Build target | Macro | Default pin | Port bit |
|---|---|---|---|
| Mega (`BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA`) | `BYBYTE_SERVO0_PIN` | `30` | PC7 |
| Mega | `BYBYTE_SERVO1_PIN` | `31` | PC6 |
| Mega | `BYBYTE_SERVO2_PIN` | `32` | PC5 |

The defaults are overridable with `#define` before including the header.
**Only pins `30`, `31`, `32` are recognized** by `ServoManager`'s internal
port-pin map; other pins will cause `attach()` to return `false`.

`frameHz` accepts **50** (20 ms frame) or **25** (40 ms frame); any other
value falls back to 50.

---

## Public interface

### `Servo()`

Construct a detached servo with continuous-rotation defaults.

| Field | Initial value |
|---|---|
| `_channel` | `-1` (not attached) |
| `_continuous` | `false` (standard angle mode) |
| `_stopUs` | `1500` µs (continuous-rotation center) |
| `_deadbandUs` | `20` µs |
| `_minUs` / `_maxUs` | `500` / `2500` µs (overridden on `attach()`) |

Non-blocking; performs no I/O.

### `bool attach(uint8_t pin, uint16_t minUs = 500, uint16_t maxUs = 2500, uint8_t frameHz = 50)`

Attach this servo to a hardware channel and begin generating pulses.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `pin`     | `uint8_t`  | — *(required)* | Servo signal pin. Mega: must be **30, 31, or 32**. |
| `minUs`   | `uint16_t` | `500`  | Minimum pulse width in µs (0°). `ServoManager` clamps to `≥ 400`. |
| `maxUs`   | `uint16_t` | `2500` | Maximum pulse width in µs (180°). `ServoManager` clamps to `≤ 2600`. |
| `frameHz` | `uint8_t`  | `50`   | Frame rate: `50` (20 ms) or `25` (40 ms). Other values ⇒ `50`. |

**Returns:** `true` if a channel was allocated and the pin is recognized;
`false` on the Nano/unknown platforms, if all 3 channels are in use, if the
channel is already enabled, or if the pin is not in the `pinToPort()` map.

Internally (Mega only):
1. Stores `minUs`/`maxUs` on this instance.
2. Calls `ServoManager::begin(frameHz)` (idempotent — also sets the frame rate).
3. Tries channels `0..2`; on the first free, enabled channel, calls
   `ServoManager::attach(channel, pin, minUs, maxUs)`, records the channel
   and pin, and returns `true`.
4. `ServoManager::attach()` configures the pin as `OUTPUT` and starts the
   Timer3 ISR if it isn't already running.

> **Channel reuse:** `ServoManager::attach()` refuses to re-attach an already
> enabled channel, so calling `attach()` twice on the same `Servo` without an
> intervening `detach()` will skip the in-use channel and try the others.

### `void detach()`

Release this servo's hardware channel.

- Mega: calls `ServoManager::detach(channel)` (marks the channel
  `enabled=false`) and resets `_channel = -1`.
- Nano/unknown: no-op.

Does not stop the Timer3 ISR (`ServoManager` keeps the timer running so other
servos can keep generating pulses).

**Returns:** nothing.

### `void write(uint8_t angle)`

Command the servo. Behavior depends on the continuous-rotation flag:

| Mode | `angle` range | Meaning |
|---|---|---|
| **Standard** (`setContinuous(false)`, default) | `0 .. 180` | Target angle via `ServoManager::writeAngle()`. Values `> 180` are internally clamped to `180`. |
| **Continuous** (`setContinuous(true)`) | `0 .. 180` | Speed shortcut: `(angle - 90) * 100 / 90` ⇒ `-100 .. +100`, then `writeSpeed()`. `90` ⇒ stop, `0` ⇒ full reverse, `180` ⇒ full forward. |

No-op when `attached()` is `false` or on the Nano/unknown platforms.

**Returns:** nothing.

### `void writeMicroseconds(uint16_t us)`

Command the servo with a raw pulse width.

| Parameter | Type | Unit | Meaning |
|---|---|---|---|
| `us` | `uint16_t` | µs | Target pulse width. `ServoManager` clamps it to the channel's `[minUs, maxUs]`. |

Use this for precise pulse control (e.g. ESC calibration), or when your
servo's angle-to-µs mapping isn't 0..180. No-op when not attached or on
non-Mega targets.

**Returns:** nothing.

### `bool attached() const`

| Return | Meaning |
|---|---|
| `true`  | A hardware channel is currently allocated (`_channel >= 0`). |
| `false` | Detached (or never attached, or on a platform where `attach()` is a no-op). |

`const`-qualified; safe to call from a hot loop. On the Nano / unknown
platforms it is always `false`.

### `void setContinuous(bool enable, uint16_t stopUs = 1500, uint16_t deadbandUs = 20)`

Switch between **standard angle** and **continuous-rotation (360°)** modes.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `enable`      | `bool`     | — *(required)* | `true` ⇒ continuous mode; `false` ⇒ standard angle mode. |
| `stopUs`      | `uint16_t` | `1500` | Pulse width (µs) that stops a continuous servo. |
| `deadbandUs`  | `uint16_t` | `20`   | Half-band around `stopUs` within which `writeSpeed()` treats values as "stop". Prevents drift at near-zero speeds. |

This **only** records the configuration on this instance — it does not touch
the hardware or affect any pending `write()`. It works on every platform
(stores state only), even though the actual pulses only generate on the Mega.

**Returns:** nothing.

### `void writeSpeed(int8_t speedPercent)`

Command a **continuous-rotation** servo's speed (or, in standard mode, treat
the value as a raw position across the `[minUs, maxUs]` span).

| Parameter | Type | Range | Meaning |
|---|---|---|---|
| `speedPercent` | `int8_t` | `-100 .. +100` | Signed speed in percent. Clamped to the `[-100, +100]` band. |

**Returns:** nothing. No-op when not attached or on non-Mega platforms.

Behavior depends on the continuous flag:

| Mode | `speedPercent` | Resulting pulse |
|---|---|---|
| **Standard** (`!_continuous`) | `-100 .. +100` | Linearly mapped to `_minUs .. _maxUs`:
  `us = _minUs + (_maxUs - _minUs) * (speedPercent + 100) / 200` (so `-100 ⇒ _minUs`, `0 ⇒ midpoint`, `+100 ⇒ _maxUs`). |
| **Continuous** (default-off region) | `-2..+2` (inside the deadband around `stopUs`) | `stopUs` (stopped). |
| **Continuous**, `≥ 2` (forward) | `+2 .. +100` | `stopUs + deadbandUs .. _maxUs`, linearly. |
| **Continuous**, `≤ -2` (reverse) | `-100 .. -2` | `_minUs .. (stopUs - deadbandUs)`, linearly. |

The continuous-mode mapping keeps a `deadbandUs`-wide dead zone around
`stopUs` so that small joystick drift near the center reliably stops the
servo rather than crawling.

---

## Timing model (Mega)

`ServoManager` owns the hardware:

- **Timer3 CTC**, prescaler 8 (`CS31`) on a 16 MHz Mega ⇒ 2 MHz timer clock.
- `OCR3A = 99` → compare every 100 ticks ⇒ **20 kHz tick** (50 µs per tick).
- Frame length `_frameTicks = frameUs / 50µs`:
  - 50 Hz ⇒ 20 000 µs / 50 µs = **400 ticks**
  - 25 Hz ⇒ 40 000 µs / 50 µs = **800 ticks** (clamped ≥ 300)
- `ISR(TIMER3_COMPA_vect)` → `ServoManager::isrTick()`:
  - At `_tick == 0`, asserts every enabled channel's pin.
  - When `_tick == channel.pulseTicks`, clears that channel's pin.
  - Wraps `_tick` at `_frameTicks`.
- `writeMicroseconds(us)` stores `pulseTicks = us / 50`, so resolution is
  **50 µs** (a single `us` increment has no effect; steps of 50 µs do).

---

## Lifecycle

```
construct() ──► attach(pin, minUs, maxUs, frameHz) ──► write(...) / writeMicroseconds(...) / writeSpeed(...) (repeat)
                       │                                       │
                       └──► (optional) setContinuous(true, ...) ──► writeSpeed(...) / write(angle as speed)
                       │
                       └──► detach() (optional) ──► attach(...) again is possible
```

1. **Construct** a `Servo` (detached, standard mode, 500..2500 µs).
2. **`attach(pin, ...)`** once — allocates a Timer3 channel on the Mega.
3. **Command** — call `write(angle)`, `writeMicroseconds(us)`, or
   (after `setContinuous(true)`) `writeSpeed(percent)`.
4. **`detach()`** to free the channel when done; other `Servo` instances keep
   running. Re-`attach()` afterward is allowed.
5. Held by value; no cleanup needed (though `detach()` before destruction
   frees the shared channel for other servos).

---

## Usage notes

- **Mega-only hardware:** all pulse generation runs on `ServoManager` /
  Timer3 and exists only for `BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA`.
  On the Nano and unknown platforms, `attach()` returns `false` and the
  command methods are no-ops; `attached()` is always `false`.
- **Supported pins:** only **D30, D31, D32** are wired in `ServoManager`'s
  port-pin map. Other pins cause `attach()` to return `false` even on the
  Mega. (`BYBYTE_SERVO0/1/2_PIN` default to these.)
- **Max 3 channels:** `ServoManager` has 3 channel slots. A 4th `attach()`
  (across any `Servo` instances) returns `false`.
- **Channel sharing:** a channel that is already `enabled` cannot be
  re-attached; call `detach()` on the owning `Servo` first.
- **Pulse resolution:** the manager's tick is 50 µs, so `writeMicroseconds()`
  is effectively quantized to multiples of 50 µs. Microsecond values finer
  than 50 µs are not represented.
- **Continuous-mode deadband:** small speeds within `±2 %` (or within
  `deadbandUs` of `stopUs`) are forced to `stopUs` to avoid drift. Tune
  `deadbandUs` if your servo creeps slightly off-center.
- **`write()` in continuous mode** is a convenience shortcut that maps
  `0..180 → -100..+100`; for finer control use `writeSpeed(int8_t)` directly.
- **Timer footprint:** `ServoManager` uses **Timer3** and the `TIMER3_COMPA`
  ISR. If your sketch (or another library) also reconfigures Timer3, the
  servo pulses will be disrupted.
- **No public accessors** for `_channel`, `_continuous`, `_stopUs`,
  `_deadbandUs`, `_minUs`, `_maxUs`; these are private. Query attachment via
  `attached()` only.
- **`ServoManager` is a separate static class** and is the real driver; this
  class is its object-oriented wrapper. `ServoManager` is intentionally
  Mega-guarded in its own header.

---

## File map

| File | Role for `Servo` |
|---|---|
| `src/Servo.h` | Declares the `Servo` class (this document). |
| `src/Servo.cpp` | Implements `Servo` by delegating to `ServoManager` on the Mega; no-ops elsewhere. |
| `src/ServoManager.h` | Declares `ServoManager` (static, Mega-guarded): the Timer3 driver behind `Servo`. |
| `src/ServoManager.cpp` | Implements `ServoManager` + `ISR(TIMER3_COMPA_vect)`. |
| `src/configs/PlatformDetect.h` | Defines `BYBYTE_PLATFORM_ID` / `BYBYTE_PLATFORM_MEGA`, which gate every method. |
| `src/configs/ByByteConfig.h` | Provides `BYBYTE_SERVO0/1/2_PIN` defaults (`30`, `31`, `32`) on the Mega. |