# `ByByte::SonarPrecise`

> High-precision, multi-instance ultrasonic distance sensor (HC-SR04-style)
> using a 1 µs `TimerManager` slot for edge-precise echo capture, with
> median filtering for stability. Supports up to 4 concurrent instances.
>
> **Header:** `src/SonarPrecise.h`
> **Namespace:** `ByByte`
> **Kind:** Concrete class

`SonarPrecise` measures distance by issuing a 10 µs trigger pulse and timing
the ECHO return with **microsecond** resolution via the `TimerManager`
1 µs slot. Unlike the simpler [`Sonar`](sonar.md) — which runs on a 1 ms
tick and supports only one active instance — `SonarPrecise` polls echo edges
at 1 µs granularity and multiplexes up to **4 independent sensors** through
a static instance array.

Each sensor is **non-blocking**: after `begin()`, the `TimerManager` drives
trigger pulses every ~50 ms and captures ECHO rising/falling edges
asynchronously. The latest distance is read out via `readCm()`, and a
median-of-3 filter (`getFilteredDistance()`) suppresses transient outliers.

Distances use the standard `cm = echo_us / 58` approximation. A minimum
threshold of **2 cm** filters near-field noise; readings exceeding
`maxDistanceCm` are reported as `0` (out-of-range / timeout sentinel).

---

## Synopsis

```cpp
namespace ByByte {

class SonarPrecise {
public:
    SonarPrecise(uint8_t trigPin, uint8_t echoPin, uint16_t maxDistanceCm = 400);

    bool begin();
    void end();

    uint16_t readCm();
    uint16_t getFilteredDistance();

    uint8_t  getState() const;
    uint32_t getLastTriggerUs() const;

    static void on1usTick();
    static void on1msTick();

    static SonarPrecise* create(uint8_t trigPin, uint8_t echoPin, uint16_t maxDistanceCm);
};

} // namespace ByByte
```

---

## Configuration

No default pins are provided — `trigPin` and `echoPin` are **required** at
construction.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `trigPin`        | `uint8_t`  | — *(required)* | Output pin driving the sensor's TRIG line. |
| `echoPin`        | `uint8_t`  | — *(required)* | Input pin reading the sensor's ECHO line. |
| `maxDistanceCm`  | `uint16_t` | `400` | Maximum believable distance in cm. Readings above this are reported as `0` (out-of-range). Also sets the internal echo timeout: `maxTimeoutUs = maxDistanceCm × 58`. |

The constructor is non-blocking and performs no I/O; it only stores the pins,
initializes state to `IDLE`, zeroes the filter buffer, and computes
`_maxTimeoutUs`.

---

## Public interface

### `SonarPrecise(uint8_t trigPin, uint8_t echoPin, uint16_t maxDistanceCm = 400)`

Construct a sensor bound to `trigPin` (output) and `echoPin` (input).

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `trigPin`        | `uint8_t`  | — *(required)* | Output pin driving the sensor's TRIG line. |
| `echoPin`        | `uint8_t`  | — *(required)* | Input pin reading the sensor's ECHO line. |
| `maxDistanceCm`  | `uint16_t` | `400` | Maximum believable distance in cm. Internally computes `_maxTimeoutUs = maxDistanceCm × 58`; if an echo does not return within this window the reading is abandoned as `0`. |

The constructor stores all parameters, sets `_state = IDLE`, and zeroes the
5-element ring buffer used by the median filter.

### `bool begin()`

Configures the pins, registers this instance in the static multi-instance
array, and subscribes to the `TimerManager` schedulers.

**Returns:** `true` on success; `false` if 4 instances are already registered
(the static array is full).

Internally:

1. Checks `_instanceCount < 4` — returns `false` if the array is saturated.
2. `pinMode(trigPin, OUTPUT)` / `pinMode(echoPin, INPUT)`; drives TRIG `LOW`.
3. Registers `this` in the first available slot of the static
   `_sonarInstances[4]` array and increments `_instanceCount`.
4. Subscribes to two `TimerManager` slots:

