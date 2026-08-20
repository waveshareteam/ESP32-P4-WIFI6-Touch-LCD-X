/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#define LCD_X_DISPLAY_7 7
#define LCD_X_DISPLAY_8 8
#define LCD_X_DISPLAY_10_1 101

#ifndef LCD_X_DISPLAY_VARIANT
#define LCD_X_DISPLAY_VARIANT LCD_X_DISPLAY_10_1
#endif

#if LCD_X_DISPLAY_VARIANT != LCD_X_DISPLAY_7 && \
    LCD_X_DISPLAY_VARIANT != LCD_X_DISPLAY_8 && \
    LCD_X_DISPLAY_VARIANT != LCD_X_DISPLAY_10_1
#error "LCD_X_DISPLAY_VARIANT must be LCD_X_DISPLAY_7, LCD_X_DISPLAY_8, or LCD_X_DISPLAY_10_1"
#endif

namespace lcd_x {

enum class DisplayVariant : uint16_t {
    LCD_7 = LCD_X_DISPLAY_7,
    LCD_8 = LCD_X_DISPLAY_8,
    LCD_10_1 = LCD_X_DISPLAY_10_1,
};

constexpr DisplayVariant configured_variant()
{
    return static_cast<DisplayVariant>(LCD_X_DISPLAY_VARIANT);
}

constexpr uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return static_cast<uint16_t>(((red & 0xF8U) << 8U) |
                                 ((green & 0xFCU) << 3U) |
                                 (blue >> 3U));
}

constexpr uint16_t BLACK = rgb565(0, 0, 0);
constexpr uint16_t WHITE = rgb565(255, 255, 255);
constexpr uint16_t RED = rgb565(255, 0, 0);
constexpr uint16_t GREEN = rgb565(0, 255, 0);
constexpr uint16_t BLUE = rgb565(0, 0, 255);
constexpr uint16_t CYAN = rgb565(0, 255, 255);
constexpr uint16_t MAGENTA = rgb565(255, 0, 255);
constexpr uint16_t YELLOW = rgb565(255, 255, 0);

struct TouchPoint {
    uint16_t x;
    uint16_t y;
    uint16_t strength;
    uint8_t track_id;
};

class Display {
public:
    bool begin(DisplayVariant variant = configured_variant());
    void fill_screen(uint16_t color);
    void fill_rect(int x, int y, int width, int height, uint16_t color);
    void fill_circle(int x, int y, int radius, uint16_t color);

    uint16_t width() const { return width_; }
    uint16_t height() const { return height_; }
    DisplayVariant variant() const { return variant_; }

private:
    void sync_rect(int x, int y, int width, int height);

    DisplayVariant variant_ = configured_variant();
    uint16_t width_ = 0;
    uint16_t height_ = 0;
    uint16_t *frame_buffer_ = nullptr;
    esp_lcd_dsi_bus_handle_t dsi_bus_ = nullptr;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
};

class Touch {
public:
    static constexpr size_t MAX_POINTS = 5;
    static constexpr gpio_num_t RESET_GPIO = GPIO_NUM_NC;
    static constexpr gpio_num_t INTERRUPT_GPIO = GPIO_NUM_NC;

    bool begin(uint16_t width, uint16_t height);
    bool read(TouchPoint *points, size_t capacity, size_t *count);

    uint8_t address() const { return address_; }
    const char *product_id() const { return product_id_; }

private:
    bool read_register(uint16_t reg, uint8_t *data, size_t size);
    bool write_register(uint16_t reg, uint8_t value);

    i2c_master_dev_handle_t device_ = nullptr;
    uint16_t width_ = 0;
    uint16_t height_ = 0;
    uint8_t address_ = 0;
    char product_id_[5] = {};
};

}  // namespace lcd_x
