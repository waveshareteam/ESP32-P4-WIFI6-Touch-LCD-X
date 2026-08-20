# ESP32-P4-WIFI6-Touch-LCD-X Running ESP-Brookesia Phone

[中文版本](./README_ZH.md)

This example demonstrates how to run the ESP-Brookesia Phone UI on the
ESP32-P4-WIFI6-Touch-LCD-X series. The default configuration targets the
7-inch 720 x 1280 panel; the 8-inch and 10.1-inch 800 x 1280 panels can be
selected in menuconfig.

## Getting Started

### Hardware Requirements

* An ESP32-P4-WIFI6-Touch-LCD-7, -8, or -10.1 development board.

### ESP-IDF Required

- This example is validated with ESP-IDF `v5.5.5` and `v6.0.2` by the
  repository CI and supports later ESP-IDF v5.5.x and v6.x releases.
- Please follow the
  [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
  to set up the development environment.

### Configuration

Run `idf.py menuconfig` and select the panel that matches your board under
`Board Support Package (ESP32-P4) > Display > Select LCD type`, then adjust
the ESP-Brookesia options as needed.

## How to Use the Example

### Build and Flash the Example

Build the project and flash it to the board, then run the monitor tool to
view serial output (replace `PORT` with your board's serial port name):

```c
idf.py -p PORT flash monitor
```

To exit the serial monitor, type `Ctrl-]`.

See the
[ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html)
for full steps to configure and use ESP-IDF to build projects.

## Technical Support and Feedback

- For board support, open an
  [issue](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/issues)
  or contact [Waveshare support](https://service.waveshare.com/).
- For ESP-Brookesia questions, use the
  [esp32.com](https://esp32.com/viewforum.php?f=22) forum or the
  [esp-brookesia repository](https://github.com/espressif/esp-brookesia).
