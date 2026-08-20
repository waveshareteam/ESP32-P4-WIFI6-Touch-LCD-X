/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lcd_x_board.h>

lcd_x::Display display;
lcd_x::Touch touch;

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-P4-WIFI6-Touch-LCD-X polling touch drawing");

    if (!display.begin()) {
        Serial.println("Display initialization failed");
        return;
    }
    display.fill_screen(lcd_x::WHITE);

    if (!touch.begin(display.width(), display.height())) {
        Serial.println("GT911-compatible touch controller unavailable");
        return;
    }

    Serial.printf("Touch ready at 0x%02X, product ID: %s\n",
                  touch.address(), touch.product_id());
}

void loop()
{
    lcd_x::TouchPoint points[lcd_x::Touch::MAX_POINTS];
    size_t count = 0;

    if (!touch.read(points, lcd_x::Touch::MAX_POINTS, &count)) {
        delay(10);
        return;
    }

    for (size_t index = 0; index < count; ++index) {
        if (points[index].x < display.width() && points[index].y < display.height()) {
            display.fill_circle(points[index].x, points[index].y, 6, lcd_x::BLUE);
        }
    }
}
