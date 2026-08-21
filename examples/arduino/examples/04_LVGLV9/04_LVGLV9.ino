/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(BOARD_HAS_PSRAM)
#error "This example requires PSRAM. Enable PSRAM in the Arduino IDE Tools menu."
#endif

#include <esp_heap_caps.h>
#include <lcd_x_board.h>
#include <lvgl.h>

lcd_x::Display display;
lcd_x::Touch touch;

namespace {

constexpr uint32_t kDrawBufferRows = 40;
constexpr uint32_t kLoopDelayMs = 5;

lv_display_t *lvgl_display = nullptr;
lv_indev_t *lvgl_touch = nullptr;
uint8_t *draw_buffer_a = nullptr;
uint8_t *draw_buffer_b = nullptr;
uint32_t last_tick_ms = 0;

void halt_with_error(const char *message)
{
    Serial.println(message);
    while (true) {
        delay(1000);
    }
}

void display_flush(lv_display_t *lv_display, const lv_area_t *area, uint8_t *pixel_map)
{
    const int width = area->x2 - area->x1 + 1;
    const int height = area->y2 - area->y1 + 1;
    display.draw_rgb565_bitmap(area->x1, area->y1, width, height,
                               reinterpret_cast<const uint16_t *>(pixel_map));
    lv_display_flush_ready(lv_display);
}

void touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    lcd_x::TouchPoint points[lcd_x::Touch::MAX_POINTS];
    size_t count = 0;

    if (touch.read(points, lcd_x::Touch::MAX_POINTS, &count) && count > 0 &&
        points[0].x < display.width() && points[0].y < display.height()) {
        data->point.x = points[0].x;
        data->point.y = points[0].y;
        data->state = LV_INDEV_STATE_PRESSED;
        return;
    }

    data->state = LV_INDEV_STATE_RELEASED;
}

void create_widgets()
{
    lv_obj_t *title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "LCD-X + LVGL v9");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *hint = lv_label_create(lv_screen_active());
    lv_label_set_text(hint, "Touch the slider to adjust it");
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 64);

    lv_obj_t *slider = lv_slider_create(lv_screen_active());
    lv_obj_set_width(slider, display.width() * 2 / 3);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);

    lv_obj_t *button = lv_button_create(lv_screen_active());
    lv_obj_align(button, LV_ALIGN_CENTER, 0, 80);
    lv_obj_t *button_label = lv_label_create(button);
    lv_label_set_text(button_label, "LVGL ready");
    lv_obj_center(button_label);
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-P4-WIFI6-Touch-LCD-X LVGL v9");

    if (!display.begin()) {
        halt_with_error("Display initialization failed");
    }
    if (!touch.begin(display.width(), display.height())) {
        halt_with_error("GT911-compatible touch controller unavailable");
    }

    lv_init();

    const size_t buffer_bytes = static_cast<size_t>(display.width()) * kDrawBufferRows *
                                LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565);
    draw_buffer_a = static_cast<uint8_t *>(
        heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    draw_buffer_b = static_cast<uint8_t *>(
        heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!draw_buffer_a || !draw_buffer_b) {
        halt_with_error("LVGL draw-buffer allocation failed");
    }

    lvgl_display = lv_display_create(display.width(), display.height());
    if (!lvgl_display) {
        halt_with_error("LVGL display allocation failed");
    }
    lv_display_set_color_format(lvgl_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(lvgl_display, display_flush);
    lv_display_set_buffers(lvgl_display, draw_buffer_a, draw_buffer_b, buffer_bytes,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lvgl_touch = lv_indev_create();
    if (!lvgl_touch) {
        halt_with_error("LVGL touch allocation failed");
    }
    lv_indev_set_type(lvgl_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvgl_touch, touch_read);

    create_widgets();
    last_tick_ms = millis();
    Serial.printf("Touch ready at 0x%02X, product ID: %s\n", touch.address(), touch.product_id());
}

void loop()
{
    const uint32_t now = millis();
    lv_tick_inc(now - last_tick_ms);
    last_tick_ms = now;
    lv_timer_handler();
    delay(kLoopDelayMs);
}
