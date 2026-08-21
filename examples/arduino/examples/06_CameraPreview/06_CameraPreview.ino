/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 *
 * OV5647 MIPI-CSI preview for ESP32-P4-WIFI6-Touch-LCD-X.
 *
 * ESP_Video is supplied by Arduino-ESP32 3.3.11. The OV5647 SCCB signals use
 * SCL=8 and SDA=7; LCD-7 reuses the display bridge bus on I2C port 1, while
 * LCD-8/LCD-10.1 create the camera bus on I2C port 0. The MIPI-CSI lanes are
 * wired on the board.
 * The ESP32-P4 ISP converts the sensor stream to RGB565 for lcd_x::Display.
 *
 * This sketch is compile-tested only. A connected OV5647 module and target
 * LCD-X hardware are required to validate camera capture and panel output.
 */

#ifndef BOARD_HAS_PSRAM
#error "This sketch requires PSRAM enabled in the Arduino Tools menu"
#endif

#include <ESP_Video.h>
#include <lcd_x_board.h>

namespace {

constexpr i2c_port_num_t kCameraSccbPort = I2C_NUM_0;
constexpr int8_t kCameraSccbScl = 8;
constexpr int8_t kCameraSccbSda = 7;
constexpr size_t kCaptureBufferCount = 2;

lcd_x::Display display;
ESPVideoClass video;
ESPVideoCaptureDevClass capture_device;
bool camera_ready = false;

void show_status(const char *message, uint16_t color = lcd_x::WHITE)
{
    display.fill_screen(lcd_x::BLACK);
    display.draw_text(12, 12, message, color, lcd_x::BLACK, 2);
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
    if (!csi_config.begin(camera_config)) {
        Serial.println("MIPI-CSI configuration failed");
        return false;
    }
    if (!video.begin(csi_config)) {
        Serial.println("OV5647 CSI initialization failed");
        return false;
    }
    if (!capture_device.begin(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, kCaptureBufferCount)) {
        Serial.println("MIPI-CSI capture device open failed");
        return false;
    }
    if (!capture_device.setFormat(ESP_VIDEO_FORMAT_RGB565)) {
        Serial.println("RGB565 capture format request failed");
        return false;
    }
    if (!capture_device.startCapture()) {
        Serial.println("MIPI-CSI capture start failed");
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

    // The capture buffer's pitch is source_width pixels. Pass that stride to
    // lcd_x so horizontal cropping never makes successive rows drift.
    const size_t source_pixels = buffer.size() / sizeof(uint16_t);
    if (source_pixels / source_width < source_height) {
        Serial.println("Camera buffer is smaller than its declared RGB565 frame");
        return;
    }

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
    show_status("OV5647 camera preview starting...");

    camera_ready = init_camera();
    if (!camera_ready) {
        show_status("Camera init failed. Connect OV5647.", lcd_x::RED);
    }
}

void loop()
{
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
