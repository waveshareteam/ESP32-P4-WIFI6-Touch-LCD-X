/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "MusicPlayer.hpp"
#include <cstdlib>
#include "lvgl.h"
#include "esp_brookesia.hpp"
#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:MusicPlayer"
#include "esp_lib_utils.h"
#include "sdkconfig.h"
#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"
#include "gui_music/lv_demo_music.h"
#include "gui_music/lv_demo_music_main.h"

#define MUSIC_DIR BSP_SPIFFS_MOUNT_POINT "/music"

LV_IMG_DECLARE(img_app_musicplayer);

static const char *TAG = "MusicPlayer";

static void destroy_file_iterator(file_iterator_instance_t **iterator)
{
    if (!iterator || !*iterator) {
        return;
    }
    file_iterator_instance_t *instance = *iterator;
    if (instance->list) {
        for (size_t i = 0; i < instance->count; ++i) {
            free(instance->list[i]);
        }
        free(instance->list);
    }
    free(const_cast<char *>(instance->directory_path));
    free(instance);
    *iterator = nullptr;
}

namespace esp_brookesia::apps
{

    MusicPlayer *MusicPlayer::_instance = nullptr;

    MusicPlayer *MusicPlayer::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new MusicPlayer(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    MusicPlayer::MusicPlayer(bool use_status_bar, bool use_navigation_bar) : App("MusicPlayer", &img_app_musicplayer, true, use_status_bar, use_navigation_bar),
                                                                             _file_iterator(NULL)
    {
    }

    MusicPlayer::~MusicPlayer()
    {
        close();
    }

    bool MusicPlayer::run(void)
    {
        ESP_UTILS_LOGD("Run");

        if (_player_active || _ui_active || _file_iterator) {
            (void)close();
        }

        esp_err_t ret = bsp_extra_player_init();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Audio player initialization failed: %s", esp_err_to_name(ret));
            return false;
        }
        _player_active = true;

        ret = bsp_extra_file_instance_init(MUSIC_DIR, &_file_iterator);
        if (ret != ESP_OK || !_file_iterator)
        {
            ESP_LOGE(TAG, "Scan music directory failed: %s", esp_err_to_name(ret));
            (void)bsp_extra_player_del();
            _player_active = false;
            _file_iterator = nullptr;
            return false;
        }

        ESP_LOGI(
            TAG,
            "Loaded %u music track(s) from %s",
            static_cast<unsigned>(file_iterator_get_count(_file_iterator)),
            MUSIC_DIR
        );
        lv_demo_music(lv_scr_act(), _file_iterator);
        _ui_active = true;
        return true;
    }

    bool MusicPlayer::back(void)
    {
        ESP_UTILS_LOGD("Back");
        // If the app needs to exit, call notifyCoreClosed() to notify the core to close the app
        ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
        return true;
    }

    bool MusicPlayer::close(void)
    {
        ESP_UTILS_LOGD("Close");
        bool ok = true;

        if (_ui_active) {
            lv_demo_music_close();
            _ui_active = false;
        }

        if (_player_active) {
            esp_err_t pause_ret = audio_player_pause();
            if (pause_ret != ESP_OK && pause_ret != ESP_ERR_INVALID_STATE) {
                ESP_LOGW(TAG, "Pause audio player failed: %s", esp_err_to_name(pause_ret));
                ok = false;
            }
            esp_err_t delete_ret = bsp_extra_player_del();
            if (delete_ret != ESP_OK) {
                ESP_LOGE(TAG, "Delete audio player failed: %s", esp_err_to_name(delete_ret));
                ok = false;
            } else {
                _player_active = false;
            }
        }

        /* Keep the iterator alive if the audio task could not be stopped; a
         * later close attempt can then finish without leaving stale state. */
        if (_file_iterator && !_player_active) {
            destroy_file_iterator(&_file_iterator);
        }
        return ok;
    }

    bool MusicPlayer::init()
    {
        ESP_UTILS_LOGD("Init");
        return true;
    }

    bool MusicPlayer::deinit()
    {
        ESP_UTILS_LOGD("Deinit");
        return close();
    }

    bool MusicPlayer::pause()
    {
        ESP_UTILS_LOGD("Pause UI; keep audio playback state");
        // Brookesia calls pause() when another screen becomes active. Audio is
        // intentionally left untouched so playback can continue in background.
        return true;
    }

    bool MusicPlayer::resume()
    {
        ESP_UTILS_LOGD("Resume UI; keep audio playback state");
        // The UI and audio player remain alive while the app is backgrounded.
        // Avoid restarting audio here, especially when the user paused it.
        return true;
    }

} // namespace esp_brookesia::apps
