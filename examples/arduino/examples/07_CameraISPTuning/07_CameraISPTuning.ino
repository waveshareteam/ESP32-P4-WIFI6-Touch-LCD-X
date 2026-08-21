/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 *
 * OV5647 MIPI-CSI preview and ISP/3A tuning for ESP32-P4-WIFI6-Touch-LCD-X.
 *
 * ESP_Video is supplied by Arduino-ESP32 3.3.11. Enter one command per line
 * in the serial monitor:
 *   g <0..1023>   analog gain
 *   e <0..10000>  exposure time in microseconds
 *   a <0..255>    automatic-exposure target level
 *   v <0|1>       vertical flip
 *   h <0|1>       horizontal flip
 *   t <0|1>       sensor test pattern
 *   s             print last accepted values
 *
 * OV5647 SCCB uses SCL=8 and SDA=7. LCD-7 reuses the display bridge bus on
 * I2C port 1; LCD-8/LCD-10.1 create the camera bus on I2C port 0. MIPI-CSI is
 * wired on-board.
 * This sketch is compile-tested only. Validate settings and camera output on
 * target LCD-X hardware with a connected OV5647 module.
 */

#ifndef BOARD_HAS_PSRAM
#error "This sketch requires PSRAM enabled in the Arduino Tools menu"
#endif

#include <ESP_Video.h>
#include <lcd_x_board.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

namespace {

constexpr i2c_port_num_t kCameraSccbPort = I2C_NUM_0;
constexpr int8_t kCameraSccbScl = 8;
constexpr int8_t kCameraSccbSda = 7;
constexpr size_t kCaptureBufferCount = 2;
constexpr size_t kCommandCapacity = 24;

struct IspSettings {
    int32_t gain = 0;
    int32_t exposure_us = 0;
    int32_t ae_target = 0;
    bool vertical_flip = false;
    bool horizontal_flip = false;
    bool test_pattern = false;
};

lcd_x::Display display;
ESPVideoClass video;
ESPVideoCaptureDevClass capture_device;
IspSettings settings;
bool camera_ready = false;
char command[kCommandCapacity] = {};
size_t command_length = 0;
bool command_overflow = false;

void show_status(const char *message, uint16_t color = lcd_x::WHITE)
{
    display.fill_screen(lcd_x::BLACK);
    display.draw_text(12, 12, message, color, lcd_x::BLACK, 2);
}

void print_settings()
{
    Serial.printf("gain=%ld exposure_us=%ld ae_target=%ld vflip=%u hflip=%u test_pattern=%u\n",
                  static_cast<long>(settings.gain), static_cast<long>(settings.exposure_us),
                  static_cast<long>(settings.ae_target), settings.vertical_flip,
                  settings.horizontal_flip, settings.test_pattern);
}

void draw_hud()
{
    char hud[112];
    snprintf(hud, sizeof(hud), "g:%ld e:%ld a:%ld v:%u h:%u t:%u",
             static_cast<long>(settings.gain), static_cast<long>(settings.exposure_us),
             static_cast<long>(settings.ae_target), settings.vertical_flip,
             settings.horizontal_flip, settings.test_pattern);
    display.draw_text(6, 6, hud, lcd_x::BLACK, lcd_x::WHITE, 1);
}

bool init_camera()
{
    ESPVideoCamConfigClass camera_config;
    i2c_master_bus_handle_t shared_bus = nullptr;
    // LCD-7 keeps its bridge on SDA=7/SCL=8 through I2C_NUM_1. Reuse that
    // bus when it exists so ESP_Video does not attempt to claim those pins a
    // second time. LCD-8 and LCD-10.1 create the SCCB bus on I2C_NUM_0.
    const bool camera_bus_ready =
        i2c_master_get_bus_handle(I2C_NUM_1, &shared_bus) == ESP_OK && shared_bus != nullptr
            ? camera_config.begin(shared_bus)
            : camera_config.begin(kCameraSccbPort, kCameraSccbScl, kCameraSccbSda);
    if (!camera_bus_ready) {
        Serial.println("OV5647 SCCB configuration failed");
        return false;
    }

    ESPVideoCSIConfigClass csi_config;
    if (!csi_config.begin(camera_config) || !video.begin(csi_config) ||
        !capture_device.begin(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, kCaptureBufferCount) ||
        !capture_device.setFormat(ESP_VIDEO_FORMAT_RGB565) || !capture_device.startCapture()) {
        Serial.println("OV5647 MIPI-CSI pipeline initialization failed");
        return false;
    }
    return true;
}

void draw_frame(const ESPVideoBufferClass &buffer)
{
    const uint32_t source_width = buffer.getWidth();
    const uint32_t source_height = buffer.getHeight();
    if (buffer.formatType() != ESP_VIDEO_FORMAT_RGB565 || source_width == 0 ||
        source_height == 0 || source_width > INT16_MAX || source_height > INT16_MAX) {
        return;
    }

    const size_t source_pixels = buffer.size() / sizeof(uint16_t);
    if (source_pixels / source_width < source_height) {
        Serial.println("Camera buffer is smaller than its declared RGB565 frame");
        return;
    }

    // Keep the full capture pitch while cropping from the centered source
    // rectangle; using draw_width as stride would shear every copied row.
    const int panel_width = display.width();
    const int panel_height = display.height();
    const int frame_width = static_cast<int>(source_width);
    const int frame_height = static_cast<int>(source_height);
    const int draw_width = min(frame_width, panel_width);
    const int draw_height = min(frame_height, panel_height);
    const int source_x = (frame_width - draw_width) / 2;
    const int source_y = (frame_height - draw_height) / 2;
    const int destination_x = (panel_width - draw_width) / 2;
    const int destination_y = (panel_height - draw_height) / 2;
    const uint16_t *pixels = reinterpret_cast<const uint16_t *>(buffer.data());

    display.draw_rgb565_bitmap(destination_x, destination_y, draw_width, draw_height,
                               pixels + static_cast<size_t>(source_y) * source_width + source_x,
                               source_width);
    draw_hud();
}

bool parse_value(const char *text, long *value)
{
    if (text == nullptr || value == nullptr || *text == '\0') {
        return false;
    }
    errno = 0;
    char *end = nullptr;
    const long parsed = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0') {
        return false;
    }
    *value = parsed;
    return true;
}

