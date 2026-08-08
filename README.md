# ByByteLib

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-00979D.svg)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-orange.svg)](https://platformio.org/)
[![Platform: AVR](https://img.shields.io/badge/Platform-AVR-1C6EA4.svg)](https://docs.platformio.org/en/latest/platforms/atmelavr.html)
[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-00599C.svg)](https://isocpp.org/)
[![Version](https://img.shields.io/badge/version-1.0.0-green.svg)](library.json)

Minimal, extensible robotics API for Arduino and PlatformIO.

## Documentation

Full public-API reference lives under [`docs/`](docs/README.md). Quick links:

| Class | Document | Scope |
|---|---|---|
| `ByByte::MotorDriver` | [docs/api/motor-driver.md](docs/api/motor-driver.md) | Self-contained motor module (Direct + Differential, auto-selects TB6612/DRV8833). |
| `ByByte::MotorController` | [docs/api/motor-controller.md](docs/api/motor-controller.md) | Abstract brushed dual-motor backend (**advanced**; use `MotorDriver` for typical use). |
| `ByByte::LdrSensor` | [docs/api/ldr-sensor.md](docs/api/ldr-sensor.md) | LDR light sensor with auto-calibration and normalized output. |
| `ByByte::Lm35Sensor` | [docs/api/lm35-sensor.md](docs/api/lm35-sensor.md) | Analog temperature sensor returning °C / °F. |
| `ByByte::BatterySensor` | [docs/api/battery-sensor.md](docs/api/battery-sensor.md) | Battery voltage / charging status / SoC (**Mega**; stub on Nano). |
| `ByByte::Sonar` | [docs/api/sonar.md](docs/api/sonar.md) | Non-blocking HC-SR04-style ultrasonic rangefinder (current sonar API). |
| `ByByte::Buzzer` | [docs/api/buzzer.md](docs/api/buzzer.md) | Non-blocking tone / pattern buzzer (built-in sequences in flash). |
| `ByByte::Servo` | [docs/api/servo.md](docs/api/servo.md) | Hobby servo: angle / µs / continuous-rotation speed (**Mega**; no-op on Nano). |
| `ByByte::Bluetooth` | [docs/api/bluetooth.md](docs/api/bluetooth.md) | HC-02/05/06/08/42 bridge: AT commands + Stream API (Mega UART+power; Nano SoftwareSerial). |
| `ByByte::SideIrSensors` | [docs/api/side-ir-sensors.md](docs/api/side-ir-sensors.md) | Pair of side-facing analog IR sensors: power-gated sampling + normalized 0..1000 output. |
| `ByByte::ByByteKit` / Nano / Mega | [docs/api/bybyte-kit.md](docs/api/bybyte-kit.md) | Product kit abstractions: pre-wired motor + Bluetooth bundles. |

> Conventions: only **public** symbols are documented; see
> [docs/README.md](docs/README.md) for the index and conventions.

## Features
- Core interfaces for robot platforms, motor control, kinematics, sensors
- Works with Arduino IDE (library.properties) and PlatformIO (library.json)
- Examples for quick start

## Installation

### Arduino IDE
1. Download this repository as ZIP.
2. In Arduino IDE: Sketch → Include Library → Add .ZIP Library… and choose the ZIP.
3. Open examples from File → Examples → ByByteLib.

### PlatformIO
Add to `platformio.ini`:

```ini
lib_deps =
  ByByteLib
```

Or use the included PlatformIO example under `examples/platformio_basic`.

## Usage

See the examples in the `examples/` directory:

- `examples/motors/` — `Simple`, `Direct`, `Tank` motor sketches
- `examples/bluetooth/` — `CarControl`, `Passtrough` Bluetooth sketches
- `examples/sensors/` — `BatteryLcd_Mega`, `LightAutoLamp`, `MPU6050_Mega`,
  `SideIrSensors`, `Sonar`, `SystemTest_Mega`, `TempLcd_Mega`
- `examples/buzzer/` — `AllFeatures`
- `examples/ir/` — `Receiver`
- `examples/line/` — `LineFollow_Mega`, `LineFollow_Nano`
- `examples/servo/` — `Continuous`, `PotControl`

Include `<ByByteLib.h>` for all modules at once, or include individual headers like `<MotorDriver.h>`, `<Bluetooth.h>`, etc.

## License

MIT — see [LICENSE](LICENSE).

