# Arduino Examples

[中文](README_ZH.md)

This directory contains the first-party Arduino examples for the
ESP32-P4-WIFI6-Touch-LCD-X 7-inch, 8-inch, and 10.1-inch variants. They use
the bundled `lcd_x` library and standard Arduino-ESP32 display and I2C APIs;
no unpublished component-registry dependency is required.

## Examples

| Sketch | Purpose |
| --- | --- |
| [`01_DisplayColorBars`](examples/01_DisplayColorBars/) | Initialize the selected MIPI-DSI panel and draw color bars. |
| [`02_TouchDrawing`](examples/02_TouchDrawing/) | Poll GT911-compatible touch input and draw touch points. |

Each sketch is built for all three display variants, for six compile rows in
total. The 8-inch and 10.1-inch variants keep separate panel initialization
tables even though they share the same controller family.

## Build

Install Arduino CLI and Arduino-ESP32 core `3.3.11`, then compile a sketch
with the ESP32-P4 Rev3.x board options used by CI:

```sh
arduino-cli compile \
  --fqbn 'esp32:esp32:esp32p4:ChipVariant=postv3,PSRAM=enabled,FlashSize=32M,FlashMode=qio,FlashFreq=80,PartitionScheme=app13M_data7M_32MB,UploadMode=default,UploadSpeed=921600' \
  --libraries examples/arduino/libraries \
  --build-property 'compiler.cpp.extra_flags=-DLCD_X_DISPLAY_VARIANT=7' \
  examples/arduino/examples/01_DisplayColorBars
```

Set `LCD_X_DISPLAY_VARIANT` to `7`, `8`, or `101` for the LCD-7, LCD-8, or
LCD-10.1 respectively. Use the same FQBN and bundled-library path for either
sketch. The `postv3` selection is for the maintained Rev3.x product firmware;
do not flash a build to a board with a different silicon-revision contract.

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
uses Arduino CLI `1.5.1` and Arduino-ESP32 `3.3.11` to compile both sketches
for all three variants. It produces no release or downloadable firmware
artifact. A successful compile validates the source/API integration only; it
does not verify display initialization, I2C wiring, touch coordinates, or
other hardware behavior on a board.
