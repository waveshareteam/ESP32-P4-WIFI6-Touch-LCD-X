| 支持的目标芯片 | ESP32-P4 |
| -------------- | -------- |

# LVGL 9 演示

[English](./README.md)

本示例使用 LVGL 9 启动开发板显示与触摸栈，打开背光并运行
`lv_demo_benchmark()`。默认配置采用三缓冲局部刷新以减少撕裂，并启用基准
测试所需的 LVGL demo、性能监视和系统监视功能。

## 选择屏幕型号

使用 `idf.py menuconfig`，在
`Board Support Package (ESP32-P4) > Display > Select LCD type` 中选择与
开发板对应的 7、8 或 10.1 英寸屏。仓库默认配置选择 10.1 英寸屏、32 MB
Flash 和 200 MHz PSRAM。

源码中还保留了 LVGL music 和 widgets demo 的注释入口。启用其他 demo 可能
需要额外媒体或音频配置，应与基准测试的默认配置分开维护。

## 构建、烧录和监视

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

可通过屏幕基准画面和串口性能数据观察渲染行为。仅编译成功不能证明实际硬件
上的屏幕选择或触摸方向正确。
