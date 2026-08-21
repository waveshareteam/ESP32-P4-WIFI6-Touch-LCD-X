/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(BOARD_HAS_PSRAM)
#error "This example requires PSRAM. Enable PSRAM in the Arduino IDE Tools menu."
#endif

#include <WiFi.h>
#include <lcd_x_board.h>

lcd_x::Display display;

namespace {

constexpr int kFirstChannel = 1;
constexpr int kLastChannel = 14;
constexpr int kPreferredLastChannel = 11;
constexpr int kRssiFloor = -100;
constexpr int kRssiCeiling = -40;
constexpr uint32_t kScanIntervalMs = 3000;

constexpr uint16_t kChannelColors[kLastChannel] = {
    lcd_x::RED, lcd_x::YELLOW, lcd_x::GREEN, lcd_x::CYAN, lcd_x::BLUE, lcd_x::MAGENTA,
    lcd_x::RED, lcd_x::YELLOW, lcd_x::GREEN, lcd_x::CYAN, lcd_x::BLUE, lcd_x::MAGENTA,
    lcd_x::RED, lcd_x::YELLOW,
};

int graph_top = 0;
int graph_baseline = 0;
int channel_width = 0;
uint32_t next_scan_ms = 0;

void halt_with_error(const char *message)
{
    Serial.println(message);
    while (true) {
        delay(1000);
    }
}

int channel_center(int channel)
{
    return (channel - kFirstChannel) * channel_width + channel_width / 2;
}

int rssi_height(int rssi)
{
    const int graph_height = graph_baseline - graph_top;
    const int bounded_rssi = constrain(rssi, kRssiFloor, kRssiCeiling);
    return map(bounded_rssi, kRssiFloor, kRssiCeiling, 2, graph_height);
}

void draw_header()
{
    display.fill_rect(0, 0, display.width(), graph_top, lcd_x::BLACK);
    display.draw_text(12, 12, "ESP32-P4 Wi-Fi Analyzer", lcd_x::WHITE, lcd_x::BLACK, 2);
}

void draw_network(int channel, int rssi, uint16_t color)
{
    const int center = channel_center(channel);
    const int height = rssi_height(rssi);
    const int half_width = channel_width * 2;
    const int top = graph_baseline - height;

    display.draw_line(center - half_width, graph_baseline, center, top, color);
    display.draw_line(center, top, center + half_width, graph_baseline, color);
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-P4-WIFI6-Touch-LCD-X Wi-Fi Analyzer");

    if (!display.begin()) {
        halt_with_error("Display initialization failed");
    }
    if (display.width() < 280 || display.height() < 160) {
        halt_with_error("Display is too small for the Wi-Fi analyzer");
    }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    graph_top = 60;
    graph_baseline = display.height() - 48;
    channel_width = display.width() / kLastChannel;
    if (channel_width < 4 || graph_baseline <= graph_top) {
        halt_with_error("Invalid Wi-Fi analyzer layout");
    }

    draw_header();
}

void loop()
{
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - next_scan_ms) < 0) {
        delay(20);
        return;
    }
    next_scan_ms = now + kScanIntervalMs;

    int access_point_count[kLastChannel] = {};
    int strongest_rssi[kLastChannel];
    int strongest_index[kLastChannel];
    int noise[kPreferredLastChannel] = {};
    for (int index = 0; index < kLastChannel; ++index) {
        strongest_rssi[index] = kRssiFloor;
        strongest_index[index] = -1;
    }

    const int found = WiFi.scanNetworks(false, true);
    draw_header();
    display.fill_rect(0, graph_top, display.width(), display.height() - graph_top, lcd_x::BLACK);

    int valid_networks = 0;
    for (int index = 0; index < found; ++index) {
        const int channel = WiFi.channel(index);
        if (channel < kFirstChannel || channel > kLastChannel) {
            continue;
        }

        const int channel_index = channel - kFirstChannel;
        const int rssi = WiFi.RSSI(index);
        ++valid_networks;
        ++access_point_count[channel_index];
        if (rssi > strongest_rssi[channel_index]) {
            strongest_rssi[channel_index] = rssi;
            strongest_index[channel_index] = index;
        }

        const int bounded_rssi = constrain(rssi, kRssiFloor, kRssiCeiling);
        const int signal_above_floor = bounded_rssi - kRssiFloor;
        const int energy = signal_above_floor * signal_above_floor;
        for (int preferred_channel = kFirstChannel; preferred_channel <= kPreferredLastChannel;
             ++preferred_channel) {
            if (abs(channel - preferred_channel) <= 4) {
                noise[preferred_channel - kFirstChannel] += energy;
            }
        }

        draw_network(channel, rssi, kChannelColors[channel_index]);
    }

    display.draw_line(0, graph_baseline, display.width() - 1, graph_baseline, lcd_x::WHITE);
    for (int channel = kFirstChannel; channel <= kLastChannel; ++channel) {
        const int channel_index = channel - kFirstChannel;
        const int x = channel_center(channel);
        display.draw_text(x - 5, graph_baseline + 6, String(channel).c_str(),
                          kChannelColors[channel_index], lcd_x::BLACK);
        if (access_point_count[channel_index] > 0) {
            display.draw_text(x - 8, graph_baseline + 20,
                              String(access_point_count[channel_index]).c_str(),
                              lcd_x::WHITE, lcd_x::BLACK);
        }
        if (strongest_index[channel_index] >= 0) {
            String label = WiFi.SSID(strongest_index[channel_index]);
            if (label.length() == 0) {
                label = WiFi.BSSIDstr(strongest_index[channel_index]);
            }
            if (label.length() > 16) {
                label = label.substring(0, 16);
            }
            display.draw_text(max(0, x - 32), graph_baseline - rssi_height(strongest_rssi[channel_index]) - 14,
                              label.c_str(), kChannelColors[channel_index], lcd_x::BLACK);
        }
    }

    int quietest_channel = kFirstChannel;
    for (int channel = kFirstChannel + 1; channel <= kPreferredLastChannel; ++channel) {
        if (noise[channel - kFirstChannel] < noise[quietest_channel - kFirstChannel]) {
            quietest_channel = channel;
        }
    }
    String status = String(valid_networks) + " networks, quietest: ch " + quietest_channel;
    display.draw_text(12, 36, status.c_str(), lcd_x::GREEN, lcd_x::BLACK);
    Serial.println(status);

    WiFi.scanDelete();
}
