| 支持的目标芯片 | ESP32-P4 |
| -------------- | -------- |

# 交互式 I2C 工具

[English](./README.md)

本示例启动一个交互式 `i2c-tools>` 控制台，用于检查和操作 I2C 总线上的器件。
控制台提供 `i2cconfig`、`i2cdetect`、`i2cget`、`i2cset`、`i2cdump` 以及
ESP-IDF 标准控制台命令。

## 开发板默认配置

- I2C 控制器：端口 0
- SCL：GPIO 8
- SDA：GPIO 7
- 命令历史：启用时挂载到 `/data` 的 FAT 文件系统

SCL 和 SDA 默认值与开发板原理图一致。操作外部器件前，请核对总线电压、
上拉电阻、器件地址和寄存器说明。`i2cset` 会写入器件寄存器，可能改变硬件
状态。

## 构建、烧录和监视

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

进入 `i2c-tools>` 提示符后可使用 `help` 查看命令和参数。引脚默认值与命令
历史设置位于 `menuconfig` 的 `Example Configuration` 中。
