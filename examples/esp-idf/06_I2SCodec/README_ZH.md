| 支持的目标芯片 | ESP32-P4 |
| -------------- | -------- |

# I2S 编解码器示例

[English](./README.md)

本示例初始化板载 ES8311 编解码器和 ESP32-P4 I2S 外设。默认 `music` 模式会
循环播放嵌入固件的 `canon.pcm` 音频；可选 `echo` 模式会读取编解码器输入并
将采样数据回送到输出。

## 开发板连接

| 功能 | GPIO |
| --- | ---: |
| I2C SCL / SDA | 8 / 7 |
| I2S MCLK / BCLK / LRCK | 13 / 12 / 10 |
| I2S 数据输出 / 输入 | 9 / 11 |
| 功放使能 | 53 |

这些引脚与开发板原理图一致。本 ESP32-P4 产品默认应使用直接硬件配置；只有
明确选择了兼容 BSP 依赖后，才应启用默认关闭的通用 BSP 选项。

## 配置与使用

`Example Configuration` 可选择 `music` 或 `echo` 模式，并调整麦克风增益和
输出音量。首次运行请使用适中音量，并避免扬声器与麦克风形成强烈声反馈。

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```
