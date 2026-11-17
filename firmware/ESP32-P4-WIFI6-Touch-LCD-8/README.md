# ESP32-P4-WIFI6-Touch-LCD-8 Firmware

[English](README.md) | [简体中文](README_ZH.md)

Factory firmware for ESP32-P4-WIFI6-Touch-LCD-8. The BIN is a merged image flashed at `0x0`; the date in its file name identifies the version.

| File | Purpose |
| --- | --- |
| [ESP32-P4-WIFI6-Touch-LCD-8_FactoryOnly_20261105.bin](ESP32-P4-WIFI6-Touch-LCD-8_FactoryOnly_20261105.bin) | Factory firmware, restores the out-of-box demo |

Run these commands in this directory, replacing `<PORT>` with the board's serial port. Erasing flash removes existing settings and user data.

```bash
python -m pip install esptool
esptool.py --chip esp32p4 --port <PORT> erase_flash
esptool.py --chip esp32p4 --port <PORT> write_flash 0x0 ESP32-P4-WIFI6-Touch-LCD-8_FactoryOnly_20261105.bin
```

Factory firmware source: [`examples/factory/brookesia`](../../examples/factory/brookesia/).
