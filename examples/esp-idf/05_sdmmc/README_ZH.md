| 支持的目标芯片 | ESP32-P4 |
| -------------- | -------- |

# SDMMC 存储卡示例

[English](./README.md)

本示例通过 ESP32-P4 SDMMC 主机将 MicroSD 卡挂载到 `/sdcard`，打印存储卡
信息，完成测试文件的写入、读取和重命名，最后卸载文件系统。

## 开发板配置

默认四线总线使用开发板[原理图](../../../schematic/ESP32-P4-WIFI6-Touch-LCD-X-Schematic.pdf)
中的连接：

| 信号 | GPIO |
| --- | ---: |
| CMD | 44 |
| CLK | 43 |
| D0 | 39 |
| D1 | 40 |
| D2 | 41 |
| D3 | 42 |

ESP32-P4 配置使用内部 LDO 通道 4 为 SD I/O 供电，必须与开发板原理图保持
一致。SD 总线需要上拉电阻；示例启用的内部上拉仅用于辅助诊断。

## 数据安全

`挂载失败时格式化` 和 `在示例流程中格式化存储卡` 默认均关闭。启用任一
选项都可能清除卡内数据。即使选择单线模式，D3 仍需上拉，以免存储卡进入
SPI 模式。

## 构建、烧录和监视

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```
