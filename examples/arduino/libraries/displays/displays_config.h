// displays_config.h — Waveshare ESP32-P4-WIFI6-Touch-LCD-X
#pragma once
#ifndef DISPLAYS_CONFIG_H
#define DISPLAYS_CONFIG_H

#include <Arduino_GFX_Library.h>
#include "i2c.h"

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

struct DisplayConfig
{
    const char *name;

    uint32_t hsync_pulse_width;
    uint32_t hsync_back_porch;
    uint32_t hsync_front_porch;
    uint32_t vsync_pulse_width;
    uint32_t vsync_back_porch;
    uint32_t vsync_front_porch;
    uint32_t prefer_speed;
    uint32_t lane_bit_rate;

    uint16_t width;
    uint16_t height;
    int8_t rotation;
    bool auto_flush;
    int8_t rst_pin;

    const lcd_init_cmd_t *init_cmds;
    size_t init_cmds_size;

    int8_t i2c_sda_pin;
    int8_t i2c_scl_pin;
    uint32_t i2c_clock_speed;
    int8_t lcd_rst;
};

#define LCD_X_BACKLIGHT_PIN ((int8_t)26)
#define LCD_X_BACKLIGHT_FREQ 5000
#define LCD_X_BACKLIGHT_RES 10

inline void display_cfg_backlight(bool on) {
  ledcAttach(LCD_X_BACKLIGHT_PIN, LCD_X_BACKLIGHT_FREQ, LCD_X_BACKLIGHT_RES);
  ledcWrite(LCD_X_BACKLIGHT_PIN, on ? ((1 << LCD_X_BACKLIGHT_RES) - 1) : 0);
}

/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 *
 * Panel command tables mirrored from the maintained LCD-X BSP in this repository.
 */

