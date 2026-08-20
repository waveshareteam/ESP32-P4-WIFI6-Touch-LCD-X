# ESP32-P4 芯片修订版

[English](silicon-revisions.md)

在 ESP-IDF 中，3.0 以前的 ESP32-P4 与 3.0 及以上修订版属于两个独立的构建
目标，同一个二进制文件不能同时支持两组芯片。本仓库所有第一方 ESP-IDF 示例
及其 CI 矩阵默认使用当前 ESP32-P4-WIFI6-Touch-LCD-X 开发板对应的 `rev3_x`
配置。

## 配置差异

| 项目 | `rev1_3` | `rev3_x`（默认） |
| --- | --- | --- |
| 适用芯片 | ESP32-P4 v1.x，包括 v1.3 | ESP32-P4 v3.0 及以上 |
| 仓库设置的最低版本 | `CONFIG_ESP32P4_REV_MIN_100=y` | `CONFIG_ESP32P4_REV_MIN_300=y` |
| ESP-IDF 修订版分组 | `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y` | 禁用 `CONFIG_ESP32P4_SELECTS_REV_LESS_V3` |
| ESP-IDF 常规 CPU 默认频率 | 360 MHz | 400 MHz |
| MIPI-DSI PHY PLL 默认参考时钟 | 旧版 PLL_F20M | XTAL |
| 本仓库 CI 产物 | 不发布 | 由 ESP-IDF 示例和维护固件工作流发布 |

由于两组芯片存在较大的硬件差异，ESP-IDF 将其设为互斥配置。引导程序会检查
构建时设置的修订版范围，并拒绝在不兼容的芯片上启动。修订版配置在构建时确定，
应用不会通过一个通用镜像在启动后自动选择配置。ESP-IDF 没有精确的
`REV_MIN_103` 选项，因此仓库的 `rev1_3` 配置使用 1.0 作为 pre-v3 分组的最低
版本。

芯片修订版不是 LCD 尺寸，也不是印刷在开发板上的 PCB 版本。可以运行
`esptool.py --chip esp32p4 -p PORT chip_id` 进行不写入 Flash 的检测，并查看其
输出的 ESP32-P4 revision。

## 构建默认 Rev3.x 配置

`examples/esp-idf` 下每个工程的 `sdkconfig.defaults` 都把 3.0 设为最低修订版。
请在示例目录中使用新的配置进行构建：

```sh
idf.py set-target esp32p4
idf.py build
```

如果目录中已有旧芯片配置生成的 `sdkconfig`，请先删除或改名，再重新配置。
生成的 `sdkconfig` 优先级高于 defaults，否则可能继续保留旧配置。

## 显式构建 Rev1.3 配置

Rev1.3 硬件仍可通过显式源码构建配置使用。应采用独立的构建目录和生成配置，
避免与默认 Rev3.x 镜像混淆：

```sh
idf.py -B build-rev1_3 \
  -DSDKCONFIG=sdkconfig.rev1_3 \
  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;../../../config/sdkconfig/rev1_3.defaults" \
  build
```

请在所选的 `examples/esp-idf/<project>` 目录中执行该命令。对于显示示例，如果
需要选择非默认屏幕，应将相应 LCD 覆盖层放在修订版覆盖层之前，例如：

```text
sdkconfig.defaults;../../../config/sdkconfig/lcd-7.defaults;../../../config/sdkconfig/rev1_3.defaults
```

修订版覆盖层必须放在最后，才能覆盖工程中的 Rev3.x 默认值。不要将 `rev1_3`
镜像烧录到 Rev3.x 芯片，也不要将 `rev3_x` 镜像烧录到 Rev1.3 芯片。Windows CI
烧录助手只接受已发布的 Rev3.x 产物。Arduino 示例同样使用 Arduino-ESP32 的
`ChipVariant=postv3` 板级选项。构建成功不能代替在相同芯片修订版开发板上的
硬件验证。
