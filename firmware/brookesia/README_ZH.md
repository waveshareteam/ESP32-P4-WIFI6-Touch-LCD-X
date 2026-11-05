# ESP-Brookesia 出厂固件

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5-E7352C?logo=espressif)](https://github.com/espressif/esp-idf/releases)
[![Target](https://img.shields.io/badge/target-ESP32--P4-1F6FEB)](https://www.espressif.com/en/products/socs/esp32-p4)
[![LVGL](https://img.shields.io/badge/LVGL-9.5.0-2AA9E0?logo=lvgl)](https://lvgl.io/)

[English](README.md) | 简体中文

本目录是 **微雪 ESP32-P4-WIFI6-Touch-LCD-7、-8 和 -10.1** 的 ESP-Brookesia 固件源码，
支持 MIPI-DSI 触摸屏、ES8311 / ES7210 音频编解码器、OV5647 摄像头、SD 卡和
ESP32-C6 Hosted Wi-Fi。

## 平台配置

| 项目 | 配置 |
| --- | --- |
| 主控 | ESP32-P4NRW32，默认 Rev3.x，通过配置文件支持 Rev1.x，32 MB Flash，32 MB PSRAM |
| 显示 / 触摸 | 微雪 7、8、10.1 英寸 MIPI-DSI 竖屏，UI 为横屏，GT911 触摸 |
| 框架 | ESP-Brookesia 0.6.0-beta2（本地 `brookesia_core` 组件） |
| UI | LVGL 9.5.0 |
| 板级支持 | ESP Board Manager 0.7.5，使用微雪板级定义中的 `esp32_p4_wifi6_touch_lcd_x` |
| LVGL 适配 | `espressif/esp_lvgl_adapter` 0.7.4 |
| 无线 | ESP32-C6 经 SDIO 连接，使用 ESP-Hosted 1.4.7 / `esp_wifi_remote` 0.14.5 |
| 语音助手 | `espressif/esp_xiaozhi` 0.1.3、ESP-SR 2.4.7、xiaozhi-fonts 1.6.0 |
| 多媒体 | `espressif/esp_extractor` 1.0.7、`esp_new_jpeg` 1.0.2、`esp_video` 2.5.0 |

接口与引脚分配见[仓库 README](../../README_ZH.md)。

## 板级支持

硬件使用 [ESP Board Manager](https://components.espressif.com/components/espressif/esp_board_manager)
描述，不再使用手写 BSP。`esp32_p4_wifi6_touch_lcd_x` 的基础配置是 10.1 英寸屏，
其他屏幕通过 amend 选择：

| 屏幕 | 分辨率 | 控制器 | MIPI-DSI 单 lane 速率 | Amend |
| --- | --- | --- | ---: | --- |
| 7 英寸 | 720 x 1280 | ILI9881C | 1000 Mbps | `lcd_7_dsi_touch_a` |
| 8 英寸 | 800 x 1280 | JD9365 | 1500 Mbps | `lcd_8_dsi_touch_a` |
| 10.1 英寸 | 800 x 1280 | JD9365 | 1500 Mbps | 无（基础板级） |

所有屏幕均为 2 条数据 lane、80 MHz DPI 时钟、3 个 RGB565 帧缓冲并启用 DMA2D。
`board` 组件是应用使用的板级层，基于 Board Manager 初始化显示/LVGL、触摸、背光、
音频、存储和 Wi-Fi。`gen_bmgr_codes` 由 `idf.py bmgr` 生成，请勿手动修改。

GT911 的 INT 与 RST 未连接到 SoC。触摸设备依次探测 I2C 地址 `0x5D` 和 `0x14`，
使用实际探测到的地址创建 panel IO，并通过轮询读取触摸数据。由于 SoC 无法复位
触摸芯片，板级层会在屏幕之前初始化 I2C 总线，并最多重试 3 次触摸探测。除非
另行变更硬件约束，否则不要加入依赖 INT/RST 的地址选择时序。

## 应用

<table>
  <tr>
    <td align="center"><img src="components/draw/assets/img_app_drawpanel.png" width="64" alt="DrawPanel 图标"><br><b>DrawPanel</b></td>
    <td align="center"><img src="components/brookesia_app_squareline_demo/assets/esp_brookesia_app_icon_launcher_squareline_112_112.png" width="64" alt="Squareline 图标"><br><b>Squareline</b></td>
    <td align="center"><img src="components/brookesia_app_calculator/assets/img_app_calculator.png" width="64" alt="Calculator 图标"><br><b>Calculator</b></td>
    <td align="center"><img src="components/SpecAnalyzer/assets/img_app_specanalyzer.png" width="64" alt="SpecAnalyzer 图标"><br><b>SpecAnalyzer</b></td>
    <td align="center"><img src="components/MusicPlayer/assets/img_app_musicplayer.png" width="64" alt="MusicPlayer 图标"><br><b>MusicPlayer</b></td>
  </tr>
  <tr>
    <td align="center"><img src="components/Camera/assets/img_app_camera.png" width="64" alt="Camera 图标"><br><b>Camera</b></td>
    <td align="center"><img src="components/VideoPlayer/assets/img_app_vedioplayer.png" width="64" alt="VideoPlayer 图标"><br><b>VideoPlayer</b></td>
    <td align="center"><img src="components/Settings/assets/img_app_settings.png" width="64" alt="Settings 图标"><br><b>Settings</b></td>
    <td align="center"><img src="components/XiaozhiApp/assets/img_app_xiaozhi.png" width="64" alt="AIChats 图标"><br><b>AIChats</b></td>
    <td></td>
  </tr>
</table>

| 应用 | 功能 | 运行依赖 |
| --- | --- | --- |
| DrawPanel | 固定画布区域绘图，在边缘绘画不会拖动整个应用 | 显示与触摸 |
| Squareline | 从组件仓库安装的 Squareline Studio UI 演示与应用模板 | 显示与触摸 |
| Calculator | 适合触摸操作的 Brookesia 计算器 | 显示与触摸 |
| SpecAnalyzer | 板载麦克风的 FFT 频谱 | ES7210 输入 |
| MusicPlayer | 播放 `/spiffs/music` 中的 MP3，带频谱可视化 | SPIFFS `storage`，ES8311 输出 |
| Camera | OV5647 MIPI CSI 预览，由 PPA 缩放铺满屏幕 | OV5647，`esp_video` |
| VideoPlayer | 全屏播放 SD 卡中的 AVI/MJPEG 视频，支持 PCM 音频 | SD 卡，JPEG/PPA，ES8311 输出 |
| Settings | WLAN 扫描/连接、声音、屏幕亮度、SNTP 时间（UTC+8）和设备信息；亮度与音量保存在 NVS | ESP32-C6 Hosted Wi-Fi |
| AIChats | 网络检查、小智激活、WakeNet 唤醒词、AEC/VAD、Opus 上下行、服务端文本、TTS 和 MCP 设备工具 | Wi-Fi，`model`/`storage` 镜像，ES7210 与 ES8311 |

## 编译

### 环境要求

使用 ESP-IDF v5.5（锁定 v5.5.6）和 `esp32p4` 工具链，需要能访问 ESP 组件仓库。
烧录时请使用支持数据传输的 USB 线。

以下命令都在本目录下运行。从仓库根目录开始，先为所用屏幕生成板级代码，再编译。
`-a` 直接写 amend 名称，会在板级目录内查找：

```sh
cd firmware/brookesia
idf.py --version

# 7 英寸
idf.py bmgr -b esp32_p4_wifi6_touch_lcd_x -a lcd_7_dsi_touch_a
# 8 英寸
idf.py bmgr -b esp32_p4_wifi6_touch_lcd_x -a lcd_8_dsi_touch_a
# 10.1 英寸
idf.py bmgr -b esp32_p4_wifi6_touch_lcd_x

# 按屏幕选择配置：lcd-7、lcd-8 或 lcd-10-1
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.lcd-7" build
```

`gen_bmgr_codes` 一次只保存一种屏幕的代码。Board Manager 会自动把生成的
`board_manager.defaults` 加入 sdkconfig 默认配置，并在切换板级时重置 `sdkconfig`。
如果 CMake 报 `SRC_DIRS` 路径不存在，或 `gen_bmgr_codes` 出现链接错误，说明生成代码
已过期：重新执行对应屏幕的 `idf.py bmgr` 命令（必要时先执行 `idf.py bmgr -x` 清理）。
切换板级请使用 `idf.py bmgr`，不要通过 `menuconfig`。

### 屏幕配置

每种屏幕有一个配置文件，用来选择对应的手机样式表。它必须和 `idf.py bmgr` 选的屏幕
一致，构建时不会检查两者是否匹配：

| 配置文件 | 屏幕 | UI 尺寸 | 样式表选项 |
| --- | --- | --- | --- |
| `sdkconfig.defaults.lcd-7` | 7 英寸 | 1280 x 720 | `CONFIG_PHONE_STYLESHEET_7_INCH` |
| `sdkconfig.defaults.lcd-8` | 8 英寸 | 1280 x 800 | `CONFIG_PHONE_STYLESHEET_8_INCH` |
| `sdkconfig.defaults.lcd-10-1` | 10.1 英寸 | 1280 x 800 | `CONFIG_PHONE_STYLESHEET_10_INCH` |

屏幕原生是竖屏，`components/board/src/board_display.c` 里的 `BOARD_DISPLAY_ROTATION`
把 UI 旋转 270° 成横屏，所以样式表按旋转后的尺寸选择。样式表大于旋转后的 UI 时，
`addStylesheet()` 会失败，启动随之中止。1280 x 720 样式表位于
`main/Stylesheet1280x720Dark.hpp`。

### 芯片版本

`sdkconfig.defaults` 面向 ESP32-P4 Rev3.x。Rev1.x 芯片需要在屏幕配置之后再加上
`sdkconfig.defaults.rev1_3`：

```sh
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.lcd-7;sdkconfig.defaults.rev1_3" build
```

Rev1.x 配置把 PSRAM 从 250 MHz 降到 200 MHz，并关闭 ISP pipeline controller，因此
相机不运行 IPA 的自动曝光和自动白平衡。两组芯片版本的固件不能混用，`esptool` 会拒绝
烧录另一组的固件。烧录前可以先确认芯片版本：
`esptool.py --chip esp32p4 -p PORT chip_id`。

### 切换屏幕或芯片版本

所有配置共用 `build/` 和 `sdkconfig`，`idf.py flash` 烧录的是最后一次构建的结果。
`sdkconfig` 优先于默认配置，已有 `sdkconfig` 时配置文件不会生效。切换屏幕或芯片版本
前先清理：

```sh
idf.py fullclean
```

然后删除 `sdkconfig`，再重新运行构建命令。

### 首次完整烧录与分区变更

首次安装必须写入 bootloader、分区表、应用、ESP-SR 模型和 SPIFFS 资源：

```sh
idf.py -p PORT flash monitor
```

修改分区偏移或大小、从旧分区布局升级、或排查模型/文件系统镜像残留时，
先擦除 Flash 再完整烧录：

```sh
idf.py -p PORT erase-flash flash monitor
```

分区布局、语音模型和 `spiffs/` 资源都没有变化时，可以只烧录应用：

```sh
idf.py -p PORT app-flash monitor
```

`app-flash` 不会更新 `srmodels.bin`、`storage.bin` 和分区表。修改
`partitions.csv`、WakeNet/模型 Kconfig、xiaozhi-fonts、内置音乐或提示音后，
需要执行完整 `flash`。

### Flash 分区

分区布局定义在 `partitions.csv`，分区表位于 `0x9000`：

| 名称 | 类型 | 偏移 | 大小 | 用途 |
| --- | --- | --- | --- | --- |
| `nvsfactory` | data / nvs | `0xA000` | 200 KiB | 出厂 NVS 数据 |
| `nvs` | data / nvs | `0x3C000` | 840 KiB | Wi-Fi、应用设置和小智状态 |
| `otadata` | data / ota | `0x10E000` | 8 KiB | OTA 数据（预留） |
| `phy_init` | data / phy | `0x110000` | 4 KiB | PHY 校准数据 |
| `model` | data / spiffs | `0x111000` | 956 KiB | ESP-SR 语音模型（`srmodels.bin`） |
| `factory` | app / factory | `0x200000` | 8 MiB | 应用程序 |
| `storage` | data / spiffs | `0xA00000` | 6 MiB | 音乐、小智提示音和字体（`storage.bin`） |

`parttool.py` 默认从 `0x8000` 读取分区表，所以要指定偏移，例如只擦除 `nvs`：

```sh
python $IDF_PATH/components/partition_table/parttool.py --port PORT --partition-table-offset 0x9000 erase_partition --partition-name=nvs
```

### 出厂固件

应用二进制命名为 `esp32-p4-lcd-x-brookesia.bin`。已发布的合并固件为
`ESP32-P4-WIFI6-Touch-LCD-7-FactoryOnly-260915.bin`、
`ESP32-P4-WIFI6-Touch-LCD-8-FactoryOnly-260820.bin` 和
`ESP32-P4-WIFI6-Touch-LCD-10.1-FactoryOnly-260820.bin`。为某个屏幕编译完成后，
可合并出从偏移 `0x0` 烧录的镜像，例如：

```sh
(cd build && python -m esptool --chip esp32p4 merge_bin -o ../../ESP32-P4-WIFI6-Touch-LCD-7-FactoryOnly-260915.bin @flash_args)
```

## AIChats / 小智

AIChats 使用 `esp_xiaozhi` 处理协议，本工程负责 UI、激活、音频和生命周期集成。

首次使用：

1. 在 Settings 中配置 WLAN。
2. 打开 AIChats。如果设备尚未绑定账号，会显示并播报激活码。
3. 在[小智控制台](https://xiaozhi.me)添加设备并输入激活码。
4. 页面进入待机后说出唤醒词，默认是“你好小智”。

如果日志出现 `No speech model found` 或 `contains no compatible models`，
请重新编译并完整烧录，以写入分区表和 `srmodels/srmodels.bin`。

本地 UI 语言和首选传输方式可在以下菜单中选择：

```text
idf.py menuconfig
  Xiaozhi Application
    Local UI language
      English
      Simplified Chinese
    Preferred Xiaozhi transport
      WebSocket
      Auto (prefer MQTT+UDP)
      MQTT+UDP
```

`sdkconfig.defaults` 选择的是 Auto。Auto 在准备聊天会话时决定传输方式：OTA 返回
`mqtt` 配置就用 MQTT+UDP，否则用 WebSocket。运行中 MQTT 或 UDP 出错时不会回退，
因此在屏蔽 UDP 的网络里请选择 WebSocket。

语言选项只影响固件自带的文本，不会翻译服务端返回的对话文本、TTS 或激活消息。
本地字符串位于 `components/XiaozhiApp/XiaozhiLocalization.*`。

ES7210 采集两路麦克风，以及作为回声参考的 ES8311 输出。AIChats 以 `MMR` 格式
送入 ESP-SR AFE 做 AEC、唤醒词和 VAD，再转换为 16 kHz 单声道 Opus 上行；播放链路
会按需转换服务端音频以适配 ES8311 输出。

### MCP 工具

AIChats 在小智 MCP 引擎上注册了设备工具，可以用语音查询和控制开发板，
例如“把音量调到 50”或“你看到了什么”：

| 工具 | 参数 | 功能 |
| --- | --- | --- |
| `device.audio.set_volume` | `volume`（0-95） | 设置扬声器音量并保存 |
| `device.audio.set_mute` | `mute`（bool） | 静音或取消静音 |
| `device.display.set_brightness` | `brightness`（20-100） | 设置屏幕亮度并保存 |
| `device.display.set_screen` | `on`（bool） | 关闭背光，或按已保存的亮度重新点亮 |
| `device.get_status` | 无 | 音量、亮度和剩余内存 |
| `device.network.get_wifi_status` | 无 | Wi-Fi 是否开启、链路状态、SSID 和 IPv4 地址 |
| `device.get_info` | 无 | 芯片、固件和 ESP-IDF 版本，屏幕尺寸、麦克风和 Wi-Fi 协处理器 |
| `device.system.reboot` | 无 | 几秒后重启设备，仅在用户明确要求时调用 |
| `self.camera.explain_image` | `question`（string） | 用 OV5647 拍照，并通过服务端视觉服务回答关于照片的问题 |

音量和亮度与 Settings 使用相同的范围和 NVS 存储，语音修改的结果重启后仍然保留。
工具在 `components/XiaozhiApp/XiaozhiDeveloperTools.cpp` 的
`registerXiaozhiDeveloperTools()` 中注册，每个 MCP 引擎在小智传输启动前注册一次。

## 摄像头与多媒体

Camera 使用 OV5647 的 800 x 800 RAW8 模式。PPA 将画面铺满屏幕：方向与屏幕不一致时
旋转 90°，并按 PPA 1/16 步进的缩放系数裁剪源画面。

VideoPlayer 只扫描 `/sdcard/avi`，`.avi` 扩展名不区分大小写。SD 卡根目录下的
AVI 以及 MP4、MOV 等其他容器不会被列出。播放器使用 `esp_extractor` 解封装 AVI，
用硬件 JPEG 解码器解码 MJPEG（硬件无法解码的帧，例如渐进式 JPEG，回退到
`esp_new_jpeg`），再经 PPA 直接写入 LCD 帧缓冲：

- 视频只支持 MJPEG，音频只支持 PCM；其他音频格式会被跳过，视频静音播放。
- 画面不旋转，会以中心为基准裁剪铺满屏幕，因此请为这些竖屏编码竖向视频。

其他视频可用 ffmpeg 转换，例如：

```sh
ffmpeg -i input.mov -c:v mjpeg -q:v 5 -pix_fmt yuvj420p -r 24 -c:a pcm_s16le -ar 44100 -ac 2 output.avi
```

如果播放丢帧，可以调大 `-q:v`（降低画质）或缩小视频分辨率。

MusicPlayer 播放 `/spiffs/music` 中的 MP3。更换曲目后需要重新生成并烧录
`storage.bin`，仅 `app-flash` 不会更新它。

## 开发说明

- 应用通过 `ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR` 自行注册。
  `main/main.cpp` 中的 `BOOT_APP_GROUPS` 只决定主屏顺序以及各组安装时显示的启动阶段；
  已注册但未列出的应用会在最后一组之后安装。
- 新应用继承 `esp_brookesia::systems::phone::App` 并实现 `run()` 和 `back()`，
  其他生命周期函数按资源归属按需重写。每个组件在自己的 `idf_component.yml`
  中声明依赖，`main` 只保留直接使用的依赖。
- 板级硬件通过 `board` 组件（`board.h` 及各 `board_*.h` 头文件）访问，
  应用不直接调用 Board Manager。
- 新增 MCP 工具时在 `registerXiaozhiDeveloperTools()` 中注册（见 [MCP 工具](#mcp-工具)）。
  只注册已实现且返回真实结果的工具，耗时操作放在应用任务中执行。
- 在 LVGL 线程中访问 LVGL 对象，或短暂持有显示锁。应用关闭时释放其定时器、
  任务、队列、编解码器会话、SD 句柄和 LVGL 对象。
- 播放期间 VideoPlayer 会启用适配层的 Dummy Draw 并暂停 LVGL，退出时停止播放并恢复
  LVGL 渲染。

## 注意事项

- LVGL 锁只用于 UI 操作。网络、SD 扫描、音频初始化、NVS 访问和模型加载都应在锁外执行。
- 修改模型、字体、提示音、音乐或分区后需要完整 `flash`。
- 音频应用共用同一条物理总线，切换应用时要停止或交接编解码器会话。
- Host 组件需要与 ESP32-C6 上的协处理器固件保持兼容。

`main/main.cpp` 中的 `EXAMPLE_SHOW_MEM_INFO` 当前为 `true`，会周期性统计 SRAM/PSRAM；
详细日志输出由 `EXAMPLE_SHOW_MEM_INFO_LOG` 单独控制。

反馈问题时，请提供屏幕尺寸、ESP-IDF 版本、完整复现步骤和日志中第一个实际错误。

本地 Brookesia core 使用 [Apache-2.0 许可证](components/brookesia_core/license.txt)，
仅适用于该组件。
