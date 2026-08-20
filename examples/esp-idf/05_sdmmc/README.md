| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# SDMMC Card Example

[中文版本](./README_ZH.md)

This example mounts a MicroSD card at `/sdcard` through the ESP32-P4 SDMMC
host, prints card information, writes and reads test files, renames a file, and
unmounts the filesystem.

## Board Configuration

The default four-line bus uses the board connections from the
[schematic](../../../schematic/ESP32-P4-WIFI6-Touch-LCD-X-Schematic.pdf):

| Signal | GPIO |
| --- | ---: |
| CMD | 44 |
| CLK | 43 |
| D0 | 39 |
| D1 | 40 |
| D2 | 41 |
| D3 | 42 |

The ESP32-P4 configuration uses internal LDO channel 4 for SD I/O power. Keep
this setting aligned with the board schematic. SD bus pull-ups are required;
the internal pull-ups enabled by the example are intended only as diagnostic
support.

## Data Safety

`Format the card if mount failed` and `Format the card as a part of the
example` are disabled by default. Enabling either option can erase data on the
card. In one-line mode, D3 still requires a pull-up so that the card does not
enter SPI mode.

## Build, Flash, and Monitor

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```
