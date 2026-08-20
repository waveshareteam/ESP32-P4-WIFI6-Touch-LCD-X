# ESP-IDF 示例

[English](./README.md)

本目录包含 ESP32-P4-WIFI6-Touch-LCD-X 产品系列的 12 个第一方 ESP-IDF
工程。每个工程都是本目录的直接子目录；组件源码树内嵌套的测试应用不属于
产品示例。

| 示例 | 功能 |
| --- | --- |
| [`01_HowToCreateProject`](./01_HowToCreateProject/) | 最小工程模板 |
| [`02_HelloWorld`](./02_HelloWorld/) | 芯片信息与重启循环 |
| [`03_i2c_tools`](./03_i2c_tools/) | 交互式 I2C 控制台 |
| [`04_wifistation`](./04_wifistation/) | ESP32-C6 Hosted Wi-Fi 站点 |
| [`05_sdmmc`](./05_sdmmc/) | 通过 SDMMC 访问 MicroSD |
| [`06_I2SCodec`](./06_I2SCodec/) | ES8311 与 I2S 音频 |
| [`07_Displaycolorbar`](./07_Displaycolorbar/) | MIPI-DSI 彩条测试 |
| [`08_lvgl_demo_v9`](./08_lvgl_demo_v9/) | LVGL 9 基准测试与触摸 |
| [`09_video_lcd_display`](./09_video_lcd_display/) | MIPI-CSI 摄像头预览 |
| [`10_mp4_player`](./10_mp4_player/) | 从 MicroSD 播放 MP4 与 AVI |
| [`11_esp_brookesia_phone`](./11_esp_brookesia_phone/) | ESP-Brookesia 手机界面 |
| [`12_usb_extend_screen`](./12_usb_extend_screen/) | USB 显示、HID 触摸与 UAC 音频 |

仓库工作流会使用当前 ESP-IDF v5.5 和 v6 版本线为 `esp32p4` 构建每个工程。
纯文档改动仍会运行发现与文档检查，但不会启动产品构建矩阵。

## 芯片修订版

所有示例默认使用 ESP32-P4 Rev3.x（`CONFIG_ESP32P4_REV_MIN_300=y`），与当前
产品硬件及 CI 产物一致。ESP32-P4 Rev1.3 属于 ESP-IDF 中互斥的 3.0 以前芯片
分组，必须显式追加 `config/sdkconfig/rev1_3.defaults` 覆盖层。两组芯片之间不能
复用二进制文件。配置项、MIPI-DSI 时钟、CPU 默认频率、检测和构建方法请参阅
[ESP32-P4 芯片修订版说明](../../docs/silicon-revisions_ZH.md)。

使用前请阅读所选工程的 README；显示类示例还需选择正确屏幕型号，并根据
仓库[原理图](../../schematic/)核对硬件相关配置。
