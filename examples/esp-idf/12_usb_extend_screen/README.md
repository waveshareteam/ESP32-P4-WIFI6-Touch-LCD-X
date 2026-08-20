| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# USB Extended Display

[中文版本](./README_ZH.md)

This example exposes the ESP32-P4 USB 2.0 High-Speed device interface as a
vendor display transport, an HID touch device, and a USB Audio Class device.
Received RGB565, RGB888, YUV420, or JPEG frames are decoded as needed and
drawn on the selected MIPI-DSI panel.

## Default Functions

- High-Speed TinyUSB device mode
- Vendor frame transport with a 300,000-byte frame limit
- HID touch reporting enabled
- USB Audio Class speaker and microphone paths enabled
- RGB565 LCD output, two display buffers, and a 60 FPS limit

HID touch and UAC audio can be disabled under `Example Configuration`.
TinyUSB class descriptors provide the final compile-time class selection.

## Configure the Board

Select the 7, 8, or 10.1-inch display that matches the board under
`Board Support Package (ESP32-P4) > Display > Select LCD type`. The checked-in
P4 defaults select the 10.1-inch panel. A received frame must match the active
panel width and height and start at coordinate `(0, 0)`.

Use the board's USB 2.0 OTG High-Speed Type-C port for the device connection;
the USB-to-UART port is for flashing and serial monitoring.

## Build, Flash, and Monitor

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

The host-side frame sender is a separate integration surface. This repository
example validates the device implementation but does not package a desktop
host application or replace the checked-in factory firmware.