static const lcd_init_cmd_t kJd9365Lcd8Commands[] = {
    // JD9365 driver preamble: select page 0, RGB565 and two DSI lanes.
    {0xE0, (uint8_t[]){0x00}, 1, 0},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},
    {0x80, (uint8_t[]){0x01}, 1, 0},
    {0xE0, (uint8_t[]){0x00}, 1, 0},
    {0xE1, (uint8_t[]){0x93}, 1, 0},
    {0xE2, (uint8_t[]){0x65}, 1, 0},
    {0xE3, (uint8_t[]){0xF8}, 1, 0},
    {0x80, (uint8_t[]){0x01}, 1, 0}, // 0X03：4-LANE;0X02：3-LANE;0X01:2-LANE

    {0xE0, (uint8_t[]){0x01}, 1, 0},
    {0x00, (uint8_t[]){0x00}, 1, 0},
    {0x01, (uint8_t[]){0x4E}, 1, 0},
    {0x03, (uint8_t[]){0x00}, 1, 0},
    {0x04, (uint8_t[]){0x65}, 1, 0},

    {0x0C, (uint8_t[]){0x74}, 1, 0},

    {0x17, (uint8_t[]){0x00}, 1, 0},
    {0x18, (uint8_t[]){0xB7}, 1, 0},
    {0x19, (uint8_t[]){0x00}, 1, 0},
    {0x1A, (uint8_t[]){0x00}, 1, 0},
    {0x1B, (uint8_t[]){0xB7}, 1, 0},
    {0x1C, (uint8_t[]){0x00}, 1, 0},

    {0x24, (uint8_t[]){0xFE}, 1, 0},

    {0x37, (uint8_t[]){0x19}, 1, 0},

    {0x38, (uint8_t[]){0x05}, 1, 0},
    {0x39, (uint8_t[]){0x00}, 1, 0},
    {0x3A, (uint8_t[]){0x01}, 1, 0},
    {0x3B, (uint8_t[]){0x01}, 1, 0},
    {0x3C, (uint8_t[]){0x70}, 1, 0},
    {0x3D, (uint8_t[]){0xFF}, 1, 0},
    {0x3E, (uint8_t[]){0xFF}, 1, 0},
    {0x3F, (uint8_t[]){0xFF}, 1, 0},

    {0x40, (uint8_t[]){0x06}, 1, 0},
    {0x41, (uint8_t[]){0xA0}, 1, 0},
    {0x43, (uint8_t[]){0x1E}, 1, 0},
    {0x44, (uint8_t[]){0x0F}, 1, 0},
    {0x45, (uint8_t[]){0x28}, 1, 0},
    {0x4B, (uint8_t[]){0x04}, 1, 0},

    //{0x4A, (uint8_t[]){0x35}, 1, 0},//bist

    {0x55, (uint8_t[]){0x02}, 1, 0},
    {0x56, (uint8_t[]){0x01}, 1, 0},
    {0x57, (uint8_t[]){0xA9}, 1, 0},
    {0x58, (uint8_t[]){0x0A}, 1, 0},
    {0x59, (uint8_t[]){0x0A}, 1, 0},
    {0x5A, (uint8_t[]){0x37}, 1, 0},
    {0x5B, (uint8_t[]){0x19}, 1, 0},

    {0x5D, (uint8_t[]){0x78}, 1, 0},
    {0x5E, (uint8_t[]){0x63}, 1, 0},
    {0x5F, (uint8_t[]){0x54}, 1, 0},
    {0x60, (uint8_t[]){0x49}, 1, 0},
    {0x61, (uint8_t[]){0x45}, 1, 0},
    {0x62, (uint8_t[]){0x38}, 1, 0},
    {0x63, (uint8_t[]){0x3D}, 1, 0},
    {0x64, (uint8_t[]){0x28}, 1, 0},
    {0x65, (uint8_t[]){0x43}, 1, 0},
    {0x66, (uint8_t[]){0x41}, 1, 0},
    {0x67, (uint8_t[]){0x43}, 1, 0},
    {0x68, (uint8_t[]){0x62}, 1, 0},
    {0x69, (uint8_t[]){0x50}, 1, 0},
    {0x6A, (uint8_t[]){0x57}, 1, 0},
    {0x6B, (uint8_t[]){0x49}, 1, 0},
    {0x6C, (uint8_t[]){0x44}, 1, 0},
    {0x6D, (uint8_t[]){0x37}, 1, 0},
    {0x6E, (uint8_t[]){0x23}, 1, 0},
    {0x6F, (uint8_t[]){0x10}, 1, 0},
    {0x70, (uint8_t[]){0x78}, 1, 0},
    {0x71, (uint8_t[]){0x63}, 1, 0},
    {0x72, (uint8_t[]){0x54}, 1, 0},
    {0x73, (uint8_t[]){0x49}, 1, 0},
    {0x74, (uint8_t[]){0x45}, 1, 0},
    {0x75, (uint8_t[]){0x38}, 1, 0},
    {0x76, (uint8_t[]){0x3D}, 1, 0},
    {0x77, (uint8_t[]){0x28}, 1, 0},
    {0x78, (uint8_t[]){0x43}, 1, 0},
    {0x79, (uint8_t[]){0x41}, 1, 0},
    {0x7A, (uint8_t[]){0x43}, 1, 0},
    {0x7B, (uint8_t[]){0x62}, 1, 0},
    {0x7C, (uint8_t[]){0x50}, 1, 0},
    {0x7D, (uint8_t[]){0x57}, 1, 0},
    {0x7E, (uint8_t[]){0x49}, 1, 0},
    {0x7F, (uint8_t[]){0x44}, 1, 0},
    {0x80, (uint8_t[]){0x37}, 1, 0},
    {0x81, (uint8_t[]){0x23}, 1, 0},
    {0x82, (uint8_t[]){0x10}, 1, 0},

    {0xE0, (uint8_t[]){0x02}, 1, 0},
    {0x00, (uint8_t[]){0x47}, 1, 0},
    {0x01, (uint8_t[]){0x47}, 1, 0},
    {0x02, (uint8_t[]){0x45}, 1, 0},
    {0x03, (uint8_t[]){0x45}, 1, 0},
    {0x04, (uint8_t[]){0x4B}, 1, 0},
    {0x05, (uint8_t[]){0x4B}, 1, 0},
    {0x06, (uint8_t[]){0x49}, 1, 0},
    {0x07, (uint8_t[]){0x49}, 1, 0},
    {0x08, (uint8_t[]){0x41}, 1, 0},
    {0x09, (uint8_t[]){0x1F}, 1, 0},
    {0x0A, (uint8_t[]){0x1F}, 1, 0},
    {0x0B, (uint8_t[]){0x1F}, 1, 0},
    {0x0C, (uint8_t[]){0x1F}, 1, 0},
    {0x0D, (uint8_t[]){0x1F}, 1, 0},
    {0x0E, (uint8_t[]){0x1F}, 1, 0},
    {0x0F, (uint8_t[]){0x5F}, 1, 0},
    {0x10, (uint8_t[]){0x5F}, 1, 0},
    {0x11, (uint8_t[]){0x57}, 1, 0},
    {0x12, (uint8_t[]){0x77}, 1, 0},
    {0x13, (uint8_t[]){0x35}, 1, 0},
    {0x14, (uint8_t[]){0x1F}, 1, 0},
    {0x15, (uint8_t[]){0x1F}, 1, 0},

    {0x16, (uint8_t[]){0x46}, 1, 0},
    {0x17, (uint8_t[]){0x46}, 1, 0},
    {0x18, (uint8_t[]){0x44}, 1, 0},
    {0x19, (uint8_t[]){0x44}, 1, 0},
    {0x1A, (uint8_t[]){0x4A}, 1, 0},
    {0x1B, (uint8_t[]){0x4A}, 1, 0},
    {0x1C, (uint8_t[]){0x48}, 1, 0},
    {0x1D, (uint8_t[]){0x48}, 1, 0},
    {0x1E, (uint8_t[]){0x40}, 1, 0},
    {0x1F, (uint8_t[]){0x1F}, 1, 0},
    {0x20, (uint8_t[]){0x1F}, 1, 0},
    {0x21, (uint8_t[]){0x1F}, 1, 0},
    {0x22, (uint8_t[]){0x1F}, 1, 0},
    {0x23, (uint8_t[]){0x1F}, 1, 0},
    {0x24, (uint8_t[]){0x1F}, 1, 0},
    {0x25, (uint8_t[]){0x5F}, 1, 0},
    {0x26, (uint8_t[]){0x5F}, 1, 0},
    {0x27, (uint8_t[]){0x57}, 1, 0},
    {0x28, (uint8_t[]){0x77}, 1, 0},
    {0x29, (uint8_t[]){0x35}, 1, 0},
    {0x2A, (uint8_t[]){0x1F}, 1, 0},
    {0x2B, (uint8_t[]){0x1F}, 1, 0},

    {0x58, (uint8_t[]){0x40}, 1, 0},
    {0x59, (uint8_t[]){0x00}, 1, 0},
    {0x5A, (uint8_t[]){0x00}, 1, 0},
    {0x5B, (uint8_t[]){0x10}, 1, 0},
    {0x5C, (uint8_t[]){0x06}, 1, 0},
    {0x5D, (uint8_t[]){0x40}, 1, 0},
    {0x5E, (uint8_t[]){0x01}, 1, 0},
    {0x5F, (uint8_t[]){0x02}, 1, 0},
    {0x60, (uint8_t[]){0x30}, 1, 0},
    {0x61, (uint8_t[]){0x01}, 1, 0},
    {0x62, (uint8_t[]){0x02}, 1, 0},
    {0x63, (uint8_t[]){0x03}, 1, 0},
    {0x64, (uint8_t[]){0x6B}, 1, 0},
    {0x65, (uint8_t[]){0x05}, 1, 0},
    {0x66, (uint8_t[]){0x0C}, 1, 0},
    {0x67, (uint8_t[]){0x73}, 1, 0},
    {0x68, (uint8_t[]){0x09}, 1, 0},
    {0x69, (uint8_t[]){0x03}, 1, 0},
    {0x6A, (uint8_t[]){0x56}, 1, 0},
    {0x6B, (uint8_t[]){0x08}, 1, 0},
    {0x6C, (uint8_t[]){0x00}, 1, 0},
    {0x6D, (uint8_t[]){0x04}, 1, 0},
    {0x6E, (uint8_t[]){0x04}, 1, 0},
    {0x6F, (uint8_t[]){0x88}, 1, 0},
    {0x70, (uint8_t[]){0x00}, 1, 0},
    {0x71, (uint8_t[]){0x00}, 1, 0},
    {0x72, (uint8_t[]){0x06}, 1, 0},
    {0x73, (uint8_t[]){0x7B}, 1, 0},
    {0x74, (uint8_t[]){0x00}, 1, 0},
    {0x75, (uint8_t[]){0xF8}, 1, 0},
    {0x76, (uint8_t[]){0x00}, 1, 0},
    {0x77, (uint8_t[]){0xD5}, 1, 0},
    {0x78, (uint8_t[]){0x2E}, 1, 0},
    {0x79, (uint8_t[]){0x12}, 1, 0},
    {0x7A, (uint8_t[]){0x03}, 1, 0},
    {0x7B, (uint8_t[]){0x00}, 1, 0},
    {0x7C, (uint8_t[]){0x00}, 1, 0},
    {0x7D, (uint8_t[]){0x03}, 1, 0},
    {0x7E, (uint8_t[]){0x7B}, 1, 0},

    {0xE0, (uint8_t[]){0x04}, 1, 0},
    {0x00, (uint8_t[]){0x0E}, 1, 0},
    {0x02, (uint8_t[]){0xB3}, 1, 0},
    {0x09, (uint8_t[]){0x60}, 1, 0},
    {0x0E, (uint8_t[]){0x2A}, 1, 0},
    {0x36, (uint8_t[]){0x59}, 1, 0},
    {0x37, (uint8_t[]){0x58}, 1, 0}, // A133
    {0x2B, (uint8_t[]){0x0F}, 1, 0}, // A133

    {0xE0, (uint8_t[]){0x00}, 1, 0},

    {0x11, (uint8_t[]){0x00}, 1, 120},

    {0x29, (uint8_t[]){0x00}, 1, 20},

    {0x35, (uint8_t[]){0x00}, 1, 0},
};