| Slot | Handler | `immediately` | Purpose |
|---|---|---|---|
| `MICROSECOND_1` | `timer1usHandler` → `on1usTick()` | `true` | Edge-precise echo detection and timeout monitoring for all registered instances. |
| `MILLISECOND_1` | `timer1msHandler` → `on1msTick()` | `false` | Periodic trigger pulse dispatch (~50 ms cadence) for all registered instances. |

The 1 µs subscription runs **in ISR context** (`immediately = true`), so the
handler must be fast. The 1 ms subscription is dispatched from the timer ISR
but not marked immediate.

> Call exactly once after construction. Returns `false` only when the
> 4-instance limit is exceeded.

### `void end()`

Releases the timer subscriptions and unregisters this instance from the
static array; symmetric counterpart to `begin()`.

1. Removes `this` from `_sonarInstances[]` and decrements `_instanceCount`.
2. Unsubscribes `timer1usHandler` and `timer1msHandler` from the
   `TimerManager`.

`end()` does **not** reset pin modes. The instance can be re-`begin()`'d
later if needed. Always call `end()` before destruction to free the timer
slot and avoid a dangling pointer in the static array.

**Returns:** nothing.

### `uint16_t readCm()`

| Return | Unit | Meaning |
|---|---|---|
| `uint16_t` | cm | Last measured distance. **`0` ⇒ out-of-range or timeout.** |

Returns the most recent asynchronous measurement. Non-blocking; safe to poll
from `loop()`. The value is `volatile`-backed and updates every ~50 ms as the
1 ms timer slot fires triggers and the 1 µs slot captures echo edges.

