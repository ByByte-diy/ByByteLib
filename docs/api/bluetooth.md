# `ByByte::Bluetooth`

> UART bridge to an HC-02 / HC-05 / HC-06 / HC-08 / HC-42 Bluetooth module,
> exposing a `Stream`-like data API plus AT-command helpers (rename, reset,
> pin, …) and a non-blocking "readiness" check.
>
> **Header:** `src/Bluetooth.h`
> **Namespace:** `ByByte`
> **Kind:** Concrete class

`Bluetooth` abstracts the wiring differences between the ByByte boards:

| Platform | Transport | Power control | Constructor args |
|---|---|---|---|
| **Mega** (`BYBYTE_PLATFORM_ID == BYBYTE_PLATFORM_MEGA`) | `HardwareSerial` (default `BYBYTE_BT_UART` = `Serial1`) | Optional supply gating on a digital power pin (default `BYBYTE_BT_PWR_PIN` = `29`; **HIGH** powers the module) | `HardwareSerial& uart`, `BtMegaBluetoothPins layout` |
| **Nano / other** | `SoftwareSerial` (heap-allocated) on configurable RX/TX (default `BYBYTE_BT_SW_RX_PIN` = `2`, `BYBYTE_BT_SW_TX_PIN` = `3`); always powered | none | `BtSoftwareSerialPins pins` |

The class is **non-blocking** after `begin()`: it opens the UART at the
requested baud, then asks the caller to poll `isReady()` (or call
`available()`, which triggers a readiness tick) until the module responds to
`AT`. Once ready, the `Stream`-like methods pass data through transparently,
and the AT helpers issue command-mode instructions.

