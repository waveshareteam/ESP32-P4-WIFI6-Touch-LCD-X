| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# 视频显示示例

[English](./README.md)

本示例使用
[`esp_video`](https://github.com/espressif/esp-video-components/tree/master/esp_video)
组件和板级 BSP，将板载 MIPI-CSI 摄像头（OV5647）画面显示到 MIPI-DSI 屏幕。

## 使用方法

1. 选择摄像头传感器和输入格式：

   ```
   idf.py menuconfig
   ```

   在 `Espressif Camera Sensors Configurations` 中启用与实际摄像头模块匹配的
   传感器。示例默认使用 `OV5647`，输入格式为 `RAW8 800x1280 50fps`。

2. 在 `Board Support Package (ESP32-P4) > Display > Select LCD type` 中选择
   与实际开发板匹配的屏幕型号（7 英寸为 ILI9881C，8 / 10.1 英寸为 JD9365）。

3. 构建、烧录并监视：

   ```
   idf.py -p PORT build flash monitor
   ```

本示例需要使用板载 OV5647 摄像头或兼容的 MIPI-CSI 摄像头模块。