static const lcd_init_cmd_t kJd9365Lcd10Commands[] = {
    // JD9365 driver preamble: select page 0, RGB565 and two DSI lanes.
    {0xE0, (uint8_t[]){0x00}, 1, 0},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},
    {0x80, (uint8_t[]){0x01}, 1, 0},
    {0xE0, (uint8_t[]){0x00}, 1, 0},
    {0xE1, (uint8_t[]){0x93}, 1, 0},
    {0xE2, (uint8_t[]){0x65}, 1, 0},
    {0xE3, (uint8_t[]){0xF8}, 1, 0},
    {0x80, (uint8_t[]){0x01}, 1, 0},

    {0xE0, (uint8_t[]){0x01}, 1, 0},
    {0x00, (uint8_t[]){0x00}, 1, 0},
    {0x01, (uint8_t[]){0x38}, 1, 0},
    {0x03, (uint8_t[]){0x10}, 1, 0},
    {0x04, (uint8_t[]){0x38}, 1, 0},

    {0x0C, (uint8_t[]){0x74}, 1, 0},

    {0x17, (uint8_t[]){0x00}, 1, 0},
    {0x18, (uint8_t[]){0xAF}, 1, 0},
    {0x19, (uint8_t[]){0x00}, 1, 0},
    {0x1A, (uint8_t[]){0x00}, 1, 0},
    {0x1B, (uint8_t[]){0xAF}, 1, 0},
    {0x1C, (uint8_t[]){0x00}, 1, 0},

    {0x35, (uint8_t[]){0x26}, 1, 0},

    {0x37, (uint8_t[]){0x09}, 1, 0},

    {0x38, (uint8_t[]){0x04}, 1, 0},
    {0x39, (uint8_t[]){0x00}, 1, 0},
    {0x3A, (uint8_t[]){0x01}, 1, 0},
    {0x3C, (uint8_t[]){0x78}, 1, 0},
    {0x3D, (uint8_t[]){0xFF}, 1, 0},
    {0x3E, (uint8_t[]){0xFF}, 1, 0},
    {0x3F, (uint8_t[]){0x7F}, 1, 0},

    {0x40, (uint8_t[]){0x06}, 1, 0},
    {0x41, (uint8_t[]){0xA0}, 1, 0},
    {0x42, (uint8_t[]){0x81}, 1, 0},
    {0x43, (uint8_t[]){0x1E}, 1, 0},
    {0x44, (uint8_t[]){0x0D}, 1, 0},
    {0x45, (uint8_t[]){0x28}, 1, 0},
    //{0x4A, (uint8_t[]){0x35}, 1, 0},//bist

    {0x55, (uint8_t[]){0x02}, 1, 0},
    {0x57, (uint8_t[]){0x69}, 1, 0},
    {0x59, (uint8_t[]){0x0A}, 1, 0},
    {0x5A, (uint8_t[]){0x2A}, 1, 0},
    {0x5B, (uint8_t[]){0x17}, 1, 0},

    {0x5D, (uint8_t[]){0x7F}, 1, 0},
    {0x5E, (uint8_t[]){0x6A}, 1, 0},
    {0x5F, (uint8_t[]){0x5B}, 1, 0},
    {0x60, (uint8_t[]){0x4F}, 1, 0},
    {0x61, (uint8_t[]){0x4A}, 1, 0},
    {0x62, (uint8_t[]){0x3D}, 1, 0},
    {0x63, (uint8_t[]){0x41}, 1, 0},
    {0x64, (uint8_t[]){0x2A}, 1, 0},
    {0x65, (uint8_t[]){0x44}, 1, 0},
    {0x66, (uint8_t[]){0x43}, 1, 0},
    {0x67, (uint8_t[]){0x44}, 1, 0},
    {0x68, (uint8_t[]){0x62}, 1, 0},
    {0x69, (uint8_t[]){0x52}, 1, 0},
    {0x6A, (uint8_t[]){0x59}, 1, 0},
    {0x6B, (uint8_t[]){0x4C}, 1, 0},
    {0x6C, (uint8_t[]){0x48}, 1, 0},
    {0x6D, (uint8_t[]){0x3A}, 1, 0},
    {0x6E, (uint8_t[]){0x26}, 1, 0},
    {0x6F, (uint8_t[]){0x00}, 1, 0},
    {0x70, (uint8_t[]){0x7F}, 1, 0},
    {0x71, (uint8_t[]){0x6A}, 1, 0},
    {0x72, (uint8_t[]){0x5B}, 1, 0},
    {0x73, (uint8_t[]){0x4F}, 1, 0},
    {0x74, (uint8_t[]){0x4A}, 1, 0},
    {0x75, (uint8_t[]){0x3D}, 1, 0},
    {0x76, (uint8_t[]){0x41}, 1, 0},
    {0x77, (uint8_t[]){0x2A}, 1, 0},
    {0x78, (uint8_t[]){0x44}, 1, 0},
    {0x79, (uint8_t[]){0x43}, 1, 0},
    {0x7A, (uint8_t[]){0x44}, 1, 0},
    {0x7B, (uint8_t[]){0x62}, 1, 0},
    {0x7C, (uint8_t[]){0x52}, 1, 0},
    {0x7D, (uint8_t[]){0x59}, 1, 0},
    {0x7E, (uint8_t[]){0x4C}, 1, 0},
    {0x7F, (uint8_t[]){0x48}, 1, 0},
    {0x80, (uint8_t[]){0x3A}, 1, 0},
    {0x81, (uint8_t[]){0x26}, 1, 0},
    {0x82, (uint8_t[]){0x00}, 1, 0},

    {0xE0, (uint8_t[]){0x02}, 1, 0},
    {0x00, (uint8_t[]){0x42}, 1, 0},
    {0x01, (uint8_t[]){0x42}, 1, 0},
    {0x02, (uint8_t[]){0x40}, 1, 0},
    {0x03, (uint8_t[]){0x40}, 1, 0},
    {0x04, (uint8_t[]){0x5E}, 1, 0},
    {0x05, (uint8_t[]){0x5E}, 1, 0},
    {0x06, (uint8_t[]){0x5F}, 1, 0},
    {0x07, (uint8_t[]){0x5F}, 1, 0},
    {0x08, (uint8_t[]){0x5F}, 1, 0},
    {0x09, (uint8_t[]){0x57}, 1, 0},
    {0x0A, (uint8_t[]){0x57}, 1, 0},
    {0x0B, (uint8_t[]){0x77}, 1, 0},
    {0x0C, (uint8_t[]){0x77}, 1, 0},
    {0x0D, (uint8_t[]){0x47}, 1, 0},
    {0x0E, (uint8_t[]){0x47}, 1, 0},
    {0x0F, (uint8_t[]){0x45}, 1, 0},
    {0x10, (uint8_t[]){0x45}, 1, 0},
    {0x11, (uint8_t[]){0x4B}, 1, 0},
    {0x12, (uint8_t[]){0x4B}, 1, 0},
    {0x13, (uint8_t[]){0x49}, 1, 0},
    {0x14, (uint8_t[]){0x49}, 1, 0},
    {0x15, (uint8_t[]){0x5F}, 1, 0},

    {0x16, (uint8_t[]){0x41}, 1, 0},
    {0x17, (uint8_t[]){0x41}, 1, 0},
    {0x18, (uint8_t[]){0x40}, 1, 0},
    {0x19, (uint8_t[]){0x40}, 1, 0},
    {0x1A, (uint8_t[]){0x5E}, 1, 0},
    {0x1B, (uint8_t[]){0x5E}, 1, 0},
    {0x1C, (uint8_t[]){0x5F}, 1, 0},
    {0x1D, (uint8_t[]){0x5F}, 1, 0},
    {0x1E, (uint8_t[]){0x5F}, 1, 0},
    {0x1F, (uint8_t[]){0x57}, 1, 0},
    {0x20, (uint8_t[]){0x57}, 1, 0},
    {0x21, (uint8_t[]){0x77}, 1, 0},
    {0x22, (uint8_t[]){0x77}, 1, 0},
    {0x23, (uint8_t[]){0x46}, 1, 0},
    {0x24, (uint8_t[]){0x46}, 1, 0},
    {0x25, (uint8_t[]){0x44}, 1, 0},
    {0x26, (uint8_t[]){0x44}, 1, 0},
    {0x27, (uint8_t[]){0x4A}, 1, 0},
    {0x28, (uint8_t[]){0x4A}, 1, 0},
    {0x29, (uint8_t[]){0x48}, 1, 0},
    {0x2A, (uint8_t[]){0x48}, 1, 0},
    {0x2B, (uint8_t[]){0x5F}, 1, 0},

    {0x2C, (uint8_t[]){0x01}, 1, 0},
    {0x2D, (uint8_t[]){0x01}, 1, 0},
    {0x2E, (uint8_t[]){0x00}, 1, 0},
    {0x2F, (uint8_t[]){0x00}, 1, 0},
    {0x30, (uint8_t[]){0x1F}, 1, 0},
    {0x31, (uint8_t[]){0x1F}, 1, 0},
    {0x32, (uint8_t[]){0x1E}, 1, 0},
    {0x33, (uint8_t[]){0x1E}, 1, 0},
    {0x34, (uint8_t[]){0x1F}, 1, 0},
    {0x35, (uint8_t[]){0x17}, 1, 0},
    {0x36, (uint8_t[]){0x17}, 1, 0},
    {0x37, (uint8_t[]){0x37}, 1, 0},
    {0x38, (uint8_t[]){0x37}, 1, 0},
    {0x39, (uint8_t[]){0x08}, 1, 0},
    {0x3A, (uint8_t[]){0x08}, 1, 0},
    {0x3B, (uint8_t[]){0x0A}, 1, 0},
    {0x3C, (uint8_t[]){0x0A}, 1, 0},
    {0x3D, (uint8_t[]){0x04}, 1, 0},
    {0x3E, (uint8_t[]){0x04}, 1, 0},
    {0x3F, (uint8_t[]){0x06}, 1, 0},
    {0x40, (uint8_t[]){0x06}, 1, 0},
    {0x41, (uint8_t[]){0x1F}, 1, 0},

    {0x42, (uint8_t[]){0x02}, 1, 0},
    {0x43, (uint8_t[]){0x02}, 1, 0},
    {0x44, (uint8_t[]){0x00}, 1, 0},
    {0x45, (uint8_t[]){0x00}, 1, 0},
    {0x46, (uint8_t[]){0x1F}, 1, 0},
    {0x47, (uint8_t[]){0x1F}, 1, 0},
    {0x48, (uint8_t[]){0x1E}, 1, 0},
    {0x49, (uint8_t[]){0x1E}, 1, 0},
    {0x4A, (uint8_t[]){0x1F}, 1, 0},
    {0x4B, (uint8_t[]){0x17}, 1, 0},
    {0x4C, (uint8_t[]){0x17}, 1, 0},
    {0x4D, (uint8_t[]){0x37}, 1, 0},
    {0x4E, (uint8_t[]){0x37}, 1, 0},
    {0x4F, (uint8_t[]){0x09}, 1, 0},
    {0x50, (uint8_t[]){0x09}, 1, 0},
    {0x51, (uint8_t[]){0x0B}, 1, 0},
    {0x52, (uint8_t[]){0x0B}, 1, 0},
    {0x53, (uint8_t[]){0x05}, 1, 0},
    {0x54, (uint8_t[]){0x05}, 1, 0},
    {0x55, (uint8_t[]){0x07}, 1, 0},
    {0x56, (uint8_t[]){0x07}, 1, 0},
    {0x57, (uint8_t[]){0x1F}, 1, 0},

    {0x58, (uint8_t[]){0x40}, 1, 0},
    {0x5B, (uint8_t[]){0x30}, 1, 0},
    {0x5C, (uint8_t[]){0x00}, 1, 0},
    {0x5D, (uint8_t[]){0x34}, 1, 0},
    {0x5E, (uint8_t[]){0x05}, 1, 0},
    {0x5F, (uint8_t[]){0x02}, 1, 0},
    {0x63, (uint8_t[]){0x00}, 1, 0},
    {0x64, (uint8_t[]){0x6A}, 1, 0},
    {0x67, (uint8_t[]){0x73}, 1, 0},
    {0x68, (uint8_t[]){0x07}, 1, 0},
    {0x69, (uint8_t[]){0x08}, 1, 0},
    {0x6A, (uint8_t[]){0x6A}, 1, 0},
    {0x6B, (uint8_t[]){0x08}, 1, 0},

    {0x6C, (uint8_t[]){0x00}, 1, 0},
    {0x6D, (uint8_t[]){0x00}, 1, 0},
    {0x6E, (uint8_t[]){0x00}, 1, 0},
    {0x6F, (uint8_t[]){0x88}, 1, 0},

    {0x75, (uint8_t[]){0xFF}, 1, 0},
    {0x77, (uint8_t[]){0xDD}, 1, 0},
    {0x78, (uint8_t[]){0x2C}, 1, 0},
    {0x79, (uint8_t[]){0x15}, 1, 0},
    {0x7A, (uint8_t[]){0x17}, 1, 0},
    {0x7D, (uint8_t[]){0x14}, 1, 0},
    {0x7E, (uint8_t[]){0x82}, 1, 0},

    {0xE0, (uint8_t[]){0x04}, 1, 0},
    {0x00, (uint8_t[]){0x0E}, 1, 0},
    {0x02, (uint8_t[]){0xB3}, 1, 0},
    {0x09, (uint8_t[]){0x61}, 1, 0},
    {0x0E, (uint8_t[]){0x48}, 1, 0},
    {0x37, (uint8_t[]){0x58}, 1, 0}, // 全志
    {0x2B, (uint8_t[]){0x0F}, 1, 0}, // 全志

    {0xE0, (uint8_t[]){0x00}, 1, 0},

    {0xE6, (uint8_t[]){0x02}, 1, 0},
    {0xE7, (uint8_t[]){0x0C}, 1, 0},

    {0x11, (uint8_t[]){0x00}, 1, 120},

    {0x29, (uint8_t[]){0x00}, 1, 20},
};

