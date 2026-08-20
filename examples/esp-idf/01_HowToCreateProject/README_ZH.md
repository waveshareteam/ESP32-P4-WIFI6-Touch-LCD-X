| 支持的目标芯片 | ESP32-P4 |
| -------------- | -------- |

# 最小工程模板

[English](./README.md)

这是仓库中最精简的第一方 ESP-IDF 工程，展示了必需的顶层 CMake 文件、
`main` 组件和一个空的 `app_main()` 入口，可作为新应用的起点。

## 工程结构

- `CMakeLists.txt` 加载 ESP-IDF 工程构建系统。
- `main/CMakeLists.txt` 将 `main.c` 注册为应用组件。
- `main/main.c` 提供一个有意留空的 `app_main()` 函数。

本示例不会初始化外设，也不会输出应用日志。

## 构建、烧录和监视

在本示例目录中运行：

```text
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

请将 `PORT` 替换为开发板对应的串口。监视器会显示 ESP-IDF 的正常启动日志，
但这个空应用不会额外打印消息。
