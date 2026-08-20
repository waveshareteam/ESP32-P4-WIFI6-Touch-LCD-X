| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# Interactive I2C Tools

[中文版本](./README_ZH.md)

This example starts an interactive `i2c-tools>` console for inspecting and
operating devices on an I2C bus. The console provides `i2cconfig`,
`i2cdetect`, `i2cget`, `i2cset`, and `i2cdump` commands together with the
standard ESP-IDF console commands.

## Board Defaults

- I2C controller: port 0
- SCL: GPIO 8
- SDA: GPIO 7
- Command history: FAT filesystem mounted at `/data` when enabled

The SCL and SDA defaults match the board schematic. Check bus voltage,
pull-ups, device addresses, and register documentation before operating a
connected device. `i2cset` writes device registers and can change hardware
state.

## Build, Flash, and Monitor

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

Use `help` at the `i2c-tools>` prompt to list commands and options. The pin
defaults and command-history setting are available under `Example
Configuration` in `menuconfig`.
