/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lcd_x_board.h>

lcd_x::Display display;

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-P4-WIFI6-Touch-LCD-X display color bars");

    if (!display.begin()) {
        Serial.println("Display initialization failed");
        return;
    }

    const uint16_t colors[] = {
        lcd_x::WHITE,
        lcd_x::YELLOW,
        lcd_x::CYAN,
        lcd_x::GREEN,
        lcd_x::MAGENTA,
        lcd_x::RED,
        lcd_x::BLUE,
        lcd_x::BLACK,
    };
    const int bar_width = display.width() / 8;
    for (int index = 0; index < 8; ++index) {
        const int x = index * bar_width;
        const int width = index == 7 ? display.width() - x : bar_width;
        display.fill_rect(x, 0, width, display.height(), colors[index]);
    }

    Serial.printf("Display ready: %u x %u, variant %u\n",
                  display.width(), display.height(),
                  static_cast<unsigned>(display.variant()));
}

void loop()
{
    delay(1000);
}
