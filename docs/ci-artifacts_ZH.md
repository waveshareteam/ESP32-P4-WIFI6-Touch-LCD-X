# CI 构建产物

[English](ci-artifacts.md)

ESP-IDF 工作流共有 48 个源码构建矩阵项：6 个共享示例在两个 ESP-IDF 版本上
构建（12 项），以及 6 个显示示例分别在 3 个独立显示型号和两个 ESP-IDF 版本上
构建（36 项）。8 英寸和 10.1 英寸型号虽然分辨率和驱动相同，但面板初始化表不同，
因此仍保持为独立型号。

ESP-IDF v5.5.5 仅编译。完整运行中，每个 ESP-IDF v6.0.2 显示示例行都会上传一个
源码构建 ZIP，共 18 个可下载的 Actions artifact。可在 GitHub Actions 的工作流运行
页面下载指定 artifact，也可使用：

```sh
gh run download <run-id> --name '<artifact-name>'
```

该 CLI 命令会在本地展开 artifact 内容。助手则按 artifact ID 下载原始传输 ZIP，
以便在解压前将该 ZIP 与 Actions artifact 摘要比对。

在 Windows 上运行 `Flash-CI-Firmware.cmd -Product lcd-7`（也可选 `lcd-8` 或
`lcd-10-1`）。未提供 `-Product` 时会显示型号选择窗口。`-ListOnly` 不联网且不修改
状态，只列出三个型号和六个显示示例；`-SelfTest` 是完全离线、无需设备的契约测试。

助手只接受当前分支 HEAD、所选型号的 ESP-IDF v6.0.2 源码构建显示产物。下载前必须满足：
工作树干净、分支非 detached 且有 upstream、恰有一个指向该 HEAD 的非 draft 开放 PR，
以及一个包含所选型号全部六个唯一且未过期 SHA-256 artifact 的成功精确 SHA 工作流运行。
它会按 artifact ID 下载原始传输 ZIP，并在解压前将该 ZIP 与 GitHub Actions artifact 的
SHA-256 摘要严格比对，再验证 ZIP 路径、
校验和、manifest 身份、分段偏移、32 MB NOR 地址范围及安全的 `flash_args`。显式提供的
`-Port` 优先于同一产品和 SHA 的有效已保存端口；否则优先使用该已保存端口，再进行 USB
自动检测。merged 镜像会校验但绝不直接写入。传输只有在输出包含
`Hash of data verified` 时才算成功；随后仍需操作者人工确认 PASS，进度才会持久化并推进。

每次非破坏性的 `chip_id` 探测都必须同时返回 ESP32-P4 修订版和 MAC 地址。助手会将
MAC 规范化，并用它绑定已保存的进度；缺少 MAC，或修订版/MAC 身份发生变化时，都会在
烧录前 fail closed（拒绝继续）。

artifact 保留 14 天。这些文件是源码示例构建产物，不是出厂固件或发布镜像，不能替代
带版本标识且不可变的 `firmware/*-FactoryOnly-*.bin` 文件。本助手不执行硬件在环验证；操作者的 PASS
仅记录人工检查结果。
## 芯片修订版配置

示例产物仅使用 `rev3_x`，不会把 48 行示例矩阵翻倍。`rev1_3` 是面向 v1.3
等 1.x 芯片的显式源码构建配置，最低版本为 1.0，并选择 3.0 以前的硬件分组；
`rev3_x` 最低版本为 3.0，也是本仓库默认配置。两组配置互斥，二进制文件不能
交叉使用。配置项和硬件相关差异请参阅
[ESP32-P4 芯片修订版说明](silicon-revisions_ZH.md)。包中记录 schema version 2、
固定 32 MiB 闪存容量、配置与范围、数值分段偏移、文件大小、SHA-256 和独立的
合并镜像元数据。发布名称使用准确 checkout SHA12。编译成功不等于硬件验证。

`firmware/brookesia` 是维护中的产品固件源码。专用 ESP-IDF v5.5.5 工作流为 lcd-7、lcd-8 和 lcd-10.1 分别构建一个 Rev3.x 产物，共三个。由于该精确提交的 CI 不发布 Rev1.3 产物，Windows 助手会拒绝 Rev1.3 硬件。带日期的 FactoryOnly BIN 是独立且不可变的交付文件，不由 CI 重新生成。`11_esp_brookesia_phone` 仍由常规示例矩阵覆盖。

[Arduino 示例工作流](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/actions/workflows/arduino-examples.yml)
是独立的 30 行编译矩阵：10 个示例分别面向 LCD-7、LCD-8 和 LCD-10.1，使用 Arduino CLI
`1.5.1`、Arduino-ESP32 `3.3.11` 和 `postv3` Rev3.x 板级配置。它使用与 LCD-5 一致的
自包含 `GFX_Library_for_Arduino`、`displays`、LVGL `9.3.0` 和 `lv_conf.h` 库树；
`04_LVGLV9_Arduino` 使用 LVGL。该工作流不生成 Actions artifact、发布镜像或出厂固件。
编译成功不等于硬件测试；`05_GFX_ESPWiFiAnalyzer` 在开发板上运行还需要 ESP32-C6 中兼容的
Hosted 固件。示例清单、板级选项和硬件验证边界请参阅
[Arduino 示例说明](../examples/arduino/README_ZH.md)。CI 使用与 Arduino IDE 用户相同的
自包含 `examples/arduino/libraries` 目录，构建期间不下载 Arduino 库。
