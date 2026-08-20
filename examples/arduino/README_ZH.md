# Arduino 示例

[English](README.md)

本目录提供 ESP32-P4-WIFI6-Touch-LCD-X 7 英寸、8 英寸和 10.1 英寸型号的
第一方 Arduino 示例。示例使用随仓库提供的 `lcd_x` 库以及标准
Arduino-ESP32 显示与 I2C API，不依赖尚未发布到组件仓库的组件。

## 示例

| 示例 | 用途 |
| --- | --- |
| [`01_DisplayColorBars`](examples/01_DisplayColorBars/) | 初始化所选 MIPI-DSI 面板并绘制色条。 |
| [`02_TouchDrawing`](examples/02_TouchDrawing/) | 轮询 GT911 兼容触摸输入并绘制触点。 |

每个示例均为 3 种显示型号构建，合计 6 个编译项。8 英寸和 10.1 英寸型号虽然
使用同一控制器系列，仍保留独立的面板初始化表。

## 构建

安装 Arduino CLI 与 Arduino-ESP32 `3.3.11` 后，使用 CI 的 ESP32-P4 Rev3.x
板级选项构建示例：

```sh
arduino-cli compile \
  --fqbn 'esp32:esp32:esp32p4:ChipVariant=postv3,PSRAM=enabled,FlashSize=32M,FlashMode=qio,FlashFreq=80,PartitionScheme=app13M_data7M_32MB,UploadMode=default,UploadSpeed=921600' \
  --libraries examples/arduino/libraries \
  --build-property 'compiler.cpp.extra_flags=-DLCD_X_DISPLAY_VARIANT=7' \
  examples/arduino/examples/01_DisplayColorBars
```

LCD-7、LCD-8 和 LCD-10.1 分别将 `LCD_X_DISPLAY_VARIANT` 设为 `7`、`8` 和
`101`。两个示例都应使用同一 FQBN 和随仓库提供的库路径。`postv3` 对应维护中的
Rev3.x 产品固件；不要将构建结果烧录到采用不同芯片修订版约定的开发板。

## 显示与触摸行为

显示屏复位引脚为 GPIO27，与触摸控制器独立。触摸库刻意不指定触摸 `INT` 和 `RST`
（均为 `GPIO_NUM_NC`），因此不会执行 GT911 地址选择时序。I2C 可用后，库会先探测
`0x5D`，再探测 `0x14`，并将设备句柄绑定到实际响应的地址。触摸仅读取轮询数据，
不会注册 GPIO 中断回调。示例使用 GT911 兼容的 GT9271 接口，并最多报告五个触点，
与当前第一方驱动支持范围一致。

## CI 与验证边界

[Arduino 示例工作流](../../.github/workflows/arduino-examples.yml) 使用 Arduino CLI
`1.5.1` 和 Arduino-ESP32 `3.3.11` 为两个示例的全部三种型号编译。该工作流不发布
固件或可下载构建产物。编译成功仅验证源码/API 集成，不验证开发板上的显示初始化、
I2C 连线、触摸坐标或其他硬件行为。
