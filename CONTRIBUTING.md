# Contributing to ESP32-P4-WIFI6-Touch-LCD-X

[简体中文](CONTRIBUTING_ZH.md)

Thanks for your interest in contributing. This repository contains
first-party ESP-IDF examples, factory firmware images, and board
documentation for the Waveshare ESP32-P4-WIFI6-Touch-LCD-X series.

## Issues

When reporting an issue, include:

- Product variant and hardware revision if known
- Example path and ESP-IDF version
- Reproduction steps, expected behavior, and actual behavior
- Build/flash logs and serial output
- Whether the failure appears with a source-built example or factory firmware

## Pull Requests

- Keep changes scoped and explain the motivation in the description.
- Update the affected example documentation (English and Simplified Chinese)
  when behavior or configuration changes.
- Validate every touched ESP-IDF example against ESP-IDF `v5.5.5` and
  `v6.0.2` for `esp32p4` before merging.
- Do not add, remove, or repackage factory firmware binaries or delivery
  archives unless the change is an explicit release update.
- Keep repository-public text free of local paths, credentials, and
  environment-specific details.

## CI

The repository CI builds changed examples on both supported ESP-IDF lines and
validates first-party Markdown links. A green CI run is required before
merge.
