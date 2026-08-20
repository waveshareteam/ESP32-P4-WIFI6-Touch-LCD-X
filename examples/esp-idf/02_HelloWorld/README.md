| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# Hello World

[中文版本](./README_ZH.md)

This example prints a greeting and reports the detected chip features,
revision, flash size, flash type, and minimum free heap. It then counts down
from ten seconds, restarts the ESP32-P4, and repeats.

## Configuration

The checked-in defaults select `esp32p4`, 32 MB flash, in-package PSRAM at
200 MHz, and the board's current ESP32-P4 revision range. These settings match
the product hardware and should be reviewed before adapting the example to a
different board.

## Build, Flash, and Monitor

```text
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with the serial port for the board. The monitor should show the
chip and memory information followed by the restart countdown.
