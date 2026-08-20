| 支持的目标芯片 | ESP32-P4 |
| -------------- | -------- |

# MIPI-DSI 彩条测试

[English](./README.md)

本示例初始化开发板的 MIPI-DSI 显示屏，打开背光，并让 LCD DPI 驱动显示垂直
彩条测试图案。它用于基础显示点亮验证，不使用 LVGL。

## 选择屏幕型号

运行 `idf.py menuconfig`，在
`Board Support Package (ESP32-P4) > Display > Select LCD type` 中选择与
开发板相符的屏幕：

- 7 英寸，720 × 1280，ILI9881C
- 8 英寸，800 × 1280，JD9365
- 10.1 英寸，800 × 1280，JD9365（默认）

三个型号均使用开发板的双通道 MIPI-DSI 接口。不能混用其他屏幕尺寸的初始化
表或分辨率。

## 构建、烧录和监视

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

稳定的全屏垂直彩条可验证屏幕、MIPI-DSI、复位和背光的基础初始化，但不能
证明触摸功能正常。
