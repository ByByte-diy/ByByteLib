# ByByteLib — Documentation

Public API reference for the ByByteLib robotics control library
(Arduino-compatible, `namespace ByByte`).

## Contents

| Document | Scope |
|---|---|
| [api/motor-driver.md](api/motor-driver.md) | `ByByte::MotorDriver` — self-contained motor module (Direct + Differential, auto-selects TB6612/DRV8833). |
| [api/motor-controller.md](api/motor-controller.md) | `ByByte::MotorController` — abstract brushed dual-motor backend (**advanced**; see `MotorDriver` for typical use). |
| [api/ldr-sensor.md](api/ldr-sensor.md) | `ByByte::LdrSensor` — light-dependent-resistor sensor with auto-calibration. |
| [api/lm35-sensor.md](api/lm35-sensor.md) | `ByByte::Lm35Sensor` — analog temperature sensor (°C / °F). |
| [api/battery-sensor.md](api/battery-sensor.md) | `ByByte::BatterySensor` — battery voltage / charging / SoC (**Mega**; stub on Nano). |
| [api/sonar.md](api/sonar.md) | `ByByte::Sonar` — non-blocking HC-SR04-style ultrasonic rangefinder (TimerManager + PCINT). |
| [api/buzzer.md](api/buzzer.md) | `ByByte::Buzzer` — non-blocking tone / pattern buzzer (built-in sequences in flash). |
| [api/servo.md](api/servo.md) | `ByByte::Servo` — Hobby servo (angle / µs / continuous-rotation speed) — **Mega**; no-op on Nano. |
| [api/bluetooth.md](api/bluetooth.md) | `ByByte::Bluetooth` — HC-02/05/06/08/42 bridge (AT commands + Stream API); Mega UART+power, Nano SoftwareSerial. |
| [api/side-ir-sensors.md](api/side-ir-sensors.md) | `ByByte::SideIrSensors` — pair of side-facing IR analog sensors with power gating and normalization. |
| [api/bybyte-kit.md](api/bybyte-kit.md) | `ByByte::ByByteKit` / `ByByteNano` / `ByByteMega` / `ByByteNanoBoy` — product kit abstractions + aggregate headers. |

## Conventions

- Only **public** symbols are documented.
- Pure-virtual methods are shown as abstract (`= 0`); concrete implementations
  are referenced where relevant to the contract.
- Pin/speed values are PWM units in the range **[-255, 255]** unless noted.
  Negative values reverse direction.
- Aggregate headers: `#include <ByByteLib.h>` (all modules), `#include <ByByteCore.h>` (foundation: Types, PinCapabilities, platform detection, configs), `#include <ByByteKits.h>` (kit abstractions).