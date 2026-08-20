# ESP32-C6 Hosted Wi-Fi 兼容性说明

[English](p4-c6-hosted-wifi.md)

ESP32-P4 没有内置 Wi-Fi 或蓝牙射频。在 ESP32-P4-WIFI6-Touch-LCD-X 系列开发板上，
ESP32-C6-MINI-1U-H8 模块通过 SDIO 使用乐鑫 Hosted 网络协议栈提供 2.4 GHz Wi-Fi 6
和 Bluetooth 5（LE）。

## 示例依赖

[`examples/esp-idf/04_wifistation`](../examples/esp-idf/04_wifistation/)
声明了本仓库使用的 Hosted 协议栈组件版本范围：

| 组件 | ESP-IDF v5.5 | ESP-IDF v6.0 |
| --- | --- | --- |
| `espressif/esp_wifi_remote` | `0.14.*` | `>=1.6,<2.0` |
| `espressif/esp_hosted` | `1.4.*` | `>=2.12,<3.0` |

CI 使用 ESP-IDF `v5.5.5` 和 `v6.0.2` 为 `esp32p4` 构建该示例。

## C6 固件假设

C6 模块运行出厂提供的 Hosted 固件，主机侧组件版本范围必须与该固件版本保持兼容。
本仓库暂未包含 C6 从机固件镜像和源码，后续更新中可能会补充。如果更换了 C6 固件，
需要同步重新验证主机侧组件版本范围和本说明。
