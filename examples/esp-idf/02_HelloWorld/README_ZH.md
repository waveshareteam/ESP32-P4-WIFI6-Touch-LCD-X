| 支持的目标芯片 | ESP32-P4 |
| -------------- | -------- |

# Hello World

[English](./README.md)

本示例会打印问候信息，并输出芯片能力、版本、Flash 容量与类型以及最小空闲堆
内存。随后程序从十秒开始倒计时，重启 ESP32-P4 并重复上述流程。

## 配置

仓库内默认配置选择 `esp32p4`、32 MB Flash、200 MHz 封装内 PSRAM 以及当前
开发板使用的 ESP32-P4 芯片版本范围。这些设置与本产品硬件一致；如果将示例
移植到其他开发板，应先重新核对。

## 构建、烧录和监视

```text
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

请将 `PORT` 替换为开发板对应的串口。监视器应先显示芯片和存储信息，然后显示
重启倒计时。