void handle_command()
{
    command[command_length] = '\0';
    char *text = command;
    while (*text == ' ' || *text == '\t') {
        ++text;
    }
    if (*text == '\0') {
        return;
    }

    const char operation = *text++;
    while (*text == ' ' || *text == '\t') {
        ++text;
    }
    if (operation == 's' && *text == '\0') {
        print_settings();
        return;
    }

    long value = 0;
    if (!parse_value(text, &value)) {
        Serial.println("Invalid command. Use g/e/a/v/h/t <value> or s");
        return;
    }

    bool accepted = false;
    switch (operation) {
    case 'g':
        if (value >= 0 && value <= 1023) {
            accepted = capture_device.setSensorGain(value);
            if (accepted) settings.gain = value;
        }
        break;
    case 'e':
        if (value >= 0 && value <= 10000) {
            accepted = capture_device.setSensorExposureTime(value);
            if (accepted) settings.exposure_us = value;
        }
        break;
    case 'a':
        if (value >= 0 && value <= 255) {
            accepted = capture_device.setSensorAETargetLevel(value);
            if (accepted) settings.ae_target = value;
        }
        break;
    case 'v':
        if (value == 0 || value == 1) {
            accepted = capture_device.setSensorVFlip(value != 0);
            if (accepted) settings.vertical_flip = value != 0;
        }
        break;
    case 'h':
        if (value == 0 || value == 1) {
            accepted = capture_device.setSensorHFlip(value != 0);
            if (accepted) settings.horizontal_flip = value != 0;
        }
        break;
    case 't':
        if (value == 0 || value == 1) {
            accepted = capture_device.setSensorTestPattern(value != 0);
            if (accepted) settings.test_pattern = value != 0;
        }
        break;
    default:
        Serial.println("Unknown command. Use g/e/a/v/h/t <value> or s");
        return;
    }

    if (!accepted) {
        Serial.println("Value is outside the allowed range or the sensor rejected it");
        return;
    }
    print_settings();
}

void handle_serial()
{
    while (Serial.available()) {
        const char character = static_cast<char>(Serial.read());
        if (character == '\n' || character == '\r') {
            if (command_overflow) {
                Serial.println("Command too long");
            } else if (command_length > 0) {
                handle_command();
            }
            command_length = 0;
            command_overflow = false;
        } else if (!command_overflow) {
            if (command_length + 1 < sizeof(command)) {
                command[command_length++] = character;
            } else {
                command_overflow = true;
            }
        }
    }
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);

    if (!display.begin()) {
        Serial.println("LCD-X display initialization failed");
        return;
    }
    show_status("OV5647 ISP tuning starting...");

    camera_ready = init_camera();
    if (!camera_ready) {
        show_status("Camera init failed. Connect OV5647.", lcd_x::RED);
        return;
    }
    Serial.println("ISP tuning ready: g/e/a/v/h/t <value>, or s");
    print_settings();
}

void loop()
{
    handle_serial();
    if (!camera_ready || !capture_device.isOpened() || !capture_device.isCaptureStarted()) {
        delay(500);
        return;
    }

    ESPVideoBufferClass buffer = capture_device.captureBuffer();
    if (!buffer.valid()) {
        delay(5);
        return;
    }
    draw_frame(buffer);
    // buffer is returned to ESP_Video when it leaves scope.
}
