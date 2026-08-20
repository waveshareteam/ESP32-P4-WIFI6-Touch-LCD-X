/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <atomic>
#include <new>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp/esp-bsp.h"
#include "esp_brookesia.hpp"
#include "boost/thread.hpp"
#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "Main"
#include "esp_lib_utils.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "bsp_board_extra.h"

#include "Drawpanel.hpp"
#include "SpecAnalyzer.hpp"
#include "MusicPlayer.hpp"
#include "Settings.hpp"
#include "Camera.hpp"
#include "VideoPlayer.hpp"
#include "esp_brookesia_app_calculator.hpp"
#include "XiaozhiApp.hpp"

using namespace esp_brookesia;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;

constexpr bool EXAMPLE_SHOW_MEM_INFO = false;

static volatile bool s_wifi_connected = false;

static void wifi_status_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_wifi_connected = true;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        s_wifi_connected = false;
    } else if (event_base == WIFI_EVENT &&
               (event_id == WIFI_EVENT_STA_DISCONNECTED || event_id == WIFI_EVENT_STA_STOP)) {
        s_wifi_connected = false;
    }
}

static void register_wifi_status_events()
{
    static bool registered = false;
    if (registered) {
        return;
    }

    esp_err_t ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_status_event_handler, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "Register Wi-Fi status handler failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_status_event_handler, nullptr);
    if (ret != ESP_OK) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "Register IP status handler failed: %s", esp_err_to_name(ret));
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_status_event_handler);
        return;
    }

    registered = true;
}

static bool is_wifi_connected()
{
    return s_wifi_connected;
}

namespace {

class BootLoadingUi {
public:
    static constexpr uint32_t STATUS_UPDATE_PERIOD_MS = 30;
    static constexpr uint32_t READY_HOLD_MS = 300;
    static constexpr uint32_t FADE_OUT_TIME_MS = 240;