> **Module must be in AT command mode** for `isReady()` and the AT helpers to
> succeed. See [Operational requirements](#operational-requirements) below —
> this is the most common reason `isReady()` never becomes true.

---

## Synopsis

```cpp
namespace ByByte {

enum class BtModuleType { Unknown, HC02, HC05, HC06, HC08_BLE, HC42_BLE };

class Bluetooth {
public:
    // Constructor signature depends on the build target (see below).

    ~Bluetooth();
    Bluetooth(const Bluetooth&)            = delete;
    Bluetooth& operator=(const Bluetooth&) = delete;

    bool begin(uint32_t desiredBaud = 9600);

    void startReadinessCheck();
    bool isReady();
    bool isChecking() const;

    // Stream-like data API
    int    available();
    int    read();
    size_t readBytes(uint8_t* buffer, size_t length);
    size_t write(uint8_t b);
    size_t write(const uint8_t* data, size_t length);
    void   flush();

    // Power control (Mega only; no-op elsewhere)
    void powerOn();
    void powerOff();

    // AT commands (classic modules)
    bool rename(const char* newName);
    bool getName(String& outName);
    bool reset();
    bool setPin(const char* pin4digits);
    bool restoreDefault();

    // BLE modules (best-effort)
    bool setBleName(const char* newName);
    bool getBleName(String& outName);

    bool ping(uint16_t timeoutMs = 500);

    BtModuleType moduleType() const;
    uint32_t     baud() const;
};

} // namespace ByByte
```

### Constructor — per platform

```cpp
// Mega
Bluetooth(HardwareSerial& uart = BYBYTE_BT_UART,
          bb::pins::by_byte_mega::BtMegaBluetoothPins layout =
              bb::pins::by_byte_mega::defaultBluetoothMega());

// Nano / other
explicit Bluetooth(bb::pins::by_byte_nano::BtSoftwareSerialPins pins =
                       bb::pins::by_byte_nano::defaultBluetoothSoftwareSerialPins());
```

The constructor is selected at compile time by `BYBYTE_PLATFORM_ID`, so the
same sketch compiles on either target without `#ifdef`s in user code.

---

## Types

### `enum class BtModuleType`

Module family reported by `moduleType()`.

| Enumerator | Module |
|---|---|
| `Unknown`  | Not yet detected / no response. |
| `HC02`     | HC-02 (classic, AT-friendly). |
| `HC05`     | HC-05 (classic master/slave). |
| `HC06`     | HC-06 (classic slave). |
| `HC08_BLE` | HC-08 BLE. |
| `HC42_BLE` | HC-42 BLE. |

> `moduleType()` returns `Unknown` until detection has run; the public API
> does not force detection automatically — call `ping()` / an AT command,
> which exercises the link. (Detection itself is an internal routine; the
> public surface only exposes the resulting type via `moduleType()`.)

---

## Configuration

Defaults come from the active platform config via `configs/ByByteConfig.h`
and the per-board pin maps (`configs/BbPinsByByteMega.h`,
`configs/BbPinsByByteNano.h`). All are overridable with `#define` before
including the header.

### Mega

| Macro | Default | Purpose |
|---|---|---|
| `BYBYTE_BT_UART`    | `Serial1` | Hardware UART connected to the module. |
| `BYBYTE_BT_PWR_PIN` | `29`      | Digital pin gating module supply (**HIGH** = powered). |

`bb::pins::by_byte_mega::BtMegaBluetoothPins` is `{ uint8_t powerPin; }`; the
default factory `defaultBluetoothMega()` returns `{ BYBYTE_BT_PWR_PIN }`.

### Nano / other

| Macro | Default | Purpose |
|---|---|---|
| `BYBYTE_BT_SW_RX_PIN` | `2` | MCU RX ← module **TX**. |
| `BYBYTE_BT_SW_TX_PIN` | `3` | MCU TX → module **RX**. |

`bb::pins::by_byte_nano::BtSoftwareSerialPins` is `{ uint8_t rx; uint8_t tx; }`;
the default factory `defaultBluetoothSoftwareSerialPins()` returns
`{ BYBYTE_BT_SW_RX_PIN, BYBYTE_BT_SW_TX_PIN }`.

> **Wiring is crossed:** MCU RX (D2) connects to the module's **TX**, and MCU
> TX (D3) connects to the module's **RX**. A straight-through cable produces
> a silent link that never becomes ready.

---

## Public interface

### Constructor (per-platform — see above)

Constructs the bridge bound to its transport.

| Platform | What the ctor does |
|---|---|
| Mega | Records the `HardwareSerial*` and `powerPin`. No I/O. |
| Nano/other | Records the pin pair and **heap-allocates** a `SoftwareSerial(rx, tx)`. No I/O. |

Non-blocking; the UART is opened later by `begin()`.

### `~Bluetooth()`

Releases the transport.

| Platform | What the dtor does |
|---|---|
| Mega | (UART lifetime is owned by the Arduino core; nothing is freed.) |
| Nano/other | `delete`s the heap `SoftwareSerial*`. |

### Copy operations — deleted

```cpp
Bluetooth(const Bluetooth&)            = delete;
Bluetooth& operator=(const Bluetooth&) = delete;
```

`Bluetooth` owns the transport handle / heap `SoftwareSerial`; copying would
alias the same UART with two objects. Move is not provided either — pass
instances by reference or hold them by value in a single owner.

### `bool begin(uint32_t desiredBaud = 9600)`

Open the UART at `desiredBaud` and start the readiness check.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `desiredBaud` | `uint32_t` | `9600` | Baud to open the link at. Must match the module's data/AT baud (HC-06/02 factory 9600; HC-05 AT-mode factory **38400**). |

**Returns:** `true` always (the call opens the UART and arms the readiness
check; actual module presence is established asynchronously by `isReady()`).

Internally:
1. **Mega:** drives the power pin `HIGH` and waits 300 ms for the regulator
   and module to boot (avoids false "init FAILED"). **Nano/other:** no power
   control — the module is assumed always powered.
2. `closeUart()` / `openUart(desiredBaud)`.
3. Records `_baud = desiredBaud`.
4. `startReadinessCheck()` — arms the non-blocking probe loop.

**Returns:** nothing is awaited; poll `isReady()` afterward.

### `void startReadinessCheck()`

Reset the readiness state machine:

- `_isReady = false`
- `_isChecking = true`
- `_checkAttempts = 0`
- `_lastCheckTime = millis()`

Called automatically by `begin()`; exposed so the caller can restart a
probe later (e.g. after `reset()` or a power cycle) without re-calling
`begin()`.

**Returns:** nothing.

### `bool isReady()`

Non-blocking readiness poll. Call from `loop()`.

**Returns:** `true` once the module has answered (or data has arrived);
`false` while still probing or after the probe gave up.

Behavior:
- If already ready ⇒ `true`.
- If not checking ⇒ `false`.
- Throttles pings to **once per second** (`millis() - _lastCheckTime >= 1000`).
- On each tick, calls `ping(500)`. On success, marks ready and returns `true`.
- After **10 failed attempts**, gives up, prints
  `BT init FAILED after 10 attempts`, leaves `_isReady = false`,
  `_isChecking = false`.

> The probe sends `AT` and looks for `OK`. This only works if the module is
> in **AT command mode** at the configured baud — see
> [Operational requirements](#operational-requirements).

### `bool isChecking() const`

| Return | Meaning |
|---|---|
| `true`  | A readiness probe is in progress (will keep polling `isReady()`). |
| `false` | Either never started, already ready, or already gave up. |

`const`-qualified; read-only introspection of the probe state.

### `int available()`

| Return | Meaning |
|---|---|
| `≥ 1`  | Number of bytes ready to `read()` from the module. |
| `0`    | Nothing ready right now (and the readiness tick may have run). |

`Stream`-like. Side effect: if bytes are available, the module is marked
ready (`_isReady = true`, `_isChecking = false`) — data arrival doubles as a
readiness signal. If nothing is available, it calls `isReady()` once to keep
the probe ticking.

### `int read()`

| Return | Meaning |
|---|---|
| `0..255` | Next byte from the module. |
| `-1`     | Nothing available / transport not open. |

`Stream`-like. Returns `int` so `-1` can signal "no data".

### `size_t readBytes(uint8_t* buffer, size_t length)`

| Parameter | Type | Purpose |
|---|---|---|
| `buffer` | `uint8_t*` | Destination buffer. |
| `length` | `size_t`   | Max bytes to read. |

| Return | Meaning |
|---|---|
| `size_t` | Number of bytes actually placed in `buffer` (may be `< length` on timeout). |

Wraps the underlying `readBytes((char*)buffer, length)` (uses the Stream
default timeout).

### `size_t write(uint8_t b)`

| Parameter | Type | Purpose |
|---|---|---|
| `b` | `uint8_t` | Single byte to send to the module. |

| Return | Number of bytes written (`1` on success, `0` if transport closed). |

### `size_t write(const uint8_t* data, size_t length)`

| Parameter | Type | Purpose |
|---|---|---|
| `data`   | `const uint8_t*` | Bytes to send. |
| `length` | `size_t`         | Byte count. |

| Return | Number of bytes written (`length` on success, `0` if transport closed). |

### `void flush()`

Block until the transmit buffer has drained to the module. No-op if the
transport is closed.

### `void powerOn()`

Assert the module supply. **Mega only:** sets the power pin `OUTPUT` and
**HIGH**. On the Nano / other boards this is a no-op (module assumed always
powered). Called automatically by `begin()` on the Mega.

### `void powerOff()`

Cut the module supply. **Mega only:** sets the power pin `OUTPUT` and
**LOW**. No-op on the Nano / other boards.

### `bool rename(const char* newName)`

Issue `AT+NAME<newName>` and look for `OK`.

| Parameter | Type | Purpose |
|---|---|---|
| `newName` | `const char*` | New Bluetooth visible name. |

**Returns:** `true` if the response contained `OK`; `false` otherwise
(includes transport closed or timeout). Requires the module to be in AT
command mode.

### `bool getName(String& outName)`

Query the module name, trying `AT+NAME?` then `AT+NAME`, looking for `+NAME`.

| Parameter | Type | Purpose |
|---|---|---|
| `outName` | `String&` | Out: the raw response containing the name token. |

**Returns:** `true` if a response matched; `false` otherwise. On success,
`outName` holds the module's reply string (caller parses the `+NAME` token).

### `bool reset()`

Issue `AT+RESET` and look for `OK` (timeout 800 ms).

**Returns:** `true` on `OK`; `false` otherwise. After a reset, the module may
reboot — re-probe readiness with `startReadinessCheck()` / `isReady()`.
Works for HC-05/06 and HC-08/42 BLE.

### `bool setPin(const char* pin4digits)`

Set the pairing PIN.

| Parameter | Type | Purpose |
|---|---|---|
| `pin4digits` | `const char*` | 4-digit PIN string (e.g. `"1234"`). |

Tries `AT+PSWD=<pin>` (HC-05 classic) then `AT+PIN<pin>` (HC-06 style);
returns `true` on the first `OK`.

### `bool restoreDefault()`

Restore factory defaults, trying module-specific commands in order:
`AT+ORGL` (HC-05) → `AT+DEFAULT` (HC-06) → `AT+RENEW` (BLE), 800 ms each.

**Returns:** `true` as soon as one command returns `OK`; `false` if none
matched.

### `bool setBleName(const char* newName)`

Best-effort BLE name set — currently delegates to `rename()` (HC-08/HC-42
often share the `AT+NAME` path with classic modules).

### `bool getBleName(String& outName)`

Best-effort BLE name query — delegates to `getName()`.

### `bool ping(uint16_t timeoutMs = 500)`

Send `AT` and look for `OK` within `timeoutMs`, at the current baud.

| Parameter | Type | Default | Purpose |
|---|---|---|---|
| `timeoutMs` | `uint16_t` | `500` | Max wait for the `OK` reply. |

**Returns:** `true` if `OK` was seen; `false` otherwise. This is the primitive
used by `isReady()`. Useful for ad-hoc link testing.

### `BtModuleType moduleType() const`

| Return | Detected module family (or `Unknown`). |
|---|---|

`const`-qualified. Reflects the result of internal detection; `Unknown`
until an AT exchange has identified the module.

### `uint32_t baud() const`

| Return | The baud the UART was opened at by `begin()` (i.e. `desiredBaud`). |
|---|---|

`const`-qualified. Note this is the *configured* baud, not a probed value.

---

## Operational requirements

The readiness probe (`isReady()` → `ping()` → `AT` → `OK`) and **all** AT
helpers only succeed when the module is in **AT command mode** and answering
at the configured baud. Hardware/setup conditions that block this:

| Module | AT-mode entry | Factory AT baud | Notes |
|---|---|---|---|
| **HC-05** | Hold **KEY/EN** HIGH **at power-up** to boot into AT mode. | **38400** | If KEY floats LOW it boots into *data* mode and ignores `AT`; pass `38400` to `begin()`. |
| **HC-06 / HC-02** | In AT mode automatically while **unpaired**. | **9600** | Once **paired to a phone**, they drop out of AT mode → no `OK`. Disable Bluetooth on the phone before running a sketch that pings. |
| **HC-08 / HC-42 BLE** | In AT mode when **not connected**. | **9600** | Same pairing caveat as HC-06. |

Other common link-failure causes (all produce the repeated
`AT try without CRLF failed` / `AT try with CRLF failed` + `BT init FAILED`
log and `isReady() == false`):

- **RX/TX straight-through** instead of crossed — MCU RX must go to module
  **TX**, MCU TX to module **RX**.
- **Missing common GND** between module and board.
- **Baud mismatch** — module not at `desiredBaud` (e.g. an HC-05 at its
  38400 AT-mode default but `begin(9600)` was called).
- **Marginal logic levels** — HC modules are 3.3 V; on a 5 V board a direct
  MCU TX (5 V) is usually tolerated, but a divider is safer; module TX at
  3.3 V is read fine by AVR.

> `begin()` **returns `true` unconditionally** — it only opens the UART and
> arms the probe. Actual module presence is determined asynchronously by
> `isReady()`. Treat `isReady() == false` for >10 s as a
> wiring/mode/baud problem, not a `begin()` failure.

---

## Lifecycle

```
construct(uart/pins) ──► begin(baud) ──► isReady() poll (loop) ──► ready ──► available()/read()/write() data + AT helpers
                                              │
                                              └─► (10 failed pings) → gives up, _isReady stays false
```

1. **Construct** with the platform transport (Mega: `HardwareSerial&` +
   power layout; Nano: SoftwareSerial pins).
2. **`begin(baud)`** once — powers the module (Mega), opens the UART, arms
   the readiness check.
3. **Poll `isReady()`** from `loop()` until it returns `true` (the module
   answered `AT` with `OK`, or data arrived).
4. **Use the data API** — `available()` / `read()` / `readBytes()` /
   `write()` — for transparent data once connected.
5. **Issue AT commands** (`rename`, `getName`, `reset`, `setPin`,
   `restoreDefault`, …) while in AT mode. After `reset()`, call
   `startReadinessCheck()` and re-probe `isReady()`.
6. **`powerOff()`** (Mega) to cut supply; **`powerOn()`** / `begin()` to
   restart. Held by value; the dtor releases the heap `SoftwareSerial` on
   the Nano.

---

## Usage notes

- **Non-blocking:** every public method returns quickly. The readiness probe
  is paced to one ping per second; do not busy-wait on `isReady()` — call it
  from `loop()`.
- **`begin()` is not a readiness verdict:** it returns `true` even with no
  module attached. Use `isReady()` to confirm.
- **`available()` advances the probe:** calling `available()` when no data is
  ready triggers a `isReady()` tick, so either method keeps the state machine
  alive.
- **AT mode is mandatory** for `isReady()` and the AT helpers; in data mode
  (paired / KEY low) the module won't answer `AT`. See
  [Operational requirements](#operational-requirements).
- **Mega power pin is active-HIGH:** `begin()` / `powerOn()` assert HIGH to
  power the module; `powerOff()` asserts LOW.
- **Nano has no power control:** the module is assumed always powered;
  `powerOn()/powerOff()` are no-ops.
- **Single transport per instance:** one `Bluetooth` owns one UART /
  `SoftwareSerial`. Multiple instances on distinct UARTs are possible but
  share the AVR's limited hardware serial / timer resources.
- **No public accessors** for `_isReady` internals beyond `isReady()` /
  `isChecking()`; `_baud` is exposed via `baud()`, `_type` via `moduleType()`.
  All probe/AT internals (`enterAtMode`, `detectModuleType`, `sendAT`, …) are
  private.
- **String-based:** AT helpers use Arduino `String` for replies; keep the
  heap headroom in mind on small AVR parts.
- **`Serial` debugging:** the implementation prints probe diagnostics to the
  global `Serial` (e.g. `AT try ... failed`, `BT init FAILED after 10
  attempts`); keep `Serial.begin(...)` in your sketch so these are visible.

---

## File map

| File | Role for `Bluetooth` |
|---|---|
| `src/Bluetooth.h` | Declares `BtModuleType`, the `Bluetooth` class, and the platform-conditional constructor. |
| `src/Bluetooth.cpp` | Implements the class; includes `<SoftwareSerial.h>` on non-Mega targets; holds the AT read/parse loop. |
| `src/configs/ByByteConfig.h` | Provides `BYBYTE_BT_UART` / `BYBYTE_BT_PWR_PIN` (Mega) and `BYBYTE_BT_SW_RX_PIN` / `BYBYTE_BT_SW_TX_PIN` (Nano) defaults. |
| `src/configs/BbPinsByByteMega.h` | Defines `bb::pins::by_byte_mega::BtMegaBluetoothPins` + `defaultBluetoothMega()`. |
| `src/configs/BbPinsByByteNano.h` | Defines `bb::pins::by_byte_nano::BtSoftwareSerialPins` + `defaultBluetoothSoftwareSerialPins()`. |
| `src/configs/PlatformDetect.h` | Defines `BYBYTE_PLATFORM_ID` / `BYBYTE_PLATFORM_MEGA` that select the constructor + transport. |