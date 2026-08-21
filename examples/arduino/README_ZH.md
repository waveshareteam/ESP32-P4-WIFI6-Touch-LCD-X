# Arduino 示例

[English](README.md)

本目录提供 ESP32-P4-WIFI6-Touch-LCD-X 7 英寸、8 英寸和 10.1 英寸型号的 10 个
第一方 Arduino 示例。涉及显示的示例使用随仓库提供的 `lcd_x` 库；共享的
[`libraries/`](libraries/) 目录还随仓库提供 LVGL `9.5.0` 及其 `lv_conf.h` 配置。
所有示例都使用 Arduino-ESP32 API，且不依赖尚未发布到组件仓库的组件。

## 示例

| 示例 | 用途 |
| --- | --- |
| [`01_DisplayColorBars`](examples/01_DisplayColorBars/) | 初始化所选 MIPI-DSI 面板并绘制色条。 |
| [`02_TouchDrawing`](examples/02_TouchDrawing/) | 轮询 GT9271 触摸输入并绘制触点。 |
| [`03_AsciiTable`](examples/03_AsciiTable/) | 使用显示文本 API 绘制可打印 ASCII 字符表。 |
| [`04_LVGLV9`](examples/04_LVGLV9/) | 运行使用轮询触摸的 LVGL 9.5.0 显示演示。 |
| [`05_WiFiAnalyzer`](examples/05_WiFiAnalyzer/) | 通过 ESP32-C6 Hosted 连接扫描并显示附近 Wi-Fi 网络。 |
| [`06_CameraPreview`](examples/06_CameraPreview/) | 在所选 LCD 上显示板载 OV5647 MIPI-CSI 摄像头预览。 |
| [`07_CameraISPTuning`](examples/07_CameraISPTuning/) | 预览摄像头，并通过串口提供受支持的 ISP 调参控制。 |
| [`08_SDCard`](examples/08_SDCard/) | 挂载 MicroSD 卡并执行基本文件访问。 |
| [`09_AudioPlayback`](examples/09_AudioPlayback/) | 通过 ES8311 编解码器和扬声器通路播放合成旋律，默认从音量 60 开始。 |
| [`10_MicRecord`](examples/10_MicRecord/) | 采集 ES7210 立体声 MIC1/MIC2 数据，并通过串口分别报告两路统计。 |

每个示例均为 3 种显示型号构建，合计 30 个编译项。8 英寸和 10.1 英寸型号虽然
使用同一控制器系列，仍保留独立的面板初始化表。只有 `04_LVGLV9` 使用 LVGL
`9.5.0` 的随仓库配置。

## 构建

安装 Arduino CLI 与 Arduino-ESP32 `3.3.11`。在 Arduino IDE 中编译前，请将本仓库
[`libraries/`](libraries/) 目录中的**内容**复制到 Arduino sketchbook 的 `libraries`
目录，复制后的结构必须为：

```text
<sketchbook>/libraries/lcd_x/
<sketchbook>/libraries/lvgl/
<sketchbook>/libraries/lv_conf.h
```

不要多套一层 `libraries/libraries/`。这样即可获得 `lcd_x`、LVGL `9.5.0` 及其配置，
无需通过 Library Manager 单独安装。

Arduino-ESP32 core 提供示例使用的标准 `WiFi`、`ESP_Video`、`SD_MMC`、I2C 和 I2S
API。音频示例使用的 ES8311 与 ES7210 驱动源码随对应示例提供。若要在本仓库中用
Arduino CLI 编译，请使用 CI 的 ESP32-P4 Rev3.x 板级选项：

```sh
arduino-cli compile \
  --fqbn 'esp32:esp32:esp32p4:ChipVariant=postv3,PSRAM=enabled,FlashSize=32M,FlashMode=qio,FlashFreq=80,PartitionScheme=app13M_data7M_32MB,UploadMode=default,UploadSpeed=921600' \
  --libraries examples/arduino/libraries \
  --build-property 'compiler.cpp.extra_flags=-DLCD_X_DISPLAY_VARIANT=7' \
  examples/arduino/examples/01_DisplayColorBars
```

LCD-7、LCD-8 和 LCD-10.1 分别将 `LCD_X_DISPLAY_VARIANT` 设为 `7`、`8` 和
`101`。CI 对每个示例使用同一 FQBN 与随仓库提供的库路径；只有涉及显示的示例会
使用 `lcd_x`。`postv3` 是维护中的 Rev3.x 默认配置；不要将构建结果烧录到采用不同
芯片修订版约定的开发板。更换示例路径和显示型号值后，同一命令也可构建
`04_LVGLV9`。

## 示例使用的板载外设

| 外设 | 板级连接 | 示例 |
| --- | --- | --- |
| 显示 | 双通道 MIPI-DSI；复位 GPIO27 | `01`–`07` |
| 触摸 | I2C SDA GPIO7 / SCL GPIO8；不指定 `INT` 与 `RST` | `02`、`04` |
| 摄像头 | OV5647 MIPI-CSI；SCCB SDA GPIO7 / SCL GPIO8 | `06`、`07` |
| Hosted Wi-Fi | 通过 SDIO 连接 ESP32-C6 协处理器 | `05` |
| MicroSD | SDMMC 4-bit：CLK GPIO43、CMD GPIO44、D0 GPIO39、D1 GPIO40、D2 GPIO41、D3 GPIO42 | `08` |
| 音频控制 | I2C SDA GPIO7 / SCL GPIO8 | `09`、`10` |
| 音频数据 | I2S MCLK GPIO13、BCLK GPIO12、LRCK GPIO10、DOUT GPIO9、DIN GPIO11；功放使能 GPIO53 | `09`、`10` |

I2C 设备共用板载总线。摄像头、无线、存储和音频示例均需要对应的板载外设；运行这些
示例时不要复用表中列出的引脚。`05_WiFiAnalyzer` 需要 ESP32-C6 协处理器中与出厂版本
兼容的 Hosted 固件；请参阅 [ESP32-C6 Hosted Wi-Fi 兼容性说明](../../docs/p4-c6-hosted-wifi_ZH.md)。
运行 `09_AudioPlayback` 时请先保持较低扬声器音量，再按需调整。麦克风示例以 24 dB
选择两个前置麦克风 MIC1 与 MIC2，并从编解码器的立体声 SDOUT1 数据流分别报告两路
统计；它不启用 MIC3 回声参考通路或未连接的 MIC4 通路。

## 显示与触摸行为

显示屏复位引脚为 GPIO27，与触摸控制器独立。触摸库刻意不指定触摸 `INT` 和 `RST`
（均为 `GPIO_NUM_NC`），因此不会执行 GT911 地址选择时序。I2C 可用后，库会先探测
`0x5D`，再探测 `0x14`，并将设备句柄绑定到实际响应的地址。触摸仅读取轮询数据，
不会注册 GPIO 中断回调。示例使用 GT911 兼容的 GT9271 接口，并最多报告五个触点，
与当前第一方驱动支持范围一致。

## CI 与验证边界

[Arduino 示例工作流](../../.github/workflows/arduino-examples.yml) 使用 Arduino CLI
`1.5.1` 和 Arduino-ESP32 `3.3.11`，以 `postv3` Rev3.x 板级配置为全部 10 个示例的
三种型号编译（30 行）。该工作流不发布固件或可下载构建产物。编译成功仅验证源码/API
集成，不验证开发板上的显示初始化、I2C 连线、触摸坐标、摄像头流、Hosted Wi-Fi、
MicroSD 访问、音频行为或其他硬件行为。