    bool create()
    {
        root_ = lv_obj_create(lv_layer_top());
        if (root_ == nullptr) {
            return false;
        }

        spinner_ = lv_spinner_create(root_);
        title_ = lv_label_create(root_);
        phase_ = lv_label_create(root_);
        status_ = lv_label_create(root_);
        progress_ = lv_bar_create(root_);
        if (spinner_ == nullptr || title_ == nullptr || phase_ == nullptr ||
                status_ == nullptr || progress_ == nullptr) {
            destroy();
            return false;
        }

        lv_obj_remove_style_all(root_);
        lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
        lv_obj_align(root_, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(root_, lv_color_hex(0x101417), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(root_, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_set_size(spinner_, 64, 64);
        lv_obj_align(spinner_, LV_ALIGN_CENTER, 0, -110);
        lv_obj_set_style_arc_width(spinner_, 5, LV_PART_MAIN);
        lv_obj_set_style_arc_width(spinner_, 5, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(spinner_, lv_color_hex(0x313B42), LV_PART_MAIN);
        lv_obj_set_style_arc_color(spinner_, lv_color_hex(0x27C1A8), LV_PART_INDICATOR);

        lv_label_set_text(title_, "ESP32-P4");
        lv_obj_set_style_text_font(title_, &lv_font_montserrat_32, LV_PART_MAIN);
        lv_obj_set_style_text_color(title_, lv_color_hex(0xF7F9FA), LV_PART_MAIN);
        lv_obj_align(title_, LV_ALIGN_CENTER, 0, -36);

        lv_label_set_text(phase_, "LOADING");
        lv_obj_set_style_text_font(phase_, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(phase_, lv_color_hex(0xF2B84B), LV_PART_MAIN);
        lv_obj_align(phase_, LV_ALIGN_CENTER, 0, 4);

        lv_obj_set_width(status_, lv_pct(84));
        lv_label_set_long_mode(status_, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_align(status_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(status_, &lv_font_montserrat_18, LV_PART_MAIN);
        lv_obj_set_style_text_color(status_, lv_color_hex(0xA8B1B8), LV_PART_MAIN);
        lv_obj_align(status_, LV_ALIGN_CENTER, 0, 44);

        lv_obj_set_size(progress_, lv_pct(52), 7);
        lv_obj_align(progress_, LV_ALIGN_CENTER, 0, 82);
        lv_bar_set_range(progress_, 0, 100);
        lv_obj_set_style_radius(progress_, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_radius(progress_, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(progress_, lv_color_hex(0x2A343B), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(progress_, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(progress_, lv_color_hex(0x27C1A8), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(progress_, LV_OPA_COVER, LV_PART_INDICATOR);

        update_timer_ = lv_timer_create(onUpdateTimer, STATUS_UPDATE_PERIOD_MS, this);
        if (update_timer_ == nullptr) {
            destroy();
            return false;
        }

        setStageNow("Preparing display...", 8);
        return true;
    }

    void queueStage(const char *status, int progress)
    {
        if (status == nullptr) {
            return;
        }

        if (progress < 0) {
            progress = 0;
        } else if (progress > 100) {
            progress = 100;
        }

        pending_status_.store(status, std::memory_order_relaxed);
        pending_progress_.store(progress, std::memory_order_relaxed);
        pending_revision_.fetch_add(1, std::memory_order_release);
    }

    void showError(const char *status)
    {
        if (root_ == nullptr) {
            return;
        }

        stopUpdateTimer();
        lv_obj_add_flag(spinner_, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(phase_, "STARTUP FAILED");
        lv_obj_set_style_text_color(phase_, lv_color_hex(0xF06A6A), LV_PART_MAIN);
        lv_obj_align(phase_, LV_ALIGN_CENTER, 0, 4);
        lv_label_set_text(status_, status);
        lv_obj_set_style_text_color(status_, lv_color_hex(0xF7F9FA), LV_PART_MAIN);
        lv_bar_set_value(progress_, 100, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(progress_, lv_color_hex(0xF06A6A), LV_PART_INDICATOR);
        lv_obj_move_foreground(root_);
    }

    void finish()
    {
        if (root_ == nullptr) {
            return;
        }

        stopUpdateTimer();
        setStageNow("Ready", 100);

        lv_anim_t fade;
        lv_anim_init(&fade);
        lv_anim_set_var(&fade, root_);
        lv_anim_set_values(&fade, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_time(&fade, FADE_OUT_TIME_MS);
        lv_anim_set_delay(&fade, READY_HOLD_MS);
        lv_anim_set_exec_cb(&fade, [](void *object, int32_t opacity) {
            lv_obj_set_style_opa(static_cast<lv_obj_t *>(object), opacity, LV_PART_MAIN);
        });
        lv_anim_set_completed_cb(&fade, lv_obj_delete_anim_completed_cb);
        lv_anim_start(&fade);

        // The animation owns root_ now and deletes it from the LVGL task.
        clearObjectPointers();
    }

    void attachToTopLayer(lv_display_t *display)
    {
        attachToLayer(display != nullptr ? lv_display_get_layer_top(display) : nullptr);
    }

    void attachToSystemLayer(lv_display_t *display)
    {
        attachToLayer(display != nullptr ? lv_display_get_layer_sys(display) : nullptr);
    }

    void destroy()
    {
        stopUpdateTimer();
        if (root_ != nullptr) {
            lv_obj_delete(root_);
        }
        clearObjectPointers();
    }

private:
    static void onUpdateTimer(lv_timer_t *timer)
    {
        auto *self = static_cast<BootLoadingUi *>(lv_timer_get_user_data(timer));
        if (self != nullptr) {
            self->applyPendingStage();
        }
    }

    void applyPendingStage()
    {
        const uint32_t revision = pending_revision_.load(std::memory_order_acquire);
        if (revision == applied_revision_) {
            return;
        }

        const char *status = pending_status_.load(std::memory_order_relaxed);
        const int progress = pending_progress_.load(std::memory_order_relaxed);
        if (status != nullptr) {
            setStageNow(status, progress);
        }
        applied_revision_ = revision;
    }

    void setStageNow(const char *status, int progress)
    {
        if (root_ == nullptr) {
            return;
        }

        lv_label_set_text(status_, status);
        lv_bar_set_value(progress_, progress, LV_ANIM_OFF);
        lv_obj_move_foreground(root_);
    }

    void stopUpdateTimer()
    {
        if (update_timer_ != nullptr) {
            lv_timer_delete(update_timer_);
            update_timer_ = nullptr;
        }
    }

    void clearObjectPointers()
    {
        root_ = nullptr;
        spinner_ = nullptr;
        title_ = nullptr;
        phase_ = nullptr;
        status_ = nullptr;
        progress_ = nullptr;
    }

    void attachToLayer(lv_obj_t *parent)
    {
        if (root_ == nullptr || parent == nullptr) {
            return;
        }

        if (lv_obj_get_parent(root_) != parent) {
            lv_obj_set_parent(root_, parent);
            lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
            lv_obj_align(root_, LV_ALIGN_CENTER, 0, 0);
        }
        lv_obj_move_foreground(root_);
    }

    lv_obj_t *root_ = nullptr;
    lv_obj_t *spinner_ = nullptr;
    lv_obj_t *title_ = nullptr;
    lv_obj_t *phase_ = nullptr;
    lv_obj_t *status_ = nullptr;
    lv_obj_t *progress_ = nullptr;
    lv_timer_t *update_timer_ = nullptr;
    std::atomic<const char *> pending_status_{nullptr};
    std::atomic<int> pending_progress_{0};
    std::atomic<uint32_t> pending_revision_{0};
    uint32_t applied_revision_ = 0;
};

static void refresh_boot_loading(BootLoadingUi &loading, const char *status, int progress)
{
    ESP_LOGI(ESP_UTILS_LOG_TAG, "Startup stage begin: %s (%d%%)", status, progress);
    loading.queueStage(status, progress);
    ESP_LOGI(ESP_UTILS_LOG_TAG, "Startup stage queued: %s (%d%%)", status, progress);
}

class BootLoadingFailureGuard {
public:
    BootLoadingFailureGuard(BootLoadingUi &loading, lv_display_t *display)
        : loading_(loading), display_(display)
    {
    }

    ~BootLoadingFailureGuard()
    {
        if (!armed_) {
            return;
        }

        LvLockGuard gui_guard;
        loading_.attachToSystemLayer(display_);
        loading_.showError("Unable to finish startup. Please restart the device.");
    }

    void dismiss()
    {
        armed_ = false;
    }

private:
    BootLoadingUi &loading_;
    lv_display_t *display_;
    bool armed_ = true;
};

class PhoneStartupOwner {
public:
    PhoneStartupOwner(Phone *phone, BootLoadingUi &loading, lv_display_t *display)
        : phone_(phone), loading_(loading), display_(display)
    {
    }

    ~PhoneStartupOwner()
    {
        reset();
    }

    PhoneStartupOwner(const PhoneStartupOwner &) = delete;
    PhoneStartupOwner &operator=(const PhoneStartupOwner &) = delete;

    Phone *get() const
    {
        return phone_;
    }

    Phone *release()
    {
        Phone *phone = phone_;
        phone_ = nullptr;
        return phone;
    }

private:
    void reset()
    {
        if (phone_ == nullptr) {
            return;
        }

        if (!bsp_extra_display_lock(-1)) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Lock display while cleaning up phone failed");
            return;
        }
        loading_.attachToTopLayer(display_);
        delete phone_;
        phone_ = nullptr;
        bsp_extra_display_unlock();
    }

    Phone *phone_;
    BootLoadingUi &loading_;
    lv_display_t *display_;
};

} // namespace

extern "C" void app_main(void)
{
    esp_log_level_set("rpc_rsp", ESP_LOG_ERROR);

    ESP_UTILS_LOGI("Reset reason: %d", static_cast<int>(esp_reset_reason()));
    ESP_UTILS_LOGI("Display ESP-Brookesia phone demo");

    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0}};

    // Keep LVGL on core 1 with a larger PSRAM-backed stack; high-resolution UI and
    // app switching can otherwise starve the GUI task or overflow the default stack.
    cfg.lv_adapter_cfg.task_stack_size = 16 * 1024;
    cfg.lv_adapter_cfg.task_priority = 10;
    cfg.lv_adapter_cfg.task_core_id = 1;
    cfg.lv_adapter_cfg.stack_in_psram = true;

    /* Configure display */
    lv_display_t *disp = bsp_display_start_with_config(&cfg);
    ESP_UTILS_CHECK_NULL_EXIT(disp, "Start display failed");
    bsp_display_backlight_off();

    /* Configure GUI lock */
    LvLock::registerCallbacks([](int timeout_ms) {
        bool locked = bsp_extra_display_lock(timeout_ms);
        ESP_UTILS_CHECK_FALSE_RETURN(locked, false, "Lock failed (timeout_ms: %d)", timeout_ms);

        return true;
    }, []() {
        bsp_extra_display_unlock();
        return true;
    });

    BootLoadingUi boot_loading;
    ESP_LOGI(ESP_UTILS_LOG_TAG, "Startup stage begin: Preparing display... (8%%)");
    {
        LvLockGuard gui_guard;
        if (!boot_loading.create()) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Create boot loading UI failed");
        }
    }
    ESP_LOGI(ESP_UTILS_LOG_TAG, "Startup stage ready: Preparing display... (8%%)");
    // Let the LVGL worker render the first frame before enabling the backlight.
    vTaskDelay(pdMS_TO_TICKS(50));
    bsp_display_backlight_on();

    BootLoadingFailureGuard boot_failure_guard(boot_loading, disp);

    refresh_boot_loading(boot_loading, "Initializing settings...", 16);
    ESP_LOGI(ESP_UTILS_LOG_TAG, "NVS initialization begin");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(ESP_UTILS_LOG_TAG, "NVS requires erase before initialization: %s", esp_err_to_name(err));
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "NVS erase failed: %s", esp_err_to_name(err));
            return;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(ESP_UTILS_LOG_TAG, "NVS initialization failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(ESP_UTILS_LOG_TAG, "NVS initialization complete");

    refresh_boot_loading(boot_loading, "Mounting storage...", 20);
    err = bsp_spiffs_mount();
    if (err != ESP_OK) {
        ESP_LOGE(ESP_UTILS_LOG_TAG, "SPIFFS mount failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(ESP_UTILS_LOG_TAG, "SPIFFS mount successfully");

    refresh_boot_loading(boot_loading, "Starting audio...", 35);
    err = bsp_extra_codec_init();
    if (err != ESP_OK) {
        ESP_LOGE(ESP_UTILS_LOG_TAG, "Codec initialization failed: %s", esp_err_to_name(err));
        return;
    }

    refresh_boot_loading(boot_loading, "Configuring audio...", 45);
    err = bsp_extra_codec_volume_set(CODEC_DEFAULT_VOLUME, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(ESP_UTILS_LOG_TAG, "Codec volume setup failed: %s", esp_err_to_name(err));
        return;
    }

    /* Create a phone object */
    refresh_boot_loading(boot_loading, "Building home screen...", 55);
    PhoneStartupOwner phone_owner(new (std::nothrow) Phone(disp), boot_loading, disp);
    Phone *phone = phone_owner.get();
    ESP_UTILS_CHECK_NULL_EXIT(phone, "Create phone failed");

    {
        // When operating on non-GUI tasks, should acquire a lock before operating on LVGL
        LvLockGuard gui_guard;

        if (BSP_LCD_H_RES == 720 && BSP_LCD_V_RES == 1280) {
            const auto &target_stylesheet = ESP_BROOKESIA_PHONE_720_1280_DARK_STYLESHEET();
            ESP_UTILS_CHECK_FALSE_EXIT(phone->addStylesheet(target_stylesheet), "Add display stylesheet failed");
            ESP_UTILS_CHECK_FALSE_EXIT(phone->activateStylesheet(target_stylesheet), "Activate display stylesheet failed");
        } else if (BSP_LCD_H_RES == 800 && BSP_LCD_V_RES == 1280) {
            const auto &target_stylesheet = ESP_BROOKESIA_PHONE_800_1280_DARK_STYLESHEET();
            ESP_UTILS_CHECK_FALSE_EXIT(phone->addStylesheet(target_stylesheet), "Add display stylesheet failed");
            ESP_UTILS_CHECK_FALSE_EXIT(phone->activateStylesheet(target_stylesheet), "Activate display stylesheet failed");
        } else {
            ESP_UTILS_LOGE(ESP_UTILS_LOG_TAG, "No compatible display stylesheet");
            return;
        }

        /* Begin the phone */
        ESP_UTILS_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");
        boot_loading.attachToSystemLayer(disp);
        // assert(phone->getDisplay().showContainerBorder() && "Show container border failed");
    }

    /* Init and install apps from registry */
    refresh_boot_loading(boot_loading, "Loading application registry...", 65);
    {
        std::vector<systems::base::Manager::RegistryAppInfo> inited_apps;
        ESP_UTILS_CHECK_FALSE_EXIT(phone->initAppFromRegistry(inited_apps), "Init app registry failed");
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installAppFromRegistry(inited_apps), "Install app registry failed");
    }

    refresh_boot_loading(boot_loading, "Loading tools...", 74);
    {
        auto app2 = esp_brookesia::apps::Calculator::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app2), "Start Calculator failed");
        auto app = esp_brookesia::apps::Drawpanel::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app), "Start Drawpanel failed");

        auto app1 = esp_brookesia::apps::SpecAnalyzer::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app1), "Start SpecAnalyzer failed");
    }

    refresh_boot_loading(boot_loading, "Loading media...", 82);
    {
        auto app5 = esp_brookesia::apps::MusicPlayer::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app5), "Start MusicPlayer failed");
        auto camera_app = esp_brookesia::apps::Camera::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(camera_app), "Start Camera failed");
        auto video_player_app = esp_brookesia::apps::VideoPlayer::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(video_player_app), "Start VideoPlayer failed");
    }

    refresh_boot_loading(boot_loading, "Loading system applications...", 90);
    {
        auto settings_app = esp_brookesia::apps::Settings::requestInstance(false, false);
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(settings_app), "Start Settings failed");
        auto xiaozhi_app = esp_brookesia::apps::XiaozhiApp::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(xiaozhi_app), "Start Xiaozhi failed");
    }

