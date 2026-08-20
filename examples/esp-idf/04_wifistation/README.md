| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# Wi-Fi Station

[中文版本](./README_ZH.md)

This example connects the ESP32-P4 to a Wi-Fi network through the onboard
ESP32-C6 module using the Espressif hosted networking stack
(`esp_hosted` and `esp_wifi_remote`).

## How to Use the Example

1. Set the target:

   ```
   idf.py set-target esp32p4
   ```

2. Configure the Wi-Fi credentials:

   ```
   idf.py menuconfig
   ```

   Open `Example Configuration` and set `WiFi SSID`, `WiFi Password`, the
   maximum retry count, and the WPA3 SAE options as needed.

3. Build, flash, and monitor:

   ```
   idf.py -p PORT build flash monitor
   ```

See
[ESP32-C6 Hosted Wi-Fi Compatibility](../../../docs/p4-c6-hosted-wifi.md)
for the hosted-stack version contract used by this example.
