# ESP32-P4-WIFI6-Touch-LCD-X ESP-Brookesia 固件

[English](README.md)

这是 ESP32-P4-WIFI6-Touch-LCD-7、-8 与 -10.1 的默认固件源码。保留 Brookesia 手机桌面及配套应用：ESP-Hosted Wi-Fi、摄像头、音频、音乐、视频、绘图、频谱分析、设置和小智。

## 要求

- ESP-IDF v5.5.5
- 仅支持 ESP32-P4 Rev3.x

## 屏幕变体

| 变体 | 屏幕 | MIPI-DSI 单 lane 速率 | 分辨率 | 控制器 | DPI 时钟 |
| --- | --- | ---: | --- | --- | ---: |
| `lcd-7` | 7 英寸 | 1000 Mbps | 720 x 1280 | ILI9881C | 80 MHz |
| `lcd-8` | 8 英寸 | 1500 Mbps | 800 x 1280 | JD9365 | 80 MHz |
| `lcd-10-1` | 10.1 英寸 | 1500 Mbps | 800 x 1280 | JD9365 | 80 MHz |

本地 BSP 对 MIPI PHY 时钟源使用显式分支：pre-v3 使用 `MIPI_DSI_PHY_CLK_SRC_DEFAULT`，Rev3.x 使用 `MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT`。

## 触摸初始化

触摸控制器不指定 INT 或 RST GPIO。BSP 依次探测 I2C 地址 `0x5D` 和 `0x14`，
使用实际探测到的地址创建 panel IO，并通过轮询读取触摸数据。除非另行变更
硬件约束，否则不要加入依赖 INT/RST 的地址选择时序。

## 编译

在本目录导出 ESP-IDF v5.5.5 环境后，按屏幕选择一条命令：

```bash
idf.py -B build-lcd-7-v5.5.5-rev3_x -D SDKCONFIG="$PWD/build-lcd-7-v5.5.5-rev3_x/sdkconfig" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.rev3_x;sdkconfig.defaults.lcd-7" build
idf.py -B build-lcd-8-v5.5.5-rev3_x -D SDKCONFIG="$PWD/build-lcd-8-v5.5.5-rev3_x/sdkconfig" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.rev3_x;sdkconfig.defaults.lcd-8" build
idf.py -B build-lcd-10-1-v5.5.5-rev3_x -D SDKCONFIG="$PWD/build-lcd-10-1-v5.5.5-rev3_x/sdkconfig" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.rev3_x;sdkconfig.defaults.lcd-10-1" build
```

应用二进制命名为 `esp32-p4-lcd-x-brookesia.bin`。仓库中的 Rev3.x 合并固件沿用 LCD-5 的出厂命名方式：`ESP32-P4-WIFI6-Touch-LCD-7-FactoryOnly-260820.bin`、`ESP32-P4-WIFI6-Touch-LCD-8-FactoryOnly-260820.bin` 和 `ESP32-P4-WIFI6-Touch-LCD-10.1-FactoryOnly-260820.bin`。

## 合并出厂固件

编译成功后运行对应命令。每条命令会生成一份 16 MiB 合并镜像，可从偏移 `0x0` 开始烧录：

```bash
(cd build-lcd-7-v5.5.5-rev3_x && python -m esptool --chip esp32p4 merge_bin -o ../../ESP32-P4-WIFI6-Touch-LCD-7-FactoryOnly-260820.bin @flash_args)
(cd build-lcd-8-v5.5.5-rev3_x && python -m esptool --chip esp32p4 merge_bin -o ../../ESP32-P4-WIFI6-Touch-LCD-8-FactoryOnly-260820.bin @flash_args)
(cd build-lcd-10-1-v5.5.5-rev3_x && python -m esptool --chip esp32p4 merge_bin -o ../../ESP32-P4-WIFI6-Touch-LCD-10.1-FactoryOnly-260820.bin @flash_args)
```