    refresh_boot_loading(boot_loading, "Finishing setup...", 96);

    register_wifi_status_events();

    {
        LvLockGuard gui_guard;

        /* Create a timer to update the clock */
        lv_timer_t *status_timer = lv_timer_create([](lv_timer_t *t) {
            time_t now;
            struct tm timeinfo;
            Phone *phone = (Phone *)t->user_data;

            ESP_UTILS_CHECK_NULL_EXIT(phone, "Invalid phone");

            time(&now);
            localtime_r(&now, &timeinfo);

            ESP_UTILS_CHECK_FALSE_EXIT(
                phone->getDisplay().getStatusBar()->setClock(timeinfo.tm_hour, timeinfo.tm_min),
                "Refresh status bar failed"
            );

            bool connected = is_wifi_connected();
            StatusBar::WifiState wifi_state = connected ?
                StatusBar::WifiState::SIGNAL_3 :
                StatusBar::WifiState::DISCONNECTED;

            ESP_UTILS_CHECK_FALSE_EXIT(
                phone->getDisplay().getStatusBar()->setWifiIconState(wifi_state),
                "Refresh status bar failed"
            );
        }, 1000, phone);
        ESP_UTILS_CHECK_NULL_EXIT(status_timer, "Create status timer failed");

        phone_owner.release();
        boot_failure_guard.dismiss();
        // Keep the completed state visible briefly, then reveal the home screen smoothly.
        boot_loading.finish();
    }

