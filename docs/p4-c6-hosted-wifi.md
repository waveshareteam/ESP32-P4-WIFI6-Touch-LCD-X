# ESP32-C6 Hosted Wi-Fi Compatibility

[简体中文](p4-c6-hosted-wifi_ZH.md)

The ESP32-P4 has no built-in Wi-Fi or Bluetooth radio. On the
ESP32-P4-WIFI6-Touch-LCD-X boards an ESP32-C6-MINI-1U-H8 module provides
2.4 GHz Wi-Fi 6 and Bluetooth 5 (LE) over SDIO using the Espressif hosted
networking stack.

## Example Dependencies

[`examples/esp-idf/04_wifistation`](../examples/esp-idf/04_wifistation/)
declares the hosted-stack component ranges used by this repository:

| Component | ESP-IDF v5.5 | ESP-IDF v6.0 |
| --- | --- | --- |
| `espressif/esp_wifi_remote` | `0.14.*` | `>=1.6,<2.0` |
| `espressif/esp_hosted` | `1.4.*` | `>=2.12,<3.0` |

CI builds the example for `esp32p4` with ESP-IDF `v5.5.5` and `v6.0.2`.

## C6 Firmware Assumption

The C6 module runs the factory-provided hosted firmware. The host-side
component ranges above must remain compatible with that firmware version.
The C6 slave firmware images and source are not included in this repository
yet and may be added in a later update. If the C6 firmware is changed, the
host-side ranges and this note must be re-validated together.
