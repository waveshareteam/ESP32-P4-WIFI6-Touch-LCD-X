| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# Wi-Fi 站点示例

[English](./README.md)

本示例通过板载 ESP32-C6 模块和乐鑫 Hosted 网络协议栈
（`esp_hosted` 与 `esp_wifi_remote`）让 ESP32-P4 连接 Wi-Fi 网络。

## 使用方法

1. 设置目标芯片：

   ```
   idf.py set-target esp32p4
   ```

2. 配置 Wi-Fi 凭据：

   ```
   idf.py menuconfig
   ```

   打开 `Example Configuration`，按需设置 `WiFi SSID`、`WiFi Password`、
   最大重试次数和 WPA3 SAE 选项。

3. 构建、烧录并监视：

   ```
   idf.py -p PORT build flash monitor
   ```

本示例使用的 Hosted 协议栈版本约定请参见
[ESP32-C6 Hosted Wi-Fi 兼容性说明](../../../docs/p4-c6-hosted-wifi_ZH.md)。
