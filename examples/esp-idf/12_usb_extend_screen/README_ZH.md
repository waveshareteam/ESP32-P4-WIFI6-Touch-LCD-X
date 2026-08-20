| 支持的目标芯片 | ESP32-P4 |
| -------------- | -------- |

# USB 扩展屏

[English](./README.md)

本示例将 ESP32-P4 USB 2.0 High-Speed 设备接口组合为厂商自定义显示传输、
HID 触摸设备和 USB Audio Class 设备。接收到的 RGB565、RGB888、YUV420 或
JPEG 帧会按需解码，并绘制到所选 MIPI-DSI 屏幕。

## 默认功能

- High-Speed TinyUSB 设备模式
- 厂商自定义帧传输，单帧上限 300,000 字节
- 默认启用 HID 触摸上报
- 默认启用 USB Audio Class 扬声器与麦克风通道
- RGB565 LCD 输出、两个显示缓冲区和 60 FPS 上限

可在 `Example Configuration` 中关闭 HID 触摸或 UAC 音频；最终参与编译的
USB 类别还取决于 TinyUSB 描述符配置。

## 配置开发板

在 `Board Support Package (ESP32-P4) > Display > Select LCD type` 中选择
与开发板对应的 7、8 或 10.1 英寸屏。仓库内 P4 默认配置选择 10.1 英寸屏。
接收帧必须与当前屏幕宽高一致，并从坐标 `(0, 0)` 开始。

设备连接应使用开发板的 USB 2.0 OTG High-Speed Type-C 接口；USB 转 UART
接口用于烧录和串口监视。

## 构建、烧录和监视

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

主机端帧发送器属于独立集成面。本仓库示例验证设备端实现，但不打包桌面主机
应用，也不会替代仓库中受版本控制的出厂固件。
