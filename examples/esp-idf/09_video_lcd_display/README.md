| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# Video LCD Display

[中文版本](./README_ZH.md)

This example streams the onboard MIPI-CSI camera (OV5647) to the MIPI-DSI
LCD using the
[`esp_video`](https://github.com/espressif/esp-video-components/tree/master/esp_video)
component and the board BSP.

## How to Use the Example

1. Select the camera sensor and input format:

   ```
   idf.py menuconfig
   ```

   Under `Espressif Camera Sensors Configurations`, enable the sensor that
   matches your camera module. The example defaults to `OV5647` with
   `RAW8 800x1280 50fps` MIPI input.

2. Select the LCD panel that matches your board under
   `Board Support Package (ESP32-P4) > Display > Select LCD type`
   (7-inch ILI9881C or 8 / 10.1-inch JD9365).

3. Build, flash, and monitor:

   ```
   idf.py -p PORT build flash monitor
   ```

The example requires the board's onboard OV5647 camera or a compatible
MIPI-CSI camera module.
