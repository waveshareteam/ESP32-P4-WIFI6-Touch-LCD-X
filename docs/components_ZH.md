# 组件归属说明

[English](./components.md)

本页记录维护示例时采用的组件归属边界。目录名为 `components` 并不能单独证明
其中的内容可以删除或替换。

## 板级支持组件

六个显示示例已不再保留本地 `esp32_p4_wifi6_touch_lcd_x` 副本，统一使用
ESP Component Registry 中的托管组件
`waveshare/esp32_p4_wifi6_touch_lcd_x ^2.0.3`。2.0.3 包含 ESP-IDF v5.5.5
与 v6.0.2 构建线所需的兼容性修复。产品专用的 `bsp_extra` 组合代码和内嵌
上游源码树仍保留在本仓库。

## 示例本地组件

- `05_sdmmc/components/sd_card` 是 SDMMC 示例及其引脚诊断使用的支持代码。
- `08_lvgl_demo_v9/components/bsp_extra` 和
  `12_usb_extend_screen/components/bsp_extra` 组合了这些示例特有的板级显示、
  触摸、音频和 USB 行为。
- `10_mp4_player/components/esp_extractor` 是嵌入式上游媒体提取代码，应保留
  上游文档和归属信息。
- `11_esp_brookesia_phone/components/brookesia_core` 是嵌入式上游
  ESP-Brookesia 源码树，`brookesia_app_squareline_demo` 则是应用功能代码。
  嵌套的 `test_apps` 不是第一方产品示例，不进入默认 CI 矩阵。

## 固件本地组件

`firmware/brookesia` 是 Rev3.x 出厂固件源码工程，按设计同时保留产品应用、
`bsp_extra`、经过补丁调整的 `brookesia_core` 与本地 LCD-X BSP。该 BSP 依赖
已发布的 `espressif/esp_lcd_jd9365 ^2.0.2`，并显式写出 pre-v3/Rev3.x 的
MIPI PHY 时钟选择。托管 BSP 2.0.3 将 `phy_clk_src` 保持为零，由 ESP-IDF 根据
构建时选择的芯片修订版得到等价时钟。应将该本地副本继续作为版本化固件源码
边界，等后续独立固件迁移确认其余本地组合代码可以使用托管 BSP 后再移除。
构建目录、托管组件缓存及包含主机路径的依赖锁文件不属于该源码边界。

## 维护规则

1. 对可复用依赖，应在确认语义和硬件等价后优先使用托管组件。
2. 产品特定的包装层、组合代码和已验证的板级补丁继续保留在本仓库。
3. 保留嵌入式上游 README、许可证、命名和归属信息，不在这些源码树内添加
   产品本地翻译。
4. 删除本地实现前，任何组件迁移都必须在两个受支持 ESP-IDF 版本线上覆盖
   所有受影响的第一方示例。
5. `firmware/` 中每个带日期的出厂固件都是独立且不可变的交付文件，不是组件或
   示例构建输出；接受替代版本后应移除已过时版本。
