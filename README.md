# DS18B20 Temperature Sensor — Raspberry Pi Pico 2

[中文](README-zh.md)

A Raspberry Pi Pico 2 project for reading DS18B20 digital temperature sensors, with real-time display on an LCD1602 I2C screen and USB serial output.

## Hardware Connections

| Pico 2 Pin | Connection | Notes |
|------------|-----------|-------|
| GPIO 7 | DS18B20 DATA (DQ) | Requires external 4.7kΩ pull-up to 3.3V |
| GPIO 12 | LCD1602 SDA | I2C0 data line |
| GPIO 13 | LCD1602 SCL | I2C0 clock line |
| VBUS (5V) | LCD1602 VCC | Display power |
| GND | DS18B20 GND, LCD1602 GND | Common ground |
| 3V3(OUT) | DS18B20 VDD | Sensor power (non-parasitic mode) |

## Project Structure

```
DS18B20/
├── DS18B20.c              # Main program (init, main loop, LCD driver fix)
├── README.md              # Chinese README
├── README-EN.md           # English README
└── lib/
    ├── onewire.h / onewire.c       # 1-Wire protocol driver (bit-banging, ROM search)
    ├── ds18b20.h / ds18b20.c       # DS18B20 sensor high-level API
    └── lcd1602_i2c.h / lcd1602_i2c.c # LCD1602 I2C display driver
```

### Module Overview

| File | Description |
|------|-------------|
| `DS18B20.c` | Entry point: I2C bus scan, LCD init (standard 4-bit protocol), sensor discovery, temperature read & display |
| `lib/onewire.c` | Maxim 1-Wire low-level protocol: bus reset, bit/byte read/write, ROM search (binary tree algorithm) |
| `lib/ds18b20.c` | DS18B20 high-level API: device discovery, address caching, temperature conversion, scratchpad read/write, resolution management |
| `lib/lcd1602_i2c.c` | PCF8574 I2C backpack driver for LCD1602: 4-bit mode, cursor control, string/char output |

## Building

### Prerequisites

- Raspberry Pi Pico SDK 2.2.0 (installed at `~/.pico-sdk/sdk/2.2.0`)
- ARM cross-compiler toolchain `arm-none-eabi-gcc` 14.2
- CMake 3.31+, Ninja 1.12+

### Build Steps

```bash
# Enter project directory
cd DS18B20

# Create build directory
mkdir build && cd build

# Configure with CMake (ensure PICO_SDK_PATH is set)
cmake -G "Ninja" ..

# Build
ninja

# Or use cmake --build
cmake --build .
```

Build outputs are in `build/`. Drag `DS18B20.uf2` onto the Pico 2 USB mass storage device to flash.

## Usage

1. Connect the DS18B20 sensor and LCD1602 display as per the hardware table above
2. Drag `build/DS18B20.uf2` onto the Pico 2
3. Open a USB serial terminal (115200 baud) to monitor output
4. The LCD shows Celsius on line 0 and Fahrenheit on line 1, updating every 2 seconds

### Serial Output Example

```
DS18B20 Temperature Sensor Demo
===============================
Scanning I2C bus (i2c0)...
  I2C device found at 0x27
I2C scan complete.

Found 1 DS18B20 device(s) on the bus.

Requesting temperatures...
Temperature (index 0): 25.50 C  |  77.90 F
---------------------------------------
```

## Key Features

- **1-Wire Bit-Banging** — Precise timing via interrupt disabling + microsecond busy-wait loops
- **ROM Search** — Supports multiple DS18B20 devices on a single bus, with automatic address caching
- **LCD 4-Bit Initialization** — Standard HD44780 8-bit to 4-bit mode transition protocol
- **I2C Auto-Scan** — Scans the I2C bus at startup to detect the display address
- **Dual Temperature Units** — Simultaneous Celsius and Fahrenheit display

## License

MIT License
