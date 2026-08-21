/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(BOARD_HAS_PSRAM)
#error "This example requires PSRAM. Enable PSRAM in the Arduino IDE Tools menu."
#endif

#include <lcd_x_board.h>

lcd_x::Display display;

namespace {

constexpr char kTitle[] = "Printable ASCII 0x20-0x7E";
constexpr int kColumns = 16;
constexpr int kRows = 6;
constexpr int kTextScale = 2;
constexpr int kGlyphWidth = 8 * kTextScale;
constexpr int kRowHeaderWidth = 48;
constexpr int kTableTop = 108;
constexpr int kCellHeight = 26;
constexpr uint8_t kFirstPrintableAscii = 32;
constexpr uint8_t kLastPrintableAscii = 126;
constexpr char kHexDigits[] = "0123456789ABCDEF";

void halt_with_error(const char *message)
{
    Serial.println(message);
    while (true) {
        delay(1000);
    }
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-P4-WIFI6-Touch-LCD-X ASCII table");

    if (!display.begin()) {
        halt_with_error("Display initialization failed");
    }

    display.fill_screen(lcd_x::BLACK);

    const int cell_width = (display.width() - kRowHeaderWidth) / kColumns;
    if (cell_width < kGlyphWidth + 4 ||
        kTableTop + kRows * kCellHeight > display.height()) {
        halt_with_error("Display is too small for the ASCII table");
    }

    const int title_width = (sizeof(kTitle) - 1) * kGlyphWidth;
    display.draw_text(max(0, (static_cast<int>(display.width()) - title_width) / 2), 28,
                      kTitle, lcd_x::GREEN, lcd_x::BLACK, kTextScale);

    for (int column = 0; column < kColumns; ++column) {
        const char label[] = {kHexDigits[column], '\0'};
        const int x = kRowHeaderWidth + column * cell_width;
        display.draw_text(x + (cell_width - kGlyphWidth) / 2, kTableTop - kCellHeight,
                          label, lcd_x::YELLOW, lcd_x::BLACK, kTextScale);
    }

    for (int row = 0; row < kRows; ++row) {
        const uint8_t row_base = kFirstPrintableAscii + row * kColumns;
        const char row_label[] = {
            kHexDigits[row_base >> 4],
            kHexDigits[row_base & 0x0F],
            '\0',
        };
        const int y = kTableTop + row * kCellHeight;
        display.draw_text(8, y + (kCellHeight - 8 * kTextScale) / 2,
                          row_label, lcd_x::CYAN, lcd_x::BLACK, kTextScale);

        for (int column = 0; column < kColumns; ++column) {
            const int x = kRowHeaderWidth + column * cell_width;
            display.draw_rect(x, y, cell_width, kCellHeight, lcd_x::BLUE);
            const uint8_t character = row_base + column;
            if (character <= kLastPrintableAscii) {
                display.draw_char(x + (cell_width - kGlyphWidth) / 2,
                                  y + (kCellHeight - 8 * kTextScale) / 2,
                                  static_cast<char>(character), lcd_x::WHITE,
                                  lcd_x::BLACK, kTextScale);
            }
        }
    }
}

void loop()
{
    delay(1000);
}