static const lcd_init_cmd_t kIli9881cLcd7Commands[] = {
    // ILI9881C driver preamble: select two DSI lanes, exit sleep and use RGB565.
    {0xFF, (uint8_t[]){0x98, 0x81, 0x01}, 3, 0},
    {0xB7, (uint8_t[]){0x03}, 1, 0},
    {0xFF, (uint8_t[]){0x98, 0x81, 0x00}, 3, 0},
    {0x11, nullptr, 0, 120},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},
    // {cmd, { data }, data_size, delay_ms}
    /**** CMD_Page 3 ****/
    {0xFF, (uint8_t[]){0x98, 0x81, 0x03}, 3, 0},
    // {0x01, (uint8_t []){0x00}, 1, 0},
    {0x01, (uint8_t[]){0x00}, 1, 0},
    {0x02, (uint8_t[]){0x00}, 1, 0},
    {0x03, (uint8_t[]){0x73}, 1, 0},
    {0x04, (uint8_t[]){0x00}, 1, 0},
    {0x05, (uint8_t[]){0x00}, 1, 0},
    {0x06, (uint8_t[]){0x0A}, 1, 0},
    {0x07, (uint8_t[]){0x00}, 1, 0},
    {0x08, (uint8_t[]){0x00}, 1, 0},
    {0x09, (uint8_t[]){0x61}, 1, 0},
    {0x0A, (uint8_t[]){0x00}, 1, 0},
    {0x0B, (uint8_t[]){0x00}, 1, 0},
    {0x0C, (uint8_t[]){0x01}, 1, 0},
    {0x0D, (uint8_t[]){0x00}, 1, 0},
    {0x0E, (uint8_t[]){0x00}, 1, 0},
    {0x0F, (uint8_t[]){0x61}, 1, 0},
    {0x10, (uint8_t[]){0x61}, 1, 0},
    {0x11, (uint8_t[]){0x00}, 1, 0},
    {0x12, (uint8_t[]){0x00}, 1, 0},
    {0x13, (uint8_t[]){0x00}, 1, 0},
    {0x14, (uint8_t[]){0x00}, 1, 0},
    {0x15, (uint8_t[]){0x00}, 1, 0},
    {0x16, (uint8_t[]){0x00}, 1, 0},
    {0x17, (uint8_t[]){0x00}, 1, 0},
    {0x18, (uint8_t[]){0x00}, 1, 0},
    {0x19, (uint8_t[]){0x00}, 1, 0},
    {0x1A, (uint8_t[]){0x00}, 1, 0},
    {0x1B, (uint8_t[]){0x00}, 1, 0},
    {0x1C, (uint8_t[]){0x00}, 1, 0},
    {0x1D, (uint8_t[]){0x00}, 1, 0},
    {0x1E, (uint8_t[]){0x40}, 1, 0},
    {0x1F, (uint8_t[]){0x80}, 1, 0},
    {0x20, (uint8_t[]){0x06}, 1, 0},
    {0x21, (uint8_t[]){0x01}, 1, 0},
    {0x22, (uint8_t[]){0x00}, 1, 0},
    {0x23, (uint8_t[]){0x00}, 1, 0},
    {0x24, (uint8_t[]){0x00}, 1, 0},
    {0x25, (uint8_t[]){0x00}, 1, 0},
    {0x26, (uint8_t[]){0x00}, 1, 0},
    {0x27, (uint8_t[]){0x00}, 1, 0},
    {0x28, (uint8_t[]){0x33}, 1, 0},
    {0x29, (uint8_t[]){0x03}, 1, 0},
    {0x2A, (uint8_t[]){0x00}, 1, 0},
    {0x2B, (uint8_t[]){0x00}, 1, 0},
    {0x2C, (uint8_t[]){0x00}, 1, 0},
    {0x2D, (uint8_t[]){0x00}, 1, 0},
    {0x2E, (uint8_t[]){0x00}, 1, 0},
    {0x2F, (uint8_t[]){0x00}, 1, 0},
    {0x30, (uint8_t[]){0x00}, 1, 0},
    {0x31, (uint8_t[]){0x00}, 1, 0},
    {0x32, (uint8_t[]){0x00}, 1, 0},
    {0x33, (uint8_t[]){0x00}, 1, 0},
    {0x34, (uint8_t[]){0x04}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x37, (uint8_t[]){0x00}, 1, 0},
    {0x38, (uint8_t[]){0x3C}, 1, 0},
    {0x39, (uint8_t[]){0x00}, 1, 0},
    {0x3A, (uint8_t[]){0x00}, 1, 0},
    {0x3B, (uint8_t[]){0x00}, 1, 0},
    {0x3C, (uint8_t[]){0x00}, 1, 0},
    {0x3D, (uint8_t[]){0x00}, 1, 0},
    {0x3E, (uint8_t[]){0x00}, 1, 0},
    {0x3F, (uint8_t[]){0x00}, 1, 0},
    {0x40, (uint8_t[]){0x00}, 1, 0},
    {0x41, (uint8_t[]){0x00}, 1, 0},
    {0x42, (uint8_t[]){0x00}, 1, 0},
    {0x43, (uint8_t[]){0x00}, 1, 0},
    {0x44, (uint8_t[]){0x00}, 1, 0},
    {0x50, (uint8_t[]){0x10}, 1, 0},
    {0x51, (uint8_t[]){0x32}, 1, 0},
    {0x52, (uint8_t[]){0x54}, 1, 0},
    {0x53, (uint8_t[]){0x76}, 1, 0},
    {0x54, (uint8_t[]){0x98}, 1, 0},
    {0x55, (uint8_t[]){0xBA}, 1, 0},
    {0x56, (uint8_t[]){0x10}, 1, 0},
    {0x57, (uint8_t[]){0x32}, 1, 0},
    {0x58, (uint8_t[]){0x54}, 1, 0},
    {0x59, (uint8_t[]){0x76}, 1, 0},
    {0x5A, (uint8_t[]){0x98}, 1, 0},
    {0x5B, (uint8_t[]){0xBA}, 1, 0},
    {0x5C, (uint8_t[]){0xDC}, 1, 0},
    {0x5D, (uint8_t[]){0xFE}, 1, 0},
    {0x5E, (uint8_t[]){0x00}, 1, 0},
    {0x5F, (uint8_t[]){0x0E}, 1, 0},
    {0x60, (uint8_t[]){0x0F}, 1, 0},
    {0x61, (uint8_t[]){0x0C}, 1, 0},
    {0x62, (uint8_t[]){0x0D}, 1, 0},
    {0x63, (uint8_t[]){0x06}, 1, 0},
    {0x64, (uint8_t[]){0x07}, 1, 0},
    {0x65, (uint8_t[]){0x02}, 1, 0},
    {0x66, (uint8_t[]){0x02}, 1, 0},
    {0x67, (uint8_t[]){0x02}, 1, 0},
    {0x68, (uint8_t[]){0x02}, 1, 0},
    {0x69, (uint8_t[]){0x01}, 1, 0},
    {0x6A, (uint8_t[]){0x00}, 1, 0},
    {0x6B, (uint8_t[]){0x02}, 1, 0},
    {0x6C, (uint8_t[]){0x15}, 1, 0},
    {0x6D, (uint8_t[]){0x14}, 1, 0},
    {0x6E, (uint8_t[]){0x02}, 1, 0},
    {0x6F, (uint8_t[]){0x02}, 1, 0},
    {0x70, (uint8_t[]){0x02}, 1, 0},
    {0x71, (uint8_t[]){0x02}, 1, 0},
    {0x72, (uint8_t[]){0x02}, 1, 0},
    {0x73, (uint8_t[]){0x02}, 1, 0},
    {0x74, (uint8_t[]){0x02}, 1, 0},
    {0x75, (uint8_t[]){0x0E}, 1, 0},
    {0x76, (uint8_t[]){0x0F}, 1, 0},
    {0x77, (uint8_t[]){0x0C}, 1, 0},
    {0x78, (uint8_t[]){0x0D}, 1, 0},
    {0x79, (uint8_t[]){0x06}, 1, 0},
    {0x7A, (uint8_t[]){0x07}, 1, 0},
    {0x7B, (uint8_t[]){0x02}, 1, 0},
    {0x7C, (uint8_t[]){0x02}, 1, 0},
    {0x7D, (uint8_t[]){0x02}, 1, 0},
    {0x7E, (uint8_t[]){0x02}, 1, 0},
    {0x7F, (uint8_t[]){0x01}, 1, 0},
    {0x80, (uint8_t[]){0x00}, 1, 0},
    {0x81, (uint8_t[]){0x02}, 1, 0},
    {0x82, (uint8_t[]){0x14}, 1, 0},
    {0x83, (uint8_t[]){0x15}, 1, 0},
    {0x84, (uint8_t[]){0x02}, 1, 0},
    {0x85, (uint8_t[]){0x02}, 1, 0},
    {0x86, (uint8_t[]){0x02}, 1, 0},
    {0x87, (uint8_t[]){0x02}, 1, 0},
    {0x88, (uint8_t[]){0x02}, 1, 0},
    {0x89, (uint8_t[]){0x02}, 1, 0},
    {0x8A, (uint8_t[]){0x02}, 1, 0},

    {0xFF, (uint8_t[]){0x98, 0x81, 0x04}, 3, 0},
    {0x38, (uint8_t[]){0x01}, 1, 0},
    {0x39, (uint8_t[]){0x00}, 1, 0},
    {0x6C, (uint8_t[]){0x15}, 1, 0},
    {0x6E, (uint8_t[]){0x2A}, 1, 0},
    {0x6F, (uint8_t[]){0x33}, 1, 0},
    {0x3A, (uint8_t[]){0x94}, 1, 0},
    {0x8D, (uint8_t[]){0x14}, 1, 0},
    {0x87, (uint8_t[]){0xBA}, 1, 0},
    {0x26, (uint8_t[]){0x76}, 1, 0},
    {0xB2, (uint8_t[]){0xD1}, 1, 0},
    {0xB5, (uint8_t[]){0x06}, 1, 0},
    {0X3B, (uint8_t[]){0X98}, 1, 0},
    {0xFF, (uint8_t[]){0x98, 0x81, 0x01}, 3, 0},
    {0x22, (uint8_t[]){0x0A}, 1, 0},
    {0x31, (uint8_t[]){0x00}, 1, 0},
    {0x53, (uint8_t[]){0x71}, 1, 0},
    {0x55, (uint8_t[]){0x8F}, 1, 0},
    {0x40, (uint8_t[]){0x33}, 1, 0},
    {0x50, (uint8_t[]){0x96}, 1, 0},
    {0x51, (uint8_t[]){0x96}, 1, 0},
    {0x60, (uint8_t[]){0x23}, 1, 0},
    {0xA0, (uint8_t[]){0x08}, 1, 0},
    {0xA1, (uint8_t[]){0x1D}, 1, 0},
    {0xA2, (uint8_t[]){0x2A}, 1, 0},
    {0xA3, (uint8_t[]){0x10}, 1, 0},
    {0xA4, (uint8_t[]){0x15}, 1, 0},
    {0xA5, (uint8_t[]){0x28}, 1, 0},
    {0xA6, (uint8_t[]){0x1C}, 1, 0},
    {0xA7, (uint8_t[]){0x1D}, 1, 0},
    {0xA8, (uint8_t[]){0x7E}, 1, 0},
    {0xA9, (uint8_t[]){0x1D}, 1, 0},
    {0xAA, (uint8_t[]){0x29}, 1, 0},
    {0xAB, (uint8_t[]){0x6B}, 1, 0},
    {0xAC, (uint8_t[]){0x1A}, 1, 0},
    {0xAD, (uint8_t[]){0x18}, 1, 0},
    {0xAE, (uint8_t[]){0x4B}, 1, 0},
    {0xAF, (uint8_t[]){0x20}, 1, 0},
    {0xB0, (uint8_t[]){0x27}, 1, 0},
    {0xB1, (uint8_t[]){0x50}, 1, 0},
    {0xB2, (uint8_t[]){0x64}, 1, 0},
    {0xB3, (uint8_t[]){0x39}, 1, 0},
    {0xC0, (uint8_t[]){0x08}, 1, 0},
    {0xC1, (uint8_t[]){0x1D}, 1, 0},
    {0xC2, (uint8_t[]){0x2A}, 1, 0},
    {0xC3, (uint8_t[]){0x10}, 1, 0},
    {0xC4, (uint8_t[]){0x15}, 1, 0},
    {0xC5, (uint8_t[]){0x28}, 1, 0},
    {0xC6, (uint8_t[]){0x1C}, 1, 0},
    {0xC7, (uint8_t[]){0x1D}, 1, 0},
    {0xC8, (uint8_t[]){0x7E}, 1, 0},
    {0xC9, (uint8_t[]){0x1D}, 1, 0},
    {0xCA, (uint8_t[]){0x29}, 1, 0},
    {0xCB, (uint8_t[]){0x6B}, 1, 0},
    {0xCC, (uint8_t[]){0x1A}, 1, 0},
    {0xCD, (uint8_t[]){0x18}, 1, 0},
    {0xCE, (uint8_t[]){0x4B}, 1, 0},
    {0xCF, (uint8_t[]){0x20}, 1, 0},
    {0xD0, (uint8_t[]){0x27}, 1, 0},
    {0xD1, (uint8_t[]){0x50}, 1, 0},
    {0xD2, (uint8_t[]){0x64}, 1, 0},
    {0xD3, (uint8_t[]){0x39}, 1, 0},

    {0xFF, (uint8_t[]){0x98, 0x81, 0x00}, 3, 0},
    {0x3A, (uint8_t[]){0x77}, 1, 0},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x11, (uint8_t[]){0x00}, 0, 150},

    {0x29, (uint8_t[]){0x00}, 0, 20},

    //============ Gamma END===========
};