The distance is already filtered through the median-of-3 pipeline before
being stored in `_lastCm` (see [Filtering](#filtering)).

### `uint16_t getFilteredDistance()`

| Return | Unit | Meaning |
|---|---|---|
| `uint16_t` | cm | Median of the last 3 valid readings. `0` if no readings have been collected. |

Returns the median of up to the last 3 valid (non-zero) distance readings
from the 5-element ring buffer. Provides stability by discarding transient
spikes. When fewer than 3 readings have been collected, returns the median
of the available set (which is the middle element after sorting). Returns
`0` when `_readingCount == 0`.

> See [Filtering](#filtering) for the ring-buffer and median algorithm.

### `uint8_t getState() const`

| Return | Meaning |
|---|---|
| `uint8_t` | Internal state enum cast to `uint8_t`. |

Returns the current internal state for diagnostics:

| Value | State | Meaning |
|---|---|---|
| `0` | `IDLE` | Waiting for the next trigger cycle. |
| `1` | `TRIGGERING` | *(Reserved; not currently set in the implementation.)* |
| `2` | `WAITING_ECHO` | 10 µs trigger pulse complete; waiting for ECHO to go HIGH. |
| `3` | `MEASURING` | ECHO is HIGH; measuring the return pulse width. |

`const`-qualified.

### `uint32_t getLastTriggerUs() const`

| Return | Unit | Meaning |
|---|---|---|
| `uint32_t` | µs | `micros()` timestamp of the most recent trigger pulse. |

Useful for diagnostics and echo-timeout calculations. `const`-qualified.

### `static void on1usTick()`

**Static callback** invoked by `TimerManager` at 1 µs intervals (ISR
context). Iterates over all registered `_sonarInstances[]` and for each:

- Reads the `echoPin` level via `digitalRead()`.
- If state is `WAITING_ECHO` and ECHO is `HIGH` → calls `handleEchoRise()`
  (records `_echoRiseUs`, transitions to `MEASURING`).
- If state is `MEASURING` and ECHO is `LOW` → calls `handleEchoFall()`
  (computes distance, pushes to filter, transitions to `IDLE`).
- If state is `WAITING_ECHO` or `MEASURING` and the elapsed time since
  `_lastTriggerUs` exceeds `_maxTimeoutUs` → resets to `IDLE` with `_lastCm = 0`.

Users do not call this directly; it is registered as a `TimerManager`
subscription via `begin()`.

### `static void on1msTick()`

**Static callback** invoked by `TimerManager` at 1 ms intervals. Maintains an
internal ~50 ms cadence and, when the gate elapses, iterates over all
registered `_sonarInstances[]` and calls `trigger()` on any instance in
`IDLE` state.

Users do not call this directly; it is registered as a `TimerManager`
subscription via `begin()`.

### `static SonarPrecise* create(uint8_t trigPin, uint8_t echoPin, uint16_t maxDistanceCm)`

**Factory.** Heap-allocates a `SonarPrecise` via `new` and returns the raw
pointer. Equivalent to:

```cpp
SonarPrecise* s = new SonarPrecise(trigPin, echoPin, maxDistanceCm);
```

The caller **owns** the returned pointer — call `end()` then `delete` when
finished. `create()` does **not** call `begin()`; the caller must invoke
`begin()` on the returned object to activate the sensor.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `trigPin`        | `uint8_t`  | — *(required)* | Output pin for TRIG. |
| `echoPin`        | `uint8_t`  | — *(required)* | Input pin for ECHO. |
| `maxDistanceCm`  | `uint16_t` | — *(required)* | Maximum range in cm (no default in this overload). |

---

## Filtering

`SonarPrecise` uses a **median-of-3 filter** over a 5-element ring buffer to
suppress transient outliers.

When `handleEchoFall()` produces a valid (non-zero) raw distance:

1. The value is stored in `_readings[_readingIndex]`, a 5-element circular
   buffer.
2. `_readingIndex` advances (`(_readingIndex + 1) % 5`).
3. `_readingCount` increments up to a maximum of 5.
4. `getFilteredDistance()` copies the **last 3** readings into a temporary
   array, bubble-sorts them, and returns the middle element (`temp[count / 2]`).
5. The median result becomes the new `_lastCm`, which `readCm()` returns.

| Scenario | Behavior |
|---|---|
| 0 readings collected | `getFilteredDistance()` returns `0`. |
| 1–2 readings collected | Median of the available set (for 1 reading: the value itself; for 2: the larger value). |
| 3+ readings collected | Median of the last 3 readings. |
| Raw distance < 2 cm or > maxDistanceCm | Treated as invalid (`0`); **not** pushed to the filter buffer. |

This approach ensures that a single glitch (e.g. acoustic cross-talk or an
early reflection) does not corrupt the filtered output.

---

## Timing model

```
begin() ──► (every ~50 ms, in TimerManager 1ms slot)
            on1msTick() : if Idle & ≥50ms since last trigger → trigger()
              trigger() : LOW(2µs) → HIGH(10µs) → LOW; _state = WAITING_ECHO
            ──► (every 1 µs, in TimerManager 1µs slot)
               on1usTick() for each instance:
                 WAITING_ECHO + ECHO HIGH → handleEchoRise(): record _echoRiseUs, state = MEASURING
                 MEASURING + ECHO LOW     → handleEchoFall(): cm = (nowUs - _echoRiseUs)/58;
                                            push to median filter; _lastCm = filtered; state = IDLE
                 (elapsed since trigger > _maxTimeoutUs) → state = IDLE, _lastCm = 0
            ──► readCm() / getFilteredDistance() return at any time
```

- **Trigger cadence:** a ping is initiated at most once every ~50 ms
  (`nowMs - lastTriggerMs >= 50`), roughly twice the Sonar rate.
- **Trigger pulse:** 10 µs HIGH (HC-SR04 spec), preceded by a 2 µs LOW
  settling delay.
- **Echo capture:** edges are detected at 1 µs resolution via
  `TimerManager::MICROSECOND_1` — significantly finer than the 1 ms slot
  used by `Sonar`.
- **Distance formula:** `cm = echo_us / 58`.
- **Range clamp:** readings < 2 cm (near-field noise) or > `maxDistanceCm`
  are discarded as `0`.
- **Timeout:** if an echo does not return within `maxDistanceCm × 58` µs,
  the measurement is abandoned and `_lastCm` is set to `0`.

### Comparison with `Sonar`

| Aspect | `Sonar` | `SonarPrecise` |
|---|---|---|
| Timer resolution | 1 ms (`MILLISECOND_1`) | 1 µs (`MICROSECOND_1`) + 1 ms |
| Echo edge capture | PCINT (Nano) or polling at 1 ms (Mega) | Polled at 1 µs on all platforms |
| Max instances | 1 (static self-pointer) | 4 (static array) |
| Trigger cadence | ~100 ms | ~50 ms |
| Filtering | None | Median-of-3 ring buffer |
| Timeout | Clamped to `maxRangeCm` after conversion | Microsecond-precise timeout (`maxTimeoutUs`) |
| `begin()` return | `void` | `bool` (fails if 4 instances exist) |

---

## Lifecycle

```
construct(trig, echo, maxDistanceCm) ──► begin() ─► readCm() / getFilteredDistance() (poll, repeat)
                                           │
                                           ▼ fail (≥4 instances)
                                         return false
                                           │
                                           ▼ success
                                         TimerManager 1µs + 1ms subscriptions active
                                           │
                                           ▼
                                         end() ──► destroy (delete if heap-allocated)
```

1. **Construct** with TRIG/ECHO pin numbers and an optional max range.
2. **`begin()`** once — checks the 4-instance limit, sets pin modes,
   registers in the static array, subscribes to both timer slots. Check the
   `bool` return; `false` means the array is full.
3. **Poll** `readCm()` or `getFilteredDistance()` in `loop()` — values update
   every ~50 ms autonomously.
4. **`end()`** to unregister from the static array and unsubscribe from the
   timers. **Always call `end()` before destruction** to prevent a dangling
   pointer in `_sonarInstances[]`.
5. If obtained via `create()`, the caller owns the pointer — `end()` then
   `delete` when done.

---

## Usage notes

- **Multi-instance:** up to 4 `SonarPrecise` objects can coexist. The static
  `_sonarInstances[4]` array is scanned by both timer callbacks on every tick.
  `begin()` returns `false` when the array is full.
- **Timer footprint:** each active `SonarPrecise` (or set of them) consumes
  one `MICROSECOND_1` subscription and one `MILLISECOND_1` subscription —
  shared across all instances by the static callbacks. Only one subscription
  of each interval is held regardless of the number of instances (1–4).
- **Always call `end()` before destruction.** The static array holds a raw
  pointer; destroying an instance without `end()` leaves a dangling pointer
  that the timer ISR will dereference on the next tick.
- **Non-blocking:** `begin()` starts autonomous ranging; `readCm()` returns
  the latest result immediately. There is no blocking `ping()` call.
- **Sentinel:** treat `0` as *no reading* (out of range, timeout, near-field
  noise, or before the first measurement completes). Do not interpret a
  literal 0 cm.
- **ISR context:** `on1usTick()` runs in ISR context (`immediately = true`
  on the 1 µs subscription). Keep `digitalRead()` calls and state-machine
  transitions lean. `on1msTick()` dispatches from the timer ISR as well.
- **Factory ownership:** `create()` heap-allocates via `new`. The caller
  owns the pointer and must `end()` then `delete` it.
- **No default pins:** TRIG and ECHO are required at construction; no
  `BYBYTE_*_PIN` defaults are used.
- **Echo pin polling:** unlike `Sonar`'s PCINT-based edge detection on the
  Nano, `SonarPrecise` polls the echo pin at 1 µs via the `TimerManager`.
  This gives uniform behaviour across platforms (Nano and Mega) without
  requiring PCINT-capable pins.

---

## File map

| File | Role for `SonarPrecise` |
|---|---|
| `src/SonarPrecise.h` | Declares the class (this document). |
| `src/SonarPrecise.cpp` | Implements the class with ISR callbacks. |
| `src/core/TimerManager.h` | Provides 1 µs and 1 ms scheduler slots. |
| `src/core/configs/PlatformDetect.h` | Platform detection. |

---

For a simpler single-instance alternative with 1 ms tick, see
[`Sonar`](sonar.md).