    if constexpr (EXAMPLE_SHOW_MEM_INFO) {
        esp_utils::thread_config_guard thread_config({
            .name = "mem_info",
            .stack_size = 4096,
        });
        boost::thread([ = ]() {
            char buffer[128];    /* Make sure buffer is enough for `sprintf` */
            size_t internal_free = 0;
            size_t internal_total = 0;
            size_t external_free = 0;
            size_t external_total = 0;

            while (1) {
                internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
                internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
                external_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                external_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
                snprintf(
                    buffer,
                    sizeof(buffer),
                    "\t           Biggest /     Free /    Total\n"
                    "\t  SRAM : [%8zu / %8zu / %8zu]\n"
                    "\t PSRAM : [%8zu / %8zu / %8zu]",
                    heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), internal_free, internal_total,
                    heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), external_free, external_total
                );
                ESP_UTILS_LOGI("\n%s", buffer);

                {
                    LvLockGuard gui_guard;
                    ESP_UTILS_CHECK_FALSE_EXIT(
                        phone->getDisplay().getRecentsScreen()->setMemoryLabel(
                            internal_free / 1024, internal_total / 1024, external_free / 1024, external_total / 1024
                        ), "Set memory label failed"
                    );
                }

                boost::this_thread::sleep_for(boost::chrono::seconds(5));
            }
        }).detach();
    }
}