// LCD-7 needs the ILI9881C DSI bridge enable sequence on I2C address 0x45.
// It is intentionally separate from panel commands because it targets the board
// bridge, not the DSI panel. The caller must run this before gfx->begin().
inline bool display_cfg_prepare() {
#if LCD_X_DISPLAY_VARIANT == LCD_X_DISPLAY_7
  DEV_I2C_Port port = DEV_I2C_Init();
  if (port.bus == NULL) {
    return false;
  }

  const i2c_device_config_t bridge_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = 0x45,
      .scl_speed_hz = 100000,
  };
  i2c_master_dev_handle_t bridge = NULL;
  if (i2c_master_bus_add_device(port.bus, &bridge_config, &bridge) != ESP_OK) {
    return false;
  }

  const uint8_t bridge_setup[][2] = {
      {0x95, 0x11},
      {0x95, 0x17},
      {0x96, 0x00},
  };
  bool ok = true;
  for (const auto &write : bridge_setup) {
    ok = ok && (i2c_master_transmit(bridge, write, sizeof(write), 100) == ESP_OK);
  }
  delay(100);
  const uint8_t bridge_enable[] = {0x96, 0xFF};
  ok = ok && (i2c_master_transmit(bridge, bridge_enable, sizeof(bridge_enable), 100) == ESP_OK);
  i2c_master_bus_rm_device(bridge);
  delay(1000);
  return ok;
