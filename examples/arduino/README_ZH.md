# Arduino 示例

[English](README.md)

本目录提供 ESP32-P4-WIFI6-Touch-LCD-X 7 英寸、8 英寸和 10.1 英寸型号的 10 个
第一方 Arduino 示例。显示示例使用与 LCD-5 一致的随仓库库：
`GFX_Library_for_Arduino`、`displays`、LVGL `9.3.0` 和 `lv_conf.h`。
所有示例都使用 Arduino-ESP32 API，且不依赖尚未发布到组件仓库的组件。

## 示例

| 示例 | 用途 |
| --- | --- |
| [`01_HelloWorld`](examples/01_HelloWorld/) | 最小 Arduino_GFX MIPI-DSI 显示初始化。 |
| [`02_AsciiTable`](examples/02_AsciiTable/) | Arduino_GFX 功能与基准测试字符表。 |
| [`03_Drawing_board`](examples/03_Drawing_board/) | GT9271 五点电容触摸绘图。 |
| [`04_LVGLV9_Arduino`](examples/04_LVGLV9_Arduino/) | 带触摸输入的 LVGL 9 控件界面。 |
| [`05_GFX_ESPWiFiAnalyzer`](examples/05_GFX_ESPWiFiAnalyzer/) | 通过板载 ESP32-C6 协处理器图形化扫描 Wi-Fi。 |
| [`06_Camera_Preview`](examples/06_Camera_Preview/) | 在显示屏上预览 OV5647 MIPI-CSI 摄像头。 |
| [`07_Camera_ISP_Tuning`](examples/07_Camera_ISP_Tuning/) | 通过串口交互式 ISP/3A 调参的实时预览。 |
| [`08_SD_Card`](examples/08_SD_Card/) | 通过 SDIO 3.0 卡槽读写 MicroSD。 |
| [`09_Audio_Playback`](examples/09_Audio_Playback/) | ES8311 编解码器以不同频率音调播放《致爱丽丝》开头。 |
| [`10_Mic_Record`](examples/10_Mic_Record/) | ES7210 麦克风采集；通过串口输出峰值、RMS 和抽取样本。 |

每个示例均为 3 种显示型号构建，合计 30 个编译项。8 英寸和 10.1 英寸型号虽然
使用同一控制器系列，仍保留独立的面板初始化表。只有 `04_LVGLV9_Arduino` 使用
LVGL `9.3.0` 的随仓库配置。

## 构建

安装 Arduino CLI 与 Arduino-ESP32 `3.3.11`。在 Arduino IDE 中编译前，请将本仓库
[`libraries/`](libraries/) 中的 `GFX_Library_for_Arduino/`、`displays/`、`lvgl/` 和
`lv_conf.h` 复制到 Arduino sketchbook 的 `libraries/` 目录，复制后的结构必须为：

```text
<sketchbook>/libraries/GFX_Library_for_Arduino/
<sketchbook>/libraries/displays/
<sketchbook>/libraries/lvgl/
<sketchbook>/libraries/lv_conf.h
```

不要多套一层 `libraries/libraries/`。

随仓库配置默认选择 LCD-10.1（`101`）。使用 Arduino IDE 时，请在编译前编辑
`<sketchbook>/libraries/displays/displays_config.h` 中的
`LCD_X_DISPLAY_VARIANT`，LCD-7、LCD-8 和 LCD-10.1 分别设为 `7`、`8` 和
`101`。应在该配置文件中设置，而不是只在某个草图内定义，以确保显示和触摸库的所有
源码都获得同一型号值。

Arduino-ESP32 core 提供示例使用的标准 `WiFi`、`ESP_Video`、`SD_MMC`、I2C 和 I2S
API。音频示例使用的 ES8311 与 ES7210 驱动源码随对应示例提供。若要在本仓库中用
Arduino CLI 编译，请使用 CI 的 ESP32-P4 Rev3.x 板级选项：

```sh
arduino-cli compile \
  --fqbn 'esp32:esp32:esp32p4:ChipVariant=postv3,PSRAM=enabled,FlashSize=32M,FlashMode=qio,FlashFreq=80,PartitionScheme=app13M_data7M_32MB,UploadMode=default,UploadSpeed=921600' \
  --libraries examples/arduino/libraries \
  --build-property 'compiler.cpp.extra_flags=-DLCD_X_DISPLAY_VARIANT=7' \
  examples/arduino/examples/01_HelloWorld
```

LCD-7、LCD-8 和 LCD-10.1 分别将 `LCD_X_DISPLAY_VARIANT` 设为 `7`、`8` 和
`101`。CI 对每个示例使用同一 FQBN 与随仓库提供的库路径。`postv3` 是维护中的
Rev3.x 默认配置；不要将构建结果烧录到采用不同芯片修订版约定的开发板。更换示例路径
和显示型号值后，同一命令也可构建 `04_LVGLV9_Arduino`。

## 示例使用的板载外设

| 外设 | 板级连接 | 示例 |
| --- | --- | --- |
| 显示 | 双通道 MIPI-DSI；复位 GPIO27 | `01`–`07` |
| 触摸 | I2C SDA GPIO7 / SCL GPIO8；`INT` 与 `RST` 均为 `GPIO_NUM_NC` | `03`、`04` |
| 摄像头 | OV5647 MIPI-CSI；SCCB SDA GPIO7 / SCL GPIO8 | `06`、`07` |
| Hosted Wi-Fi | 通过 SDIO 连接 ESP32-C6 协处理器 | `05` |
| MicroSD | SDMMC 4-bit：CLK GPIO43、CMD GPIO44、D0 GPIO39、D1 GPIO40、D2 GPIO41、D3 GPIO42 | `08` |
| 音频控制 | I2C SDA GPIO7 / SCL GPIO8 | `09`、`10` |
| 音频数据 | I2S MCLK GPIO13、BCLK GPIO12、LRCK GPIO10、DOUT GPIO9、DIN GPIO11；功放使能 GPIO53 | `09`、`10` |

I2C 设备共用板载总线。摄像头、无线、存储和音频示例均需要对应的板载外设；运行这些
示例时不要复用表中列出的引脚。`05_GFX_ESPWiFiAnalyzer` 需要 ESP32-C6 协处理器中与出厂版本
兼容的 Hosted 固件；请参阅 [ESP32-C6 Hosted Wi-Fi 兼容性说明](../../docs/p4-c6-hosted-wifi_ZH.md)。
音频示例保留 LCD-5 的编解码器与 I2S 示例设置。在依赖扬声器输出或采集数据前，仍需
在实机上确认扬声器电平和麦克风/声道映射。

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
