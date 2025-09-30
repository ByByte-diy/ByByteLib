# ByByteLib

Minimal, extensible robotics API for Arduino and PlatformIO.

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
See `examples/BasicMove/BasicMove.ino` and `examples/DifferentialDrive/DifferentialDrive.ino`.

## License
MIT

