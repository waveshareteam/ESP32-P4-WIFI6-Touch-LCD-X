# Component Ownership

[中文版本](./components_ZH.md)

This page records the ownership boundary used when maintaining the examples.
A directory named `components` is not, by itself, evidence that its contents
can be removed or replaced.

## Board Support Component

The six display examples no longer keep example-local
`esp32_p4_wifi6_touch_lcd_x` copies. They consistently use the managed
`waveshare/esp32_p4_wifi6_touch_lcd_x ^2.0.3` component from the ESP Component
Registry. Version 2.0.3 contains the compatibility fixes required by the
ESP-IDF v5.5.5 and v6.0.2 build lines. Product-specific `bsp_extra` composition
code and embedded upstream trees remain local.

## Example-Local Components

- `05_sdmmc/components/sd_card` is support code for the SDMMC example and its
  pin diagnostics.
- `08_lvgl_demo_v9/components/bsp_extra` and
  `12_usb_extend_screen/components/bsp_extra` compose board-local display,
  touch, audio, and USB behavior that is specific to those examples.
- `10_mp4_player/components/esp_extractor` is embedded upstream media
  extraction code. Preserve its upstream documentation and attribution.
- `11_esp_brookesia_phone/components/brookesia_core` is an embedded upstream
  ESP-Brookesia tree, while `brookesia_app_squareline_demo` is application
  feature code. Nested `test_apps` are not first-party product examples and
  remain outside the default CI matrix.

## Firmware-Local Components

`firmware/brookesia` is the Rev3.x factory-source project and intentionally
keeps its product applications, `bsp_extra`, patched `brookesia_core`, and a
local LCD-X BSP together. The BSP requires the published
`espressif/esp_lcd_jd9365 ^2.0.2` dependency and spells out the pre-v3/Rev3.x
MIPI PHY clock choice explicitly. Managed BSP 2.0.3 leaves `phy_clk_src` at
zero, which makes ESP-IDF select the equivalent clock from the configured chip
revision. Keep the firmware copy as a versioned source boundary until a
separate firmware migration verifies that the remaining local composition can
use the managed BSP. Generated build directories, managed components, and the
host-specific dependency lock are not part of this source boundary.

## Maintenance Rules

1. Prefer managed components for reusable dependencies after semantic and
   hardware equivalence is established.
2. Keep product-specific wrappers, composition code, and verified board-local
   patches local.
3. Preserve embedded-upstream README files, licenses, naming, and attribution;
   do not add product-local translations inside those trees.
4. Test any component migration across every affected first-party example on
   both supported ESP-IDF release lines before removing a local implementation.
5. Treat each dated factory binary under `firmware/` as an immutable delivery
   rather than component or example build output; remove superseded versions
   when a replacement is accepted.