#else
  return true;
#endif
}

const DisplayConfig SCREEN_LCD_7 = {
    .name = "LCD-X-7-ILI9881C",
    .hsync_pulse_width = 50,
    .hsync_back_porch = 239,
    .hsync_front_porch = 33,
    .vsync_pulse_width = 30,
    .vsync_back_porch = 20,
    .vsync_front_porch = 2,
    .prefer_speed = 80000000,
    .lane_bit_rate = 1000,
    .width = 720,
    .height = 1280,
    .rotation = 0,
    .auto_flush = true,
    .rst_pin = -1,
    .init_cmds = kIli9881cLcd7Commands,
    .init_cmds_size = sizeof(kIli9881cLcd7Commands) / sizeof(lcd_init_cmd_t),
    .i2c_sda_pin = 7,
    .i2c_scl_pin = 8,
    .i2c_clock_speed = 100000,
    .lcd_rst = 27,
};

const DisplayConfig SCREEN_LCD_8 = {
    .name = "LCD-X-8-JD9365",
    .hsync_pulse_width = 20,
    .hsync_back_porch = 20,
    .hsync_front_porch = 40,
    .vsync_pulse_width = 4,
    .vsync_back_porch = 12,
    .vsync_front_porch = 30,
    .prefer_speed = 80000000,
    .lane_bit_rate = 1500,
    .width = 800,
    .height = 1280,
    .rotation = 0,
    .auto_flush = true,
    .rst_pin = -1,
    .init_cmds = kJd9365Lcd8Commands,
    .init_cmds_size = sizeof(kJd9365Lcd8Commands) / sizeof(lcd_init_cmd_t),
    .i2c_sda_pin = 7,
    .i2c_scl_pin = 8,
    .i2c_clock_speed = 100000,
    .lcd_rst = 27,
};

