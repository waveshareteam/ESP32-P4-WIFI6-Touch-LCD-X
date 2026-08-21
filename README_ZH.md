<div align="center">
  <h1>ESP32-P4-WIFI6-Touch-LCD-X</h1>
  <p><strong>ESP32-P4 7 / 8 / 10.1 英寸 MIPI-DSI 触控屏开发板</strong></p>
  <p>
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/actions/workflows/esp-idf-examples.yml"><img alt="ESP-IDF 示例" src="https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/actions/workflows/esp-idf-examples.yml/badge.svg"></a>
    <a href="https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/actions/workflows/arduino-examples.yml"><img alt="Arduino 示例" src="https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/actions/workflows/arduino-examples.yml/badge.svg"></a>
    <a href="LICENSE.txt"><img alt="许可证" src="https://img.shields.io/github/license/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X"></a>
  </p>
  <p>
    <a href="README.md">English</a> ·
    <a href="https://www.waveshare.com/esp32-p4-wifi6-touch-lcd-7-8-10.1.htm">🌐 产品页面</a> ·
    <a href="https://docs.waveshare.net/ESP32-P4-WIFI6-Touch-LCD-X/">📚 中文文档</a> ·
    <a href="firmware/">📦 出厂固件</a> ·
    <a href="examples/esp-idf/">🧩 ESP-IDF 示例</a> ·
    <a href="examples/arduino/">🔧 Arduino 示例</a>
  </p>
  <p>
    <img src="assets/ESP32-P4-WIFI6-Touch-LCD-X.jpg" alt="微雪 ESP32-P4-WIFI6-Touch-LCD-X 产品系列" width="760">
  </p>
</div>

---

## ✨ 产品简介

ESP32-P4-WIFI6-Touch-LCD-X 系列将 ESP32-P4 多媒体处理器、ESP32-C6
无线协处理器和电容式 MIPI-DSI 触摸显示屏集成于一体，提供 7、8 和 10.1
英寸三种规格，适用于智能家居终端、工业控制、边缘计算、多媒体和交互式
HMI 应用。

本仓库提供第一方 ESP-IDF 与 Arduino 示例、出厂烧录固件以及开发板原理图。
最新规格和包装清单请以官方商品页为准。

## 📋 产品型号

