# `ByByte::Sonar`

> Async, non-blocking ultrasonic distance sensor (HC-SR04-style) using a TRIG
> output pulse and an ECHO input, driven by `TimerManager` (and PCINT on the
> Nano).
>
> **Header:** `src/Sonar.h`
> **Namespace:** `ByByte`
> **Kind:** Concrete class (header-only, all methods inline)

`Sonar` measures distance by issuing a periodic 10 µs trigger pulse and
timing the ECHO return. It is **asynchronous**: after `begin()`, the
`TimerManager` 1 ms scheduler fires the trigger every ~100 ms, and the ECHO
edge timestamp (captured via `micros()` — through a PCINT callback on the
Nano) is converted to centimeters. The latest result is read out
non-blockingly via `readCm()`.

Distances are produced with the standard `cm = echo_us / 58` approximation
(sound speed). `0` is used as the **out-of-range / timeout** sentinel.

> **Single-instance design:** `Sonar` uses a static self-pointer
> (`instance()`) to route the TimerManager/PCINT static callbacks to the
> active object. Only one `Sonar` is wired to the timers at a time. For
> higher-precision, multi-instance sonar, see `SonarPrecise` (separate class).

---

## Synopsis

```cpp
namespace ByByte {

class Sonar {
public:
    Sonar(uint8_t trigPin, uint8_t echoPin, uint16_t maxRangeCm = 400);

    void begin();
    void end();

    uint16_t readCm() const;   // last measured distance; 0 = out-of-range/timeout

    static Sonar* create(uint8_t trigPin, uint8_t echoPin, uint16_t maxRangeCm = 400);
};

} // namespace ByByte
```

---

## Public interface

### `Sonar(uint8_t trigPin, uint8_t echoPin, uint16_t maxRangeCm = 400)`

Construct a sensor bound to `trigPin` (output) and `echoPin` (input).

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `trigPin`    | `uint8_t`  | — *(required)* | Output pin driving the sensor's TRIG line. |
| `echoPin`    | `uint8_t`  | — *(required)* | Input pin reading the sensor's ECHO line. |
| `maxRangeCm` | `uint16_t` | `400` | Maximum believable distance in cm. Readings above this are reported as `0` (out-of-range). |

The constructor is non-blocking and performs no I/O; it only stores the pins
and initializes state to `Idle` / `_lastCm = 0`.

### `void begin()`

Configures the pins and registers the sensor with the platform schedulers.

Internally:

1. `pinMode(trigPin, OUTPUT)` / `pinMode(echoPin, INPUT)`; drives TRIG `LOW`.
2. Registers `this` as the active static instance (`instance(this)`), so the
   TimerManager/PCINT static callbacks route to this object.
3. Subscribes to the shared schedulers per platform:

| Platform | Trigger source | Echo edge source |
|---|---|---|
| **Mega** (`BYBYTE_PLATFORM_ID == MEGA`) | `TimerManager` `MILLISECOND_1` subscription (`onTickStatic`, not immediate) | *(polling intended; see note below)* |
| **Nano** (`BYBYTE_PLATFORM_ID == NANO`) | `TimerManager` `MILLISECOND_1` subscription (`onTickStatic`, not immediate) | `PcintManager::subscribe(echoPin, onPcintStatic)` — echo edge detection via Pin Change Interrupt |

> **Echo capture path:** On the Nano, ECHO rising/falling edges are captured
> asynchronously via PCINT, yielding accurate `micros()` timestamps at the
> edge. On the Mega, PCINT dispatch is available but the implementation keeps
> the same `onTick()` 1 ms slot active without a separate echo-edge handler
> registered here — i.e. the Mega relies on the shared tick. `SonarPrecise`
> provides higher-precision 1 µs + edge-detection on the Mega if needed.

**Returns:** nothing. Call exactly once after construction.

### `void end()`

Releases the scheduler subscriptions; symmetric counterpart to `begin()`.

| Platform | What it unsubscribes |
|---|---|
| **Mega** | `TimerManager::unsubscribe(&Sonar::onTickStatic)` |
| **Nano** | `TimerManager::unsubscribe(&Sonar::onTickStatic)` **and** `PcintManager::unsubscribe(echoPin)` |

`end()` does **not** reset the static `instance()` pointer — the next
`begin()` re-registers. It also does not change pin modes back.

**Returns:** nothing.

### `uint16_t readCm() const`

| Return | Unit | Meaning |
|---|---|---|
| `uint16_t` | cm | Last measured distance. **`0` ⇒ out-of-range or timeout.** |

Returns the most recent asynchronous measurement. Non-blocking, ISR-safe
enough for poll-from-`loop()` reads (the value is `volatile`). The value
updates roughly every 100 ms as the TimerManager 1 ms slot fires `onTick()`.

Conversion:

```
cm = echo_duration_us / 58        // standard HC-SR04 approximation
cm > maxRangeCm ? 0 : cm         // clamp out-of-range to 0
```

`const`-qualified; safe to call from a hot loop.

### `static Sonar* create(uint8_t trigPin, uint8_t echoPin, uint16_t maxRangeCm = 400)`

**Factory.** Heap-allocates a `Sonar`, registers it as the active static
instance, and returns it. Equivalent to:

```cpp
Sonar* s = new Sonar(trigPin, echoPin, maxRangeCm);
// instance(s) already registered by create()
```

