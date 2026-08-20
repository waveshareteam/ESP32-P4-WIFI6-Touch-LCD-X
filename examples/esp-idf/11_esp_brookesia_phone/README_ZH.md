# ESP32-P4-WIFI6-Touch-LCD-X 运行 ESP-Brookesia Phone 示例

[English Version](./README.md)

本示例演示如何在 ESP32-P4-WIFI6-Touch-LCD-X 系列开发板上运行 ESP-Brookesia
Phone 界面。默认配置针对 7 英寸 720 x 1280 屏幕；8 英寸和 10.1 英寸
800 x 1280 屏幕可在 menuconfig 中选择。

## 入门指南

### 硬件要求

* 一块 ESP32-P4-WIFI6-Touch-LCD-7、-8 或 -10.1 开发板。

### ESP-IDF 要求

- 本示例已通过仓库 CI 在 ESP-IDF `v5.5.5` 和 `v6.0.2` 下验证，
  并支持更新的 ESP-IDF v5.5.x 和 v6.x 版本。
- 请按照
  [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html)
  设置开发环境。

### 配置

运行 `idf.py menuconfig`，在
`Board Support Package (ESP32-P4) > Display > Select LCD type` 中选择与
实际开发板匹配的屏幕型号，并按需调整 ESP-Brookesia 选项。

## 如何使用示例

### 构建和烧录示例

构建项目并将其烧录到开发板，然后运行监视工具查看串行输出（将 `PORT`
替换为您的开发板串口名称）：

```c
idf.py -p PORT flash monitor
```

要退出串行监视器，请输入 `Ctrl-]`。

完整的配置和使用 ESP-IDF 构建项目的步骤，请参见
[ESP-IDF 入门指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/get-started/index.html)。

## 技术支持和反馈

- 开发板相关问题请提交
  [GitHub issue](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/issues)
  或联系[微雪技术支持](https://service.waveshare.com/)。
- ESP-Brookesia 相关问题请访问
  [esp32.com](https://esp32.com/viewforum.php?f=35) 论坛或
  [esp-brookesia 仓库](https://github.com/espressif/esp-brookesia)。
