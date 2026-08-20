| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# MIPI-DSI Color Bar

[中文版本](./README_ZH.md)

This example initializes the board's MIPI-DSI display, enables the backlight,
and asks the LCD DPI driver to show a vertical color-bar test pattern. It is a
small display bring-up test and does not use LVGL.

## Select the Display Variant

Run `idf.py menuconfig`, then select the panel that matches the board under
`Board Support Package (ESP32-P4) > Display > Select LCD type`:

- 7-inch, 720 × 1280, ILI9881C
- 8-inch, 800 × 1280, JD9365
- 10.1-inch, 800 × 1280, JD9365 (default)

All variants use the board's two-lane MIPI-DSI interface. Do not use the
initialization table or resolution for a different panel size.

## Build, Flash, and Monitor

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

A stable full-screen vertical color-bar pattern confirms basic panel,
MIPI-DSI, reset, and backlight initialization. It does not validate touch.
