/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lcd_x_board.h"

#include <algorithm>
#include <string.h>

#include "esp_cache.h"
#include "esp_err.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_panel_commands.h"

namespace lcd_x {
namespace {

constexpr i2c_port_num_t kI2cPort = I2C_NUM_1;
constexpr gpio_num_t kI2cSda = GPIO_NUM_7;
constexpr gpio_num_t kI2cScl = GPIO_NUM_8;
constexpr uint32_t kI2cFrequencyHz = 400000;
constexpr gpio_num_t kLcdReset = GPIO_NUM_27;
constexpr uint8_t kBacklightPin = 26;
constexpr uint32_t kBacklightFrequencyHz = 5000;
constexpr uint8_t kBacklightResolutionBits = 10;
constexpr uint8_t kGt911Address = 0x5D;
constexpr uint8_t kGt911BackupAddress = 0x14;
constexpr uint16_t kGt911ProductIdRegister = 0x8140;
constexpr uint16_t kGt911TouchStatusRegister = 0x814E;

struct LcdInitCommand {
    int command;
    const void *data;
    size_t data_size;
    uint32_t delay_ms;
};

#include "lcd_x_panel_commands.inc"

struct PanelConfig {
    DisplayVariant variant;
    uint16_t width;
    uint16_t height;
    uint16_t lane_bit_rate_mbps;
    float dpi_clock_mhz;
    uint16_t hsync_back_porch;
    uint16_t hsync_pulse_width;
    uint16_t hsync_front_porch;
    uint16_t vsync_back_porch;
    uint16_t vsync_pulse_width;
    uint16_t vsync_front_porch;
    const LcdInitCommand *commands;
    size_t command_count;
};

constexpr PanelConfig kPanelConfigs[] = {
    {
        DisplayVariant::LCD_7,
        720,
        1280,
        1000,
        80,
        239,
        50,
        33,
        20,
        30,
        2,
        kIli9881cLcd7Commands,
        sizeof(kIli9881cLcd7Commands) / sizeof(kIli9881cLcd7Commands[0]),
    },
    {
        DisplayVariant::LCD_8,
        800,
        1280,
        1500,
        80,
        20,
        20,
        40,
        12,
        4,
        30,
        kJd9365Lcd8Commands,
        sizeof(kJd9365Lcd8Commands) / sizeof(kJd9365Lcd8Commands[0]),
    },
    {
        DisplayVariant::LCD_10_1,
        800,
        1280,
        1500,
        80,
        20,
        20,
        40,
        12,
        4,
        30,
        kJd9365Lcd10Commands,
        sizeof(kJd9365Lcd10Commands) / sizeof(kJd9365Lcd10Commands[0]),
    },
};

i2c_master_bus_handle_t g_i2c_bus = nullptr;
esp_ldo_channel_handle_t g_dsi_phy_power = nullptr;

const PanelConfig *find_panel_config(DisplayVariant variant)
{
    for (const PanelConfig &config : kPanelConfigs) {
        if (config.variant == variant) {
            return &config;
        }
    }
    return nullptr;
}

bool check_esp(esp_err_t error, const char *step)
{
    if (error == ESP_OK) {
        return true;
    }
    Serial.printf("LCD-X %s failed: %s (0x%X)\n",
                  step, esp_err_to_name(error), static_cast<unsigned>(error));
    return false;
}

uint64_t integer_square_root(uint64_t value)
{
    uint64_t result = 0;
    uint64_t bit = UINT64_C(1) << 62;
    while (bit > value) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

i2c_master_bus_handle_t shared_i2c_bus()
{
    if (g_i2c_bus != nullptr) {
        return g_i2c_bus;
    }

    if (i2c_master_get_bus_handle(kI2cPort, &g_i2c_bus) == ESP_OK) {
        return g_i2c_bus;
    }

    i2c_master_bus_config_t config = {};
    config.i2c_port = kI2cPort;
    config.sda_io_num = kI2cSda;
    config.scl_io_num = kI2cScl;
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = 0;
    if (!check_esp(i2c_new_master_bus(&config, &g_i2c_bus), "I2C bus initialization")) {
        g_i2c_bus = nullptr;
    }
    return g_i2c_bus;
}

void configure_display_bridge_best_effort()
{
    i2c_master_bus_handle_t bus = shared_i2c_bus();
    if (bus == nullptr) {
        Serial.println("LCD-X display bridge setup skipped: I2C unavailable");
        return;
    }

    i2c_device_config_t config = {};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = 0x45;
    config.scl_speed_hz = 100000;
    i2c_master_dev_handle_t device = nullptr;
    if (i2c_master_bus_add_device(bus, &config, &device) != ESP_OK) {
        Serial.println("LCD-X display bridge setup skipped: device registration failed");
        return;
    }

    // Mirror the best-effort setup used by the maintained ILI9881C driver.
    // That driver does not make a missing ACK fatal, so neither do we.
    const uint8_t writes[][2] = {
        {0x95, 0x11},
        {0x95, 0x17},
        {0x96, 0x00},
    };
    for (const auto &write : writes) {
        i2c_master_transmit(device, write, sizeof(write), 100);
    }
    delay(100);
    const uint8_t enable[] = {0x96, 0xFF};
    i2c_master_transmit(device, enable, sizeof(enable), 100);
    i2c_master_bus_rm_device(device);
    delay(1000);
}

bool send_command(esp_lcd_panel_io_handle_t io, const LcdInitCommand &command)
{
    if (!check_esp(esp_lcd_panel_io_tx_param(
                       io, command.command, command.data, command.data_size),
                   "panel command")) {
        return false;
    }
    if (command.delay_ms > 0) {
        delay(command.delay_ms);
    }
    return true;
}

bool send_panel_commands(esp_lcd_panel_io_handle_t io, const PanelConfig &config)
{
    if (config.variant == DisplayVariant::LCD_7) {
        const uint8_t page_one[] = {0x98, 0x81, 0x01};
        const uint8_t lane_count[] = {0x03};
        const uint8_t page_zero[] = {0x98, 0x81, 0x00};
        const uint8_t madctl[] = {0x00};
        const uint8_t colmod[] = {0x55};
        const LcdInitCommand prefix[] = {
            {0xFF, page_one, sizeof(page_one), 0},
            {0xB7, lane_count, sizeof(lane_count), 0},
            {0xFF, page_zero, sizeof(page_zero), 0},
            {LCD_CMD_SLPOUT, nullptr, 0, 120},
            {LCD_CMD_MADCTL, madctl, sizeof(madctl), 0},
            {LCD_CMD_COLMOD, colmod, sizeof(colmod), 0},
        };
        for (const LcdInitCommand &command : prefix) {
            if (!send_command(io, command)) {
                return false;
            }
        }
    } else {
        const uint8_t page_zero[] = {0x00};
        const uint8_t madctl[] = {0x00};
        const uint8_t colmod[] = {0x55};
        const uint8_t lane_count[] = {0x01};
        const LcdInitCommand prefix[] = {
            {0xE0, page_zero, sizeof(page_zero), 0},
            {LCD_CMD_MADCTL, madctl, sizeof(madctl), 0},
            {LCD_CMD_COLMOD, colmod, sizeof(colmod), 0},
            {0x80, lane_count, sizeof(lane_count), 0},
        };
        for (const LcdInitCommand &command : prefix) {
            if (!send_command(io, command)) {
                return false;
            }
        }
    }

    for (size_t index = 0; index < config.command_count; ++index) {
        if (!send_command(io, config.commands[index])) {
            return false;
        }
    }
    return true;
}

}  // namespace

bool Display::begin(DisplayVariant variant)
{
    if (frame_buffer_ != nullptr) {
        return variant == variant_;
    }

    const PanelConfig *config = find_panel_config(variant);
    if (config == nullptr) {
        Serial.println("LCD-X unknown display variant");
        return false;
    }
    if (!psramFound()) {
        Serial.println("LCD-X display requires PSRAM");
        return false;
    }

    bool acquired_dsi_phy_power = false;
    const auto release_partial_display = [&]() {
        if (panel_ != nullptr) {
            esp_lcd_panel_del(panel_);
            panel_ = nullptr;
        }
        if (panel_io_ != nullptr) {
            esp_lcd_panel_io_del(panel_io_);
            panel_io_ = nullptr;
        }
        if (dsi_bus_ != nullptr) {
            esp_lcd_del_dsi_bus(dsi_bus_);
            dsi_bus_ = nullptr;
        }
        if (acquired_dsi_phy_power && g_dsi_phy_power != nullptr) {
            esp_ldo_release_channel(g_dsi_phy_power);
            g_dsi_phy_power = nullptr;
        }
    };

    if (g_dsi_phy_power == nullptr) {
        esp_ldo_channel_config_t ldo_config = {};
        ldo_config.chan_id = 3;
        ldo_config.voltage_mv = 2500;
        if (!check_esp(esp_ldo_acquire_channel(&ldo_config, &g_dsi_phy_power),
                       "MIPI DSI PHY power")) {
            return false;
        }
        acquired_dsi_phy_power = true;
    }

    esp_lcd_dsi_bus_config_t bus_config = {};
    bus_config.bus_id = 0;
    bus_config.num_data_lanes = 2;
#if defined(CONFIG_ESP32P4_SELECTS_REV_LESS_V3)
    bus_config.phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT;
#else
    bus_config.phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT;
#endif
    bus_config.lane_bit_rate_mbps = config->lane_bit_rate_mbps;
    if (!check_esp(esp_lcd_new_dsi_bus(&bus_config, &dsi_bus_), "MIPI DSI bus")) {
        release_partial_display();
        return false;
    }

    esp_lcd_dbi_io_config_t io_config = {};
    io_config.virtual_channel = 0;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    if (!check_esp(esp_lcd_new_panel_io_dbi(dsi_bus_, &io_config, &panel_io_),
                   "MIPI DBI command channel")) {
        release_partial_display();
        return false;
    }

    if (variant == DisplayVariant::LCD_7) {
        configure_display_bridge_best_effort();
    }

    esp_lcd_dpi_panel_config_t dpi_config = {};
    dpi_config.virtual_channel = 0;
    dpi_config.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
    dpi_config.dpi_clock_freq_mhz = config->dpi_clock_mhz;
    dpi_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
    dpi_config.num_fbs = 1;
    dpi_config.video_timing.h_size = config->width;
    dpi_config.video_timing.v_size = config->height;
    dpi_config.video_timing.hsync_back_porch = config->hsync_back_porch;
    dpi_config.video_timing.hsync_pulse_width = config->hsync_pulse_width;
    dpi_config.video_timing.hsync_front_porch = config->hsync_front_porch;
    dpi_config.video_timing.vsync_back_porch = config->vsync_back_porch;
    dpi_config.video_timing.vsync_pulse_width = config->vsync_pulse_width;
    dpi_config.video_timing.vsync_front_porch = config->vsync_front_porch;
    dpi_config.flags.use_dma2d = 1;
    if (!check_esp(esp_lcd_new_panel_dpi(dsi_bus_, &dpi_config, &panel_),
                   "MIPI DPI panel")) {
        release_partial_display();
        return false;
    }

    pinMode(static_cast<uint8_t>(kLcdReset), OUTPUT);
    digitalWrite(static_cast<uint8_t>(kLcdReset), HIGH);
    delay(5);
    digitalWrite(static_cast<uint8_t>(kLcdReset), LOW);
    delay(10);
    digitalWrite(static_cast<uint8_t>(kLcdReset), HIGH);
    delay(120);

    if (!send_panel_commands(panel_io_, *config)) {
        release_partial_display();
        return false;
    }
    if (!check_esp(esp_lcd_panel_init(panel_), "MIPI DPI start")) {
        release_partial_display();
        return false;
    }

    void *frame_buffer = nullptr;
    if (!check_esp(esp_lcd_dpi_panel_get_frame_buffer(panel_, 1, &frame_buffer),
                   "frame buffer lookup")) {
        release_partial_display();
        return false;
    }

    if (!ledcAttach(kBacklightPin, kBacklightFrequencyHz,
                    kBacklightResolutionBits)) {
        Serial.println("LCD-X backlight PWM initialization failed");
        release_partial_display();
        return false;
    }

    if (!ledcWrite(kBacklightPin, (1U << kBacklightResolutionBits) - 1U)) {
        Serial.println("LCD-X backlight PWM write failed");
        ledcDetach(kBacklightPin);
        release_partial_display();
        return false;
    }

    frame_buffer_ = static_cast<uint16_t *>(frame_buffer);
    variant_ = variant;
    width_ = config->width;
    height_ = config->height;
    fill_screen(BLACK);
    return true;
}

void Display::fill_screen(uint16_t color)
{
    if (frame_buffer_ == nullptr) {
        return;
    }
    std::fill_n(frame_buffer_, static_cast<size_t>(width_) * height_, color);
    check_esp(esp_cache_msync(
                  frame_buffer_, static_cast<size_t>(width_) * height_ * sizeof(uint16_t),
                  ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED),
              "frame buffer sync");
}

void Display::fill_rect(int x, int y, int width, int height, uint16_t color)
{
    if (frame_buffer_ == nullptr || width <= 0 || height <= 0) {
        return;
    }

    const int64_t right = static_cast<int64_t>(x) + width;
    const int64_t bottom = static_cast<int64_t>(y) + height;
    const int x0 = static_cast<int>(std::max<int64_t>(0, x));
    const int y0 = static_cast<int>(std::max<int64_t>(0, y));
    const int x1 = static_cast<int>(std::min<int64_t>(width_, right));
    const int y1 = static_cast<int>(std::min<int64_t>(height_, bottom));
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    for (int row = y0; row < y1; ++row) {
        uint16_t *start = frame_buffer_ + static_cast<size_t>(row) * width_ + x0;
        std::fill(start, start + (x1 - x0), color);
    }
    sync_rect(x0, y0, x1 - x0, y1 - y0);
}

void Display::fill_circle(int x, int y, int radius, uint16_t color)
{
    if (frame_buffer_ == nullptr) {
        return;
    }
    if (radius <= 0) {
        fill_rect(x, y, 1, 1, color);
        return;
    }

    const int64_t radius_64 = radius;
    const int64_t center_x = x;
    const int64_t center_y = y;
    const int64_t first_row = std::max<int64_t>(0, center_y - radius_64);
    const int64_t last_row = std::min<int64_t>(height_ - 1, center_y + radius_64);
    if (first_row > last_row) {
        return;
    }

    const int64_t radius_squared = radius_64 * radius_64;
    for (int64_t row = first_row; row <= last_row; ++row) {
        const int64_t offset_y = row - center_y;
        const int64_t horizontal_squared = radius_squared - offset_y * offset_y;
        const int64_t extent = static_cast<int64_t>(
            integer_square_root(static_cast<uint64_t>(horizontal_squared)));

        const int64_t first_column = std::max<int64_t>(0, center_x - extent);
        const int64_t last_column = std::min<int64_t>(width_ - 1, center_x + extent);
        if (first_column <= last_column) {
            fill_rect(static_cast<int>(first_column), static_cast<int>(row),
                      static_cast<int>(last_column - first_column + 1), 1, color);
        }
    }
}

void Display::sync_rect(int x, int y, int width, int height)
{
    for (int row = y; row < y + height; ++row) {
        void *start = frame_buffer_ + static_cast<size_t>(row) * width_ + x;
        check_esp(esp_cache_msync(
                      start, static_cast<size_t>(width) * sizeof(uint16_t),
                      ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED),
                  "frame buffer sync");
    }
}

bool Touch::begin(uint16_t width, uint16_t height)
{
    if (device_ != nullptr) {
        return true;
    }

    i2c_master_bus_handle_t bus = shared_i2c_bus();
    if (bus == nullptr) {
        return false;
    }

    // INT and RST intentionally remain GPIO_NUM_NC. Without an address-select
    // reset sequence, probe both addresses and bind the driver to the one that
    // actually acknowledges.
    static_assert(RESET_GPIO == GPIO_NUM_NC, "touch RST must remain unassigned");
    static_assert(INTERRUPT_GPIO == GPIO_NUM_NC, "touch INT must remain unassigned");
    delay(200);
    if (i2c_master_probe(bus, kGt911Address, 100) == ESP_OK) {
        address_ = kGt911Address;
    } else if (i2c_master_probe(bus, kGt911BackupAddress, 100) == ESP_OK) {
        address_ = kGt911BackupAddress;
    } else {
        Serial.println("GT911-compatible touch not found at 0x5D or 0x14");
        return false;
    }

    i2c_device_config_t config = {};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = address_;
    config.scl_speed_hz = kI2cFrequencyHz;
    if (!check_esp(i2c_master_bus_add_device(bus, &config, &device_),
                   "touch device registration")) {
        device_ = nullptr;
        address_ = 0;
        return false;
    }

    width_ = width;
    height_ = height;
    uint8_t id[4] = {};
    if (read_register(kGt911ProductIdRegister, id, sizeof(id))) {
        for (size_t index = 0; index < sizeof(id); ++index) {
            product_id_[index] = id[index] >= 0x20 && id[index] <= 0x7E
                                     ? static_cast<char>(id[index])
                                     : '?';
        }
        product_id_[4] = '\0';
    } else {
        memcpy(product_id_, "????", sizeof(product_id_));
    }

    Serial.printf("GT911-compatible touch found at 0x%02X; INT/RST unused\n", address_);
    return true;
}

bool Touch::read(TouchPoint *points, size_t capacity, size_t *count)
{
    if (count != nullptr) {
        *count = 0;
    }
    if (device_ == nullptr || points == nullptr || count == nullptr || capacity == 0) {
        return false;
    }

    uint8_t status = 0;
    if (!read_register(kGt911TouchStatusRegister, &status, 1)) {
        return false;
    }
    if ((status & 0x80U) == 0) {
        write_register(kGt911TouchStatusRegister, 0);
        return false;
    }

    const uint8_t reported = status & 0x0FU;
    if (reported == 0 || reported > MAX_POINTS) {
        write_register(kGt911TouchStatusRegister, 0);
        return false;
    }

    uint8_t raw[MAX_POINTS * 8] = {};
    if (!read_register(kGt911TouchStatusRegister + 1, raw,
                       static_cast<size_t>(reported) * 8U)) {
        write_register(kGt911TouchStatusRegister, 0);
        return false;
    }
    if (!write_register(kGt911TouchStatusRegister, 0)) {
        return false;
    }

    const size_t requested = std::min(static_cast<size_t>(reported), capacity);
    size_t accepted = 0;
    for (size_t index = 0; index < requested; ++index) {
        const size_t base = index * 8U;
        TouchPoint point = {
            static_cast<uint16_t>(raw[base + 1] | (raw[base + 2] << 8U)),
            static_cast<uint16_t>(raw[base + 3] | (raw[base + 4] << 8U)),
            static_cast<uint16_t>(raw[base + 5] | (raw[base + 6] << 8U)),
            raw[base],
        };
        if (point.x < width_ && point.y < height_) {
            points[accepted++] = point;
        }
    }
    *count = accepted;
    return accepted > 0;
}

bool Touch::read_register(uint16_t reg, uint8_t *data, size_t size)
{
    const uint8_t command[] = {
        static_cast<uint8_t>(reg >> 8U),
        static_cast<uint8_t>(reg & 0xFFU),
    };
    return check_esp(i2c_master_transmit_receive(
                         device_, command, sizeof(command), data, size, 100),
                     "touch register read");
}

bool Touch::write_register(uint16_t reg, uint8_t value)
{
    const uint8_t command[] = {
        static_cast<uint8_t>(reg >> 8U),
        static_cast<uint8_t>(reg & 0xFFU),
        value,
    };
    return check_esp(i2c_master_transmit(device_, command, sizeof(command), 100),
                     "touch register write");
}

}  // namespace lcd_x
