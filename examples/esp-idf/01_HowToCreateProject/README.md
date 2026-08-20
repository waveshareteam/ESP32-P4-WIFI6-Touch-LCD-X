| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# Minimal Project Template

[中文版本](./README_ZH.md)

This example is the smallest first-party ESP-IDF project in the repository. It
shows the required top-level CMake file, a `main` component, and an empty
`app_main()` entry point that can be used as the starting point for a new
application.

## Project Structure

- `CMakeLists.txt` loads the ESP-IDF project build system.
- `main/CMakeLists.txt` registers `main.c` as the application component.
- `main/main.c` provides an intentionally empty `app_main()` function.

The example does not initialize peripherals or print application output.

## Build, Flash, and Monitor

Run the following commands from this example directory:

```text
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with the serial port for the board. The monitor will show the
normal ESP-IDF boot log, but this empty application does not add its own log
messages.
