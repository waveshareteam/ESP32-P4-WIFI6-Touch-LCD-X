# ESP-IDF Examples

[中文版本](./README_ZH.md)

This directory contains the 12 first-party ESP-IDF projects for the
ESP32-P4-WIFI6-Touch-LCD-X product family. Each project is a direct child of
this directory; embedded component test applications are not product examples.

| Example | Focus |
| --- | --- |
| [`01_HowToCreateProject`](./01_HowToCreateProject/) | Minimal project template |
| [`02_HelloWorld`](./02_HelloWorld/) | Chip information and restart loop |
| [`03_i2c_tools`](./03_i2c_tools/) | Interactive I2C console |
| [`04_wifistation`](./04_wifistation/) | ESP32-C6 Hosted Wi-Fi station |
| [`05_sdmmc`](./05_sdmmc/) | MicroSD access through SDMMC |
| [`06_I2SCodec`](./06_I2SCodec/) | ES8311 and I2S audio |
| [`07_Displaycolorbar`](./07_Displaycolorbar/) | MIPI-DSI color-bar test |
| [`08_lvgl_demo_v9`](./08_lvgl_demo_v9/) | LVGL 9 benchmark and touch |
| [`09_video_lcd_display`](./09_video_lcd_display/) | MIPI-CSI camera preview |
| [`10_mp4_player`](./10_mp4_player/) | MP4 and AVI playback from MicroSD |
| [`11_esp_brookesia_phone`](./11_esp_brookesia_phone/) | ESP-Brookesia phone UI |
| [`12_usb_extend_screen`](./12_usb_extend_screen/) | USB display, HID touch, and UAC audio |

The repository workflow builds every project for `esp32p4` with the current
ESP-IDF v5.5 and v6 release lines. Documentation-only changes run discovery
and documentation checks without starting the product build matrix.

## Silicon Revision

Every example defaults to ESP32-P4 Rev3.x (`CONFIG_ESP32P4_REV_MIN_300=y`),
matching the current product hardware and CI artifacts. ESP32-P4 Rev1.3 belongs
to ESP-IDF's mutually exclusive pre-v3 group and requires the explicit
`config/sdkconfig/rev1_3.defaults` overlay. Do not reuse binaries between the
two revision groups. See [ESP32-P4 Silicon
Revisions](../../docs/silicon-revisions.md) for the configuration, MIPI-DSI
clock, CPU default, detection, and build instructions.

Read the selected project's README, choose the correct display variant when
applicable, and verify hardware-facing settings against the repository
[schematics](../../schematic/).