Note: `create()` registers the instance via `instance(s)` but still requires
an explicit `begin()` call on the returned object to configure the pins and
subscribe to the timers. Ownership of the returned `Sonar*` belongs to the
caller; there is no `destroy()` counterpart — `delete` it (after `end()`)
when finished.

---

## Timing model

```
begin() ──► (every ~100 ms, in TimerManager 1ms slot)
            onTick() : if Idle & ≥100ms since last ping → 10µs TRIG pulse, state=WaitHigh
            ──► (Nano: ECHO edge via PCINT)
               onPcint() :
                 WaitHigh + HIGH → record _riseUs, state=Measuring
                 Measuring + LOW  → cm = (nowUs - _riseUs)/58; clamp to maxRangeCm or 0; state=Idle
            ──► readCm() returns _lastCm at any time
```

- **Trigger cadence:** an ECHO capture is initiated at most once per ~100 ms
  (`nowMs - _lastPingMs >= 100`). Both `Idle` and the 100 ms gate must hold.
- **Trigger pulse width:** 10 µs (HC-SR04 spec).
- **Distance formula:** `cm = echo_us / 58` — the classic approximation
  (speed of sound ≈ 340 m/s, with the round-trip factor).
- **Range clamp:** any reading greater than `maxRangeCm` is reported as `0`,
  the out-of-range sentinel.

---

## Platform notes

- **Nano (`BYBYTE_PLATFORM_NANO`)** — full async operation: TRIG from the
  1 ms TimerManager slot (lightweight), ECHO edges via `PcintManager` on the
  chosen `echoPin`. The `echoPin` must be a **PCINT-capable** pin for the
  on-edge callback to fire; `PcintManager` maps Arduino pin → PCINT number via
  its internal table (Digital 8 → PCINT0 on ATmega328P/168, etc.).
- **Mega (`BYBYTE_PLATFORM_MEGA`)** — TRIG from the 1 ms TimerManager slot;
  PCINT **not** subscribed in `begin()` (the design avoids Timer1 ISR
  conflicts). ECHO timing is therefore driven by the same 1 ms tick rather
  than edge-precise capture; for sub-ms precision on the Mega, use
  `SonarPrecise` (1 µs slot + edge detection), which is a separate class and
  supports up to 4 instances.
- **Unknown platform** — `begin()` / `end()` perform pin setup and
  registration but subscribe to **no** timers (both subscription blocks are
  platform-guarded), so the sensor will not produce autonomous readings. Wire
  it manually or pick a known platform.
- `Sonar::State` (`Idle`, `WaitHigh`, `Measuring`) is an **internal private
  enum** — not part of the public API.

---

## Lifecycle

```
construct(trig, echo, maxRangeCm) ──► begin() ─► readCm() (poll, repeat) ──► end() (optional) ──► destroy
```

1. **Construct** with TRIG/ECHO pin numbers and an optional max range.
2. **`begin()`** once — sets pin modes, registers the static self-pointer,
   subscribes to `TimerManager` (Nano + Mega) and `PcintManager` (Nano).
3. **Poll** `readCm()` in `loop()` — it updates every ~100 ms autonomously.
4. **`end()`** to unsubscribe from the timers (e.g. before sleeping, or when
   switching sensors); safe to `begin()` again afterward.
5. If obtained via `create()`, the caller owns the pointer — `end()` then
   `delete` when done.

---

## Usage notes

- **Non-blocking:** `begin()` starts autonomous ranging; `readCm()` returns
  the latest result immediately. There is no `ping()`-style blocking call.
- **Sentinel:** treat `0` as *no reading* (out of range, timeout, or before
  the first measurement completes). Do not interpret a literal 0 cm.
- **Single active instance:** the static `instance()` pointer means at most
  one `Sonar` at a time is wired to the TimerManager/PCINT callbacks. For
  multiple sonars, prefer `SonarPrecise` (up to 4 instances) or rotate by
  `end()` / `begin()` between sensors.
- **Nano echo pin must be PCINT-capable:** `PcintManager::subscribe()` is
  called with the Arduino `echoPin` and requires its underlying PCINT number
  to exist in the manager's pin map.
- **Mega precision:** ECHO is not edge-captured here; the 1 ms tick limits
  resolution. Use `SonarPrecise` for 1 µs edge-precise ranging on the Mega.
- **ISR context:** `onTick()`/`onPcint()` run from ISR/TimerManager context
  (callbacks marked `immediately=false`, so 1 ms slot is dispatched from the
  timer ISR). Keep `readCm()` reads tolerant of the value updating between
  samples.
- **Does not clean pin modes / static instance:** `end()` unsubscribes only.
- **No default ctor / no pin defaults:** TRIG and ECHO are required at
  construction (no `BYBYTE_*_PIN` defaults are used for sonar pins).

---

## File map

| File | Role for `Sonar` |
|---|---|
| `src/Sonar.h` | Defines the class (header-only, all inline). |
| `src/TimerManager.h` | Provides the 1 ms scheduler slot (`TimerInterval::MILLISECOND_1`) that drives the periodic trigger. |
| `src/PcintManager.h` | Provides Pin Change Interrupt echo-edge subscription on the Nano. |
| `src/configs/PlatformDetect.h` | Defines `BYBYTE_PLATFORM_ID` / `BYBYTE_PLATFORM_NANO` / `BYBYTE_PLATFORM_MEGA`, branched on in `begin()` / `end()`. |
| `src/SonarPrecise.h` | Separate higher-precision, multi-instance alternative (up to 4 sonars, 1 µs slot). |