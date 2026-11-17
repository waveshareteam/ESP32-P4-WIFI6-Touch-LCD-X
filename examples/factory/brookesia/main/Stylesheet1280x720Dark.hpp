#pragma once

#include "esp_brookesia.hpp"

namespace esp_brookesia::systems::phone {

// 1280x720 landscape stylesheet for the 7 inch panel at rotation 90/270. It keeps the
// 720x1280 dark style and overrides only the sizes that do not fit a 720-pixel-high UI.
inline const Stylesheet STYLESHEET_1280_720_DARK = [] {
    Stylesheet stylesheet = ESP_BROOKESIA_PHONE_720_1280_DARK_STYLESHEET();

    stylesheet.core.name = "1280x720 Dark";
    stylesheet.core.screen_size = gui::StyleSize::RECT(1280, 720);

    // 720 minus the 60-pixel status bar and the 80 pixels of page indicator.
    stylesheet.display.app_launcher.data.table.size = gui::StyleSize::RECT_W_PERCENT(100, 580);

    // Four columns and two rows per page. The 300x280 cell keeps the 112-pixel icon unscaled.
    auto &icon = stylesheet.display.app_launcher.data.icon;
    icon.main.size = gui::StyleSize::RECT(300, 280);
    icon.image.default_size = gui::StyleSize::SQUARE_PERCENT(40);
    icon.image.press_size = gui::StyleSize::SQUARE_PERCENT(36);

    // The snapshot table is 508 pixels high: 720 minus the 28-pixel memory label, 64-pixel
    // trash icon, four 20-pixel row gaps and 40-pixel bottom padding. The image keeps the
    // 16:9 screen aspect ratio below a 60-pixel title.
    auto &snapshot = stylesheet.display.recents_screen.data.snapshot_table.snapshot;
    snapshot.main_size = gui::StyleSize::RECT(768, 492);
    snapshot.title.main_size = gui::StyleSize::RECT(768, 60);
    snapshot.image.main_size = gui::StyleSize::RECT(768, 432);

    return stylesheet;
}();

} // namespace esp_brookesia::systems::phone
