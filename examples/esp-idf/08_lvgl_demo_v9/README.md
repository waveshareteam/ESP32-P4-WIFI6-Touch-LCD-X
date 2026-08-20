| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# LVGL 9 Demo

[中文版本](./README_ZH.md)

This example starts the board display and touch stack with LVGL 9, enables the
backlight, and runs `lv_demo_benchmark()`. The default configuration uses
triple-buffer partial refresh to reduce tearing and enables the LVGL demo,
performance-monitor, and system-monitor features required by the benchmark.

## Select the Display Variant

Use `idf.py menuconfig` to select the 7, 8, or 10.1-inch panel that matches the
board under `Board Support Package (ESP32-P4) > Display > Select LCD type`.
The checked-in defaults select the 10.1-inch panel, 32 MB flash, and 200 MHz
PSRAM.

The source also contains commented entry points for the LVGL music and widgets
demos. Enabling a different demo may require additional media or audio
configuration; keep those changes separate from the benchmark baseline.

## Build, Flash, and Monitor

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

Use the on-screen benchmark and serial performance data to inspect rendering
behavior. Compile success alone does not validate panel selection or touch
orientation on hardware.
