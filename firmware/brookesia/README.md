# ESP32-P4-WIFI6-Touch-LCD-X ESP-Brookesia Firmware

[中文](README_ZH.md)

Default firmware source for the ESP32-P4-WIFI6-Touch-LCD-7, -8, and -10.1 boards. It keeps the Brookesia phone launcher and bundled applications, including Wi-Fi through ESP-Hosted, camera, audio, music, video, drawing, spectrum analysis, settings, and Xiaozhi.

## Requirements

- ESP-IDF v5.5.5
- ESP32-P4 Rev3.x only

## Display variants

| Variant | Panel | MIPI-DSI lane rate | Resolution | Controller | DPI clock |
| --- | --- | ---: | --- | --- | ---: |
| `lcd-7` | 7 inch | 1000 Mbps | 720 x 1280 | ILI9881C | 80 MHz |
| `lcd-8` | 8 inch | 1500 Mbps | 800 x 1280 | JD9365 | 80 MHz |
| `lcd-10-1` | 10.1 inch | 1500 Mbps | 800 x 1280 | JD9365 | 80 MHz |

The local BSP uses an explicit MIPI PHY clock branch: pre-v3 uses `MIPI_DSI_PHY_CLK_SRC_DEFAULT`; Rev3.x uses `MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT`.

## Touch initialization

The touch controller runs without assigned INT or RST GPIOs. The BSP probes
I2C address `0x5D` and then `0x14`, creates the panel IO with the detected
address, and reads touch data by polling. Do not add an INT/RST address-selection
sequence unless the hardware contract is changed separately.

## Build

Run one command from this directory after exporting ESP-IDF v5.5.5:

```bash
idf.py -B build-lcd-7-v5.5.5-rev3_x -D SDKCONFIG="$PWD/build-lcd-7-v5.5.5-rev3_x/sdkconfig" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.rev3_x;sdkconfig.defaults.lcd-7" build
idf.py -B build-lcd-8-v5.5.5-rev3_x -D SDKCONFIG="$PWD/build-lcd-8-v5.5.5-rev3_x/sdkconfig" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.rev3_x;sdkconfig.defaults.lcd-8" build
idf.py -B build-lcd-10-1-v5.5.5-rev3_x -D SDKCONFIG="$PWD/build-lcd-10-1-v5.5.5-rev3_x/sdkconfig" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.rev3_x;sdkconfig.defaults.lcd-10-1" build
```

The application binary is named `esp32-p4-lcd-x-brookesia.bin`. The checked-in Rev3.x merged images follow the LCD-5 factory naming convention: `ESP32-P4-WIFI6-Touch-LCD-7-FactoryOnly-260820.bin`, `ESP32-P4-WIFI6-Touch-LCD-8-FactoryOnly-260820.bin`, and `ESP32-P4-WIFI6-Touch-LCD-10.1-FactoryOnly-260820.bin`.

## Merge factory images

Run the matching command after a successful build. Each command creates a 16 MiB image that is ready to flash at offset `0x0`:

```bash
(cd build-lcd-7-v5.5.5-rev3_x && python -m esptool --chip esp32p4 merge_bin -o ../../ESP32-P4-WIFI6-Touch-LCD-7-FactoryOnly-260820.bin @flash_args)
(cd build-lcd-8-v5.5.5-rev3_x && python -m esptool --chip esp32p4 merge_bin -o ../../ESP32-P4-WIFI6-Touch-LCD-8-FactoryOnly-260820.bin @flash_args)
(cd build-lcd-10-1-v5.5.5-rev3_x && python -m esptool --chip esp32p4 merge_bin -o ../../ESP32-P4-WIFI6-Touch-LCD-10.1-FactoryOnly-260820.bin @flash_args)
```
