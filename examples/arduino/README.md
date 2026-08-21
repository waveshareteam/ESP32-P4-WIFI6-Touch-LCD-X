# Arduino Examples

[中文](README_ZH.md)

This directory contains 10 first-party Arduino examples for the
ESP32-P4-WIFI6-Touch-LCD-X 7-inch, 8-inch, and 10.1-inch variants. The
display sketches use the LCD-5-style bundled libraries:
`GFX_Library_for_Arduino`, `displays`, LVGL `9.3.0`, and `lv_conf.h`.
All sketches use Arduino-ESP32 APIs and require no unpublished
component-registry dependency.

## Examples

| Sketch | Purpose |
| --- | --- |
| [`01_HelloWorld`](examples/01_HelloWorld/) | Minimal Arduino_GFX MIPI-DSI display bring-up. |
| [`02_AsciiTable`](examples/02_AsciiTable/) | Arduino_GFX capability and benchmark table. |
| [`03_Drawing_board`](examples/03_Drawing_board/) | GT9271 five-point capacitive-touch drawing. |
| [`04_LVGLV9_Arduino`](examples/04_LVGLV9_Arduino/) | LVGL 9 widgets UI with touch input. |
| [`05_GFX_ESPWiFiAnalyzer`](examples/05_GFX_ESPWiFiAnalyzer/) | Graphical Wi-Fi scan through the on-board ESP32-C6 coprocessor. |
| [`06_Camera_Preview`](examples/06_Camera_Preview/) | OV5647 MIPI-CSI camera preview on the display. |
| [`07_Camera_ISP_Tuning`](examples/07_Camera_ISP_Tuning/) | Live preview with interactive ISP/3A tuning over Serial. |
| [`08_SD_Card`](examples/08_SD_Card/) | MicroSD read/write over the SDIO 3.0 slot. |
| [`09_Audio_Playback`](examples/09_Audio_Playback/) | ES8311 codec plays the opening of “Für Elise” as different-frequency tones. |
| [`10_Mic_Record`](examples/10_Mic_Record/) | ES7210 microphone capture; prints peak, RMS, and decimated samples over Serial. |

Each sketch is built for all three display variants, for 30 compile rows in
total. The 8-inch and 10.1-inch variants keep separate panel initialization
tables even though they share the same controller family. Only
`04_LVGLV9_Arduino` uses the bundled LVGL `9.3.0` configuration.

## Build

Install Arduino CLI and Arduino-ESP32 core `3.3.11`. Before compiling with the
Arduino IDE, copy `GFX_Library_for_Arduino/`, `displays/`, `lvgl/`, and
`lv_conf.h` from this repository's [`libraries/`](libraries/) directory into
the sketchbook's `libraries/` directory. The resulting layout must be:

```text
<sketchbook>/libraries/GFX_Library_for_Arduino/
<sketchbook>/libraries/displays/
<sketchbook>/libraries/lvgl/
<sketchbook>/libraries/lv_conf.h
```

Do not create an extra `libraries/libraries/` level.

The bundled configuration defaults to LCD-10.1 (`101`). In Arduino IDE, select
LCD-7, LCD-8, or LCD-10.1 by editing `LCD_X_DISPLAY_VARIANT` in
`<sketchbook>/libraries/displays/displays_config.h` to `7`, `8`, or `101`
before compiling. Define it there, rather than only inside one sketch, so the
same value reaches every display and touch library source file.

The Arduino-ESP32 core provides the standard `WiFi`, `ESP_Video`, `SD_MMC`,
I2C, and I2S APIs used by the sketches. The ES8311 and ES7210 driver sources
used by the audio sketches are included with those sketches. To compile from
this checkout with Arduino CLI, use the ESP32-P4 Rev3.x board options used by
CI:

```sh
arduino-cli compile \
  --fqbn 'esp32:esp32:esp32p4:ChipVariant=postv3,PSRAM=enabled,FlashSize=32M,FlashMode=qio,FlashFreq=80,PartitionScheme=app13M_data7M_32MB,UploadMode=default,UploadSpeed=921600' \
  --libraries examples/arduino/libraries \
  --build-property 'compiler.cpp.extra_flags=-DLCD_X_DISPLAY_VARIANT=7' \
  examples/arduino/examples/01_HelloWorld
```

Set `LCD_X_DISPLAY_VARIANT` to `7`, `8`, or `101` for the LCD-7, LCD-8, or
LCD-10.1 respectively. CI uses the same FQBN and bundled-library path for
every sketch. The `postv3` selection is the maintained Rev3.x default; do not
flash a build to a board with a different silicon-revision contract. The same
command, with the target sketch path and variant value changed, also builds
`04_LVGLV9_Arduino`.

## Board peripherals used by the sketches

| Peripheral | Board connection | Sketches |
| --- | --- | --- |
| Display | Two-lane MIPI-DSI; reset GPIO27 | `01`–`07` |
| Touch | I2C SDA GPIO7 / SCL GPIO8; `INT` and `RST` are `GPIO_NUM_NC` | `03`, `04` |
| Camera | OV5647 MIPI-CSI; SCCB SDA GPIO7 / SCL GPIO8 | `06`, `07` |
| Hosted Wi-Fi | ESP32-C6 coprocessor over SDIO | `05` |
| MicroSD | SDMMC 4-bit: CLK GPIO43, CMD GPIO44, D0 GPIO39, D1 GPIO40, D2 GPIO41, D3 GPIO42 | `08` |
| Audio control | I2C SDA GPIO7 / SCL GPIO8 | `09`, `10` |
| Audio data | I2S MCLK GPIO13, BCLK GPIO12, LRCK GPIO10, DOUT GPIO9, DIN GPIO11; amplifier enable GPIO53 | `09`, `10` |

The I2C devices share the board bus. Camera, wireless, storage, and audio
examples require their corresponding onboard peripheral; do not repurpose the
listed pins while running those sketches. `05_GFX_ESPWiFiAnalyzer` requires
factory-compatible hosted firmware on the ESP32-C6 coprocessor; see
[ESP32-C6 hosted Wi-Fi compatibility](../../docs/p4-c6-hosted-wifi.md).
The audio sketches retain the LCD-5 codec and I2S example settings. Verify the
speaker level and microphone/channel mapping on hardware before relying on
audio output or captured data.

## Display and touch behavior

The display reset line is GPIO27. It is independent of the touch controller.
The touch library deliberately leaves both touch `INT` and `RST` unspecified
(`GPIO_NUM_NC`), so it does not perform GT911 address-selection timing.
After I2C is available it probes address `0x5D`, then `0x14`, and binds the
device handle to the address that responds. Touch reads are polling-only; no
GPIO interrupt callback is registered. The examples use the GT911-compatible
GT9271 interface and report at most five points, matching the supported
first-party driver behavior.

## CI and validation boundary

The [Arduino examples workflow](../../.github/workflows/arduino-examples.yml)
uses Arduino CLI `1.5.1` and Arduino-ESP32 `3.3.11` to compile all 10 sketches
for all three variants with the `postv3` Rev3.x board profile (30 rows). It
produces no release or downloadable firmware artifact. A successful compile
validates source/API integration only; it does not verify display
initialization, I2C wiring, touch coordinates, camera streaming, hosted Wi-Fi,
MicroSD access, audio behavior, or other hardware behavior on a board.
