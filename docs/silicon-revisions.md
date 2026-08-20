# ESP32-P4 Silicon Revisions

[中文](silicon-revisions_ZH.md)

ESP32-P4 revisions below 3.0 and revisions 3.0 or later are separate build
targets in ESP-IDF. A single binary does not support both groups. This
repository defaults every first-party ESP-IDF example and its CI matrix to the
`rev3_x` profile used by current ESP32-P4-WIFI6-Touch-LCD-X boards.

## Profile Differences

| Item | `rev1_3` | `rev3_x` (default) |
| --- | --- | --- |
| Intended silicon | ESP32-P4 v1.x, including v1.3 | ESP32-P4 v3.0 or later |
| Repository minimum | `CONFIG_ESP32P4_REV_MIN_100=y` | `CONFIG_ESP32P4_REV_MIN_300=y` |
| ESP-IDF revision group | `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y` | `CONFIG_ESP32P4_SELECTS_REV_LESS_V3` is disabled |
| Normal CPU default in ESP-IDF | 360 MHz | 400 MHz |
| Default MIPI-DSI PHY PLL reference | legacy PLL_F20M | XTAL |
| Repository CI artifacts | not published | published by the ESP-IDF example and maintained-firmware workflows |

ESP-IDF treats the two revision groups as mutually exclusive because their
hardware differs substantially. The bootloader checks the configured revision
range and refuses to boot on an incompatible chip. Revision selection is a
build-time setting; the application does not build one image and select a
profile after startup. ESP-IDF has no exact `REV_MIN_103` choice, so the
repository's `rev1_3` profile uses the 1.0 minimum for the pre-v3 group.

The silicon revision is not the LCD size and is not a PCB revision printed on
the board. Detect it without writing flash by running `esptool.py --chip
esp32p4 -p PORT chip_id` and checking the reported ESP32-P4 revision.

## Build the Default Rev3.x Profile

Each project under `examples/esp-idf` has a `sdkconfig.defaults` file that
selects revision 3.0 as its minimum. From an example directory, use a fresh
configuration:

```sh
idf.py set-target esp32p4
idf.py build
```

If the directory already has an `sdkconfig` generated for older silicon,
remove or rename that generated file before configuring again. `sdkconfig`
has priority over defaults and can otherwise preserve the old profile.

## Build Explicitly for Rev1.3

Rev1.3 hardware remains available as an explicit source-build profile. Use a
separate build directory and generated configuration so it cannot be confused
with the default Rev3.x image:

```sh
idf.py -B build-rev1_3 \
  -DSDKCONFIG=sdkconfig.rev1_3 \
  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;../../../config/sdkconfig/rev1_3.defaults" \
  build
```

Run this command from the selected `examples/esp-idf/<project>` directory. For
a display example, append the required LCD overlay before the revision overlay
when selecting a non-default display, for example:

```text
sdkconfig.defaults;../../../config/sdkconfig/lcd-7.defaults;../../../config/sdkconfig/rev1_3.defaults
```

Keep the revision overlay last so it overrides the Rev3.x project default.
Do not flash a `rev1_3` image to Rev3.x silicon or a `rev3_x` image to Rev1.3
silicon. The Windows CI flashing helper intentionally accepts only the
published Rev3.x artifacts. The Arduino examples likewise use the
Arduino-ESP32 `ChipVariant=postv3` board option. A successful build does not
replace hardware validation on the matching board revision.