const DisplayConfig SCREEN_LCD_10_1 = {
    .name = "LCD-X-10.1-JD9365",
    .hsync_pulse_width = 20,
    .hsync_back_porch = 20,
    .hsync_front_porch = 40,
    .vsync_pulse_width = 4,
    .vsync_back_porch = 12,
    .vsync_front_porch = 30,
    .prefer_speed = 80000000,
    .lane_bit_rate = 1500,
    .width = 800,
    .height = 1280,
    .rotation = 0,
    .auto_flush = true,
    .rst_pin = -1,
    .init_cmds = kJd9365Lcd10Commands,
    .init_cmds_size = sizeof(kJd9365Lcd10Commands) / sizeof(lcd_init_cmd_t),
    .i2c_sda_pin = 7,
    .i2c_scl_pin = 8,
    .i2c_clock_speed = 100000,
    .lcd_rst = 27,
};

#if LCD_X_DISPLAY_VARIANT == LCD_X_DISPLAY_7
inline const DisplayConfig& display_cfg = SCREEN_LCD_7;
#elif LCD_X_DISPLAY_VARIANT == LCD_X_DISPLAY_8
inline const DisplayConfig& display_cfg = SCREEN_LCD_8;
#else
inline const DisplayConfig& display_cfg = SCREEN_LCD_10_1;
#endif

#endif