| 产品 | 显示屏 | 驱动芯片 | SKU |
| --- | --- | --- | ---: |
| [ESP32-P4-WIFI6-Touch-LCD-7](https://www.waveshare.net/shop/ESP32-P4-WIFI6-Touch-LCD-7.htm) | 7 英寸，720 × 1280 | ILI9881C | 30738 |
| [ESP32-P4-WIFI6-Touch-LCD-8](https://www.waveshare.net/shop/ESP32-P4-WIFI6-Touch-LCD-8.htm) | 8 英寸，800 × 1280 | JD9365 | 33673 |
| [ESP32-P4-WIFI6-Touch-LCD-10.1](https://www.waveshare.net/shop/ESP32-P4-WIFI6-Touch-LCD-10.1.htm) | 10.1 英寸，800 × 1280 | JD9365 | 33672 |

三个型号均使用双通道 MIPI-DSI 显示接口和 GT9271 十点电容触摸控制器，
屏幕方向可通过软件配置。

## 🖥️ 硬件概览

| 功能 | 器件 / 接口 |
| --- | --- |
| 主处理器 | ESP32-P4NRW32，高性能双核 RISC-V + 低功耗单核 RISC-V |
| 存储 | 封装内 32 MB PSRAM，板载 32 MB NOR Flash |
| 无线连接 | ESP32-C6-MINI-1U-H8，通过 SDIO 提供 2.4 GHz Wi-Fi 6 和 Bluetooth 5 LE（[Hosted Wi-Fi 兼容性说明](docs/p4-c6-hosted-wifi_ZH.md)） |
| 显示 | 7 / 8 / 10.1 英寸 IPS 屏，双通道 MIPI-DSI |
| 触摸 | GT9271 十点电容触摸控制器 |
| 摄像头 | 板载 500 万像素 OV5647 前置摄像头，使用双通道 MIPI-CSI |
| 音频 | ES8311 音频编解码、ES7210 回声消除电路、双麦克风和扬声器 |
| 存储扩展 | SDIO 3.0 MicroSD 卡槽 |
| USB | USB 转 UART Type-C 接口和 USB 2.0 OTG High-Speed Type-C 接口 |
| 扩展 | 40PIN 排座、电池接口、摄像头/显示接口及板对板连接接口 |

GPIO 分配和电路细节请查阅仓库内原理图：

- [ESP32-P4-WIFI6-Touch-LCD-X 原理图](schematic/ESP32-P4-WIFI6-Touch-LCD-X-Schematic.pdf)
- [ESP32-P4-Connect-Adapter 原理图](schematic/ESP32-P4-Connect-Adapter-Schematic.pdf)

## 🚀 快速开始

1. 确认开发板所配屏幕尺寸和产品型号。
2. 按照官方 [ESP-IDF 环境搭建与示例说明](https://docs.waveshare.net/ESP32-P4-WIFI6-Touch-LCD-X/Development-Environment-Setup-IDF/)
   配置开发环境。
3. 选择 [`examples/esp-idf/`](examples/esp-idf/) 下的工程，将目标芯片设置为
   `esp32p4`，并阅读该示例的 README 或配置选项。示例默认面向 ESP32-P4
   Rev3.x；只有旧款 Rev1.3 硬件才应显式选择对应的
   [芯片修订版配置](docs/silicon-revisions_ZH.md)。
4. 对于显示类示例，在 `menuconfig` 的
   `Board Support Package (ESP32-P4) > Display > Select LCD type`
   中选择正确的屏幕型号。
5. 按照官方文档使用 ESP-IDF 工具构建、烧录并监视所选示例。

部分多媒体示例需要额外硬件或媒体文件，烧录前请先阅读示例内的说明。

## 🧪 ESP-IDF 示例

| 示例 | 功能 |
| --- | --- |
| [`01_HowToCreateProject`](examples/esp-idf/01_HowToCreateProject/) | 最小 ESP-IDF 工程模板 |
| [`02_HelloWorld`](examples/esp-idf/02_HelloWorld/) | 基础应用与芯片信息 |
| [`03_i2c_tools`](examples/esp-idf/03_i2c_tools/) | 交互式 I2C 总线工具 |
| [`04_wifistation`](examples/esp-idf/04_wifistation/) | 通过 ESP32-C6 Hosted 连接 Wi-Fi |
| [`05_sdmmc`](examples/esp-idf/05_sdmmc/) | 通过 SDMMC 访问 MicroSD 卡 |
| [`06_I2SCodec`](examples/esp-idf/06_I2SCodec/) | ES8311 编解码器与 I2S 音频 |
| [`07_Displaycolorbar`](examples/esp-idf/07_Displaycolorbar/) | MIPI-DSI 显示色条 |
| [`08_lvgl_demo_v9`](examples/esp-idf/08_lvgl_demo_v9/) | LVGL 9 显示与触摸演示 |
| [`09_video_lcd_display`](examples/esp-idf/09_video_lcd_display/) | 在 LCD 上预览 MIPI-CSI 摄像头画面 |
| [`10_mp4_player`](examples/esp-idf/10_mp4_player/) | MP4 / AVI 媒体播放；请查看示例硬件说明 |
| [`11_esp_brookesia_phone`](examples/esp-idf/11_esp_brookesia_phone/) | ESP-Brookesia 手机风格 UI |
| [`12_usb_extend_screen`](examples/esp-idf/12_usb_extend_screen/) | USB 扩展屏、触摸和音频功能 |

## 🔧 Arduino

[`examples/arduino/`](examples/arduino/) 提供 10 个第一方示例，涵盖显示、触摸、
文本、LVGL 9、Hosted Wi-Fi 分析、摄像头预览与 ISP 调参、MicroSD、音频播放和
麦克风采集。其随仓库提供的库结构与 LCD-5 一致：`GFX_Library_for_Arduino`、
`displays`、LVGL `9.3.0` 和 `lv_conf.h`。全部 10 个示例均会针对 LCD-7、LCD-8
和 LCD-10.1 进行编译检查。完整的示例清单、外设引脚、
Arduino-ESP32 `3.3.11` FQBN 与型号构建方式请参阅
[Arduino 示例说明](examples/arduino/README_ZH.md)。

显示屏复位引脚为 GPIO27。触摸 `INT` 与 `RST` 刻意保持为 `GPIO_NUM_NC`；库会先探测
`0x5D`，再探测 `0x14`，绑定实际响应地址，并且只使用轮询。编译结果不验证开发板上的
显示或触摸行为。

## 🤖 持续集成

[ESP-IDF 示例工作流](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/actions/workflows/esp-idf-examples.yml)
会发现 12 个第一方工程，并使用 ESP-IDF `v5.5.5` 和 `v6.0.2` 为
`esp32p4` 分别构建。6 个显示示例会针对 7 英寸、8 英寸和 10.1 英寸显示型号展开，
完整运行共 48 个源码构建项。仅 `04_wifistation` 和 `05_sdmmc` 使用非空的
`sdkconfig.ci` 覆盖层；显示示例还会使用对应 LCD 覆盖层。ESP-IDF v5.5.5 仅编译，
18 个 v6.0.2 显示行会上传保留 14 天的[CI 构建产物](docs/ci-artifacts_ZH.md)。
全部 48 个构建项和 18 个可下载示例产物均使用 `rev3_x` 配置。Rev1.3 与 Rev3.x
是互斥的 ESP-IDF 构建目标；为旧版开发板构建前请先阅读
[ESP32-P4 芯片修订版说明](docs/silicon-revisions_ZH.md)。独立的文档工作流会运行完整
Markdown 门禁，检查归属、中英文配对与同语言链接、首页对称性、仓库内链接和
公开文本隐私；同时验证纯文档范围。因此示例发现作业会在每个拉取请求中保持
可见，但纯文档改动和仅涉及出厂固件的改动不会选择任何示例构建。固定名称的
`ESP-IDF CI gate` 会聚合发现结果和已选择的矩阵构建，为分支保护规则提供
一个稳定的检查结果。

[Arduino 示例工作流](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/actions/workflows/arduino-examples.yml)
使用 Arduino CLI `1.5.1` 和 Arduino-ESP32 `3.3.11` 为 10 个 Arduino 示例的全部
三种显示型号编译（共 30 行）。它使用 `postv3` ESP32-P4 板级配置，仅编译，不发布
固件或可下载构建产物。`05_GFX_ESPWiFiAnalyzer` 在硬件上运行时需要 ESP32-C6 协处理器中
兼容的 Hosted 固件；请参阅 [Hosted Wi-Fi 兼容性说明](docs/p4-c6-hosted-wifi_ZH.md)。

出厂二进制文件不会进入源码构建发现范围。本地板级胶水、嵌入式上游代码和
托管组件候选之间的边界请参阅[组件归属说明](docs/components_ZH.md)。
11 号手机风格示例仅有编译覆盖；其 7 英寸硬件/UI 尚未验证，本仓库也未新增
720x1280 专用 stylesheet。

## 📦 出厂固件

默认 ESP-Brookesia 固件源码位于 [`firmware/brookesia/`](firmware/brookesia/)。
由该源码编译的 Rev3.x 合并出厂固件为：

- `ESP32-P4-WIFI6-Touch-LCD-7-FactoryOnly-260820.bin`
- `ESP32-P4-WIFI6-Touch-LCD-8-FactoryOnly-260820.bin`
- `ESP32-P4-WIFI6-Touch-LCD-10.1-FactoryOnly-260820.bin`

请仅使用与实际硬件及 ESP32-P4 Rev3.x 相符的固件。旧的固定名称 FactoryOnly
固件已移除，仅保留当前带日期的版本。这些文件与源码 CI 构建产物相互独立。
烧录工具和相关资料请参考官方[相关资料](https://docs.waveshare.net/ESP32-P4-WIFI6-Touch-LCD-X/Resources-And-Documents/)。

## 🗂️ 仓库结构

| 路径 | 用途 |
| --- | --- |
| [`examples/esp-idf/`](examples/esp-idf/) | 第一方 ESP-IDF 工程 |
| [`examples/arduino/`](examples/arduino/) | 第一方 Arduino 示例与 LCD-5 一致的随仓库库 |
| [`firmware/`](firmware/) | 出厂烧录固件与默认固件源码 |
| [`docs/`](docs/) | 兼容性与维护说明 |
| [`schematic/`](schematic/) | 开发板和转接板原理图 |
| [`assets/`](assets/) | 文档使用的产品图片 |
| [`.github/`](.github/) | CI 工作流和示例发现工具 |

## 🤝 支持与贡献

欢迎提交代码改进和可复现的问题报告。请提供产品型号、硬件版本（如已知）、
示例路径、ESP-IDF 版本、复现步骤、预期行为、实际行为和相关串口日志。

- [提交 GitHub Issue](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/issues)
- [贡献指南](CONTRIBUTING_ZH.md)
- [支持](SUPPORT_ZH.md)
- [微雪技术支持](https://service.waveshare.com/)
- [中文产品文档](https://docs.waveshare.net/ESP32-P4-WIFI6-Touch-LCD-X/)
- [English Documentation](https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-X)

## 📄 许可证

本仓库使用 Apache License 2.0，详情请参阅
[`LICENSE.txt`](LICENSE.txt)。
