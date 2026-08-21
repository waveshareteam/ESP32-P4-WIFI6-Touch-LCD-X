# Arduino Examples

[中文](README_ZH.md)

This directory contains 10 first-party Arduino examples for the
ESP32-P4-WIFI6-Touch-LCD-X 7-inch, 8-inch, and 10.1-inch variants. The
display-facing sketches use the bundled `lcd_x` library. The shared
[`libraries/`](libraries/) directory also bundles LVGL `9.5.0` and its
`lv_conf.h` configuration. All sketches use Arduino-ESP32 APIs and require no
unpublished component-registry dependency.

## Examples

| Sketch | Purpose |
| --- | --- |
| [`01_DisplayColorBars`](examples/01_DisplayColorBars/) | Initialize the selected MIPI-DSI panel and draw color bars. |
| [`02_TouchDrawing`](examples/02_TouchDrawing/) | Poll GT9271 touch input and draw touch points. |
| [`03_AsciiTable`](examples/03_AsciiTable/) | Render a printable ASCII character table using the display text API. |
| [`04_LVGLV9`](examples/04_LVGLV9/) | Run an LVGL 9.5.0 display and polling-touch demonstration. |
| [`05_WiFiAnalyzer`](examples/05_WiFiAnalyzer/) | Scan and display nearby Wi-Fi networks through the ESP32-C6 hosted connection. |
| [`06_CameraPreview`](examples/06_CameraPreview/) | Show the onboard OV5647 MIPI-CSI camera preview on the selected LCD. |
| [`07_CameraISPTuning`](examples/07_CameraISPTuning/) | Preview the camera and expose supported ISP tuning controls over Serial. |
| [`08_SDCard`](examples/08_SDCard/) | Mount the MicroSD card and exercise basic file access. |
| [`09_AudioPlayback`](examples/09_AudioPlayback/) | Play a synthesized melody through the ES8311 codec and speaker path, starting at volume 60. |
| [`10_MicRecord`](examples/10_MicRecord/) | Capture the ES7210 stereo MIC1/MIC2 stream and report separate channel statistics over Serial. |

Each sketch is built for all three display variants, for 30 compile rows in
total. The 8-inch and 10.1-inch variants keep separate panel initialization
tables even though they share the same controller family. Only `04_LVGLV9`
uses the bundled LVGL `9.5.0` configuration.

## Build

Install Arduino CLI and Arduino-ESP32 core `3.3.11`. Before compiling with the
Arduino IDE, copy the **contents** of this repository's
[`libraries/`](libraries/) directory into the sketchbook's `libraries`
directory. The resulting layout must be:

```text
<sketchbook>/libraries/lcd_x/
<sketchbook>/libraries/lvgl/
<sketchbook>/libraries/lv_conf.h
```

Do not create an extra `libraries/libraries/` level. This provides `lcd_x`,
LVGL `9.5.0`, and its configuration without a separate Library Manager
install.

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
  examples/arduino/examples/01_DisplayColorBars
```

Set `LCD_X_DISPLAY_VARIANT` to `7`, `8`, or `101` for the LCD-7, LCD-8, or
LCD-10.1 respectively. CI uses the same FQBN and bundled-library path for
every sketch; only the display-facing sketches use `lcd_x`. The `postv3`
selection is the maintained Rev3.x default; do not flash a build to a board
with a different silicon-revision contract. The same command, with the target
sketch path and variant value changed, also builds `04_LVGLV9`.

## Board peripherals used by the sketches

| Peripheral | Board connection | Sketches |
| --- | --- | --- |
| Display | Two-lane MIPI-DSI; reset GPIO27 | `01`–`07` |
| Touch | I2C SDA GPIO7 / SCL GPIO8; `INT` and `RST` are not assigned | `02`, `04` |
| Camera | OV5647 MIPI-CSI; SCCB SDA GPIO7 / SCL GPIO8 | `06`, `07` |
| Hosted Wi-Fi | ESP32-C6 coprocessor over SDIO | `05` |
| MicroSD | SDMMC 4-bit: CLK GPIO43, CMD GPIO44, D0 GPIO39, D1 GPIO40, D2 GPIO41, D3 GPIO42 | `08` |
| Audio control | I2C SDA GPIO7 / SCL GPIO8 | `09`, `10` |
| Audio data | I2S MCLK GPIO13, BCLK GPIO12, LRCK GPIO10, DOUT GPIO9, DIN GPIO11; amplifier enable GPIO53 | `09`, `10` |

The I2C devices share the board bus. Camera, wireless, storage, and audio
examples require their corresponding onboard peripheral; do not repurpose the
listed pins while running those sketches. `05_WiFiAnalyzer` requires
factory-compatible hosted firmware on the ESP32-C6 coprocessor; see
[ESP32-C6 hosted Wi-Fi compatibility](../../docs/p4-c6-hosted-wifi.md).
Start `09_AudioPlayback` at low speaker volume before adjusting it. The
microphone example selects the two front microphones, MIC1 and MIC2, at 24 dB
and reports both channels from the codec's stereo SDOUT1 stream. It does not
enable the MIC3 echo-reference path or the unconnected MIC4 path.

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
