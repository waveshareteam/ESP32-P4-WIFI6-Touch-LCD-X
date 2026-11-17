# Firmware

[English](README.md) | [简体中文](README_ZH.md)

Select the directory that matches your board model to download the factory firmware and view flashing instructions. Each model has its own image; use the firmware for your board's screen size.

| Model | Firmware and flashing instructions |
| --- | --- |
| ESP32-P4-WIFI6-Touch-LCD-7 | [Download and flash](ESP32-P4-WIFI6-Touch-LCD-7/README.md) |
| ESP32-P4-WIFI6-Touch-LCD-8 | [Download and flash](ESP32-P4-WIFI6-Touch-LCD-8/README.md) |
| ESP32-P4-WIFI6-Touch-LCD-10.1 | [Download and flash](ESP32-P4-WIFI6-Touch-LCD-10.1/README.md) |

## Firmware manifest

[`manifest.json`](manifest.json) declares the model IDs, firmware files, and bilingual instructions used by the firmware center. All paths are relative to the repository root. When adding a firmware version, add its full BIN path to the corresponding model in the manifest and update that model's README files. Keep older file paths in the manifest if those versions should remain available in the firmware center.
