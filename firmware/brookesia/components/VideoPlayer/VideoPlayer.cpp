/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "VideoPlayer.hpp"
#include "lvgl.h"
#include "esp_brookesia.hpp"

#include "bsp/touch.h"
#include "esp_lib_utils.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include <errno.h>
#include <sys/stat.h>
#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:VideoPlayer"

LV_IMG_DECLARE(img_app_vedioplayer);

#define DISPLAY_WIDTH  BSP_LCD_H_RES
#define DISPLAY_HEIGHT BSP_LCD_V_RES
#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB565
#define VIDEO_JPEG_PIXEL_FORMAT JPEG_PIXEL_FORMAT_RGB565_LE
#define VIDEO_PPA_COLOR_MODE    PPA_SRM_COLOR_MODE_RGB565
#define VIDEO_BYTES_PER_PIXEL   2
#elif CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
#define VIDEO_JPEG_PIXEL_FORMAT JPEG_PIXEL_FORMAT_RGB888
#define VIDEO_PPA_COLOR_MODE    PPA_SRM_COLOR_MODE_RGB888
#define VIDEO_BYTES_PER_PIXEL   3
#else
#error "Unsupported BSP LCD color format"
#endif

namespace esp_brookesia::apps
{

    VideoPlayer *VideoPlayer::_instance = nullptr;

    VideoPlayer *VideoPlayer::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new VideoPlayer(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    VideoPlayer::VideoPlayer(bool use_status_bar, bool use_navigation_bar) : App("VideoPlayer", &img_app_vedioplayer, true, use_status_bar, use_navigation_bar), sd_mounted(false)
    {
        display = nullptr;
        display_panel = nullptr;
        current_buf_idx = 0;
        ppa_srm_handle = nullptr;
        touch_handle = nullptr;
        jpeg_handle = nullptr;
        jpeg_output_buffer = nullptr;
        jpeg_output_buffer_size = 0;
        avi_player_handle = nullptr;
        play_task_handle = nullptr;
        close_wait_task = nullptr;
        is_playing = false;
        is_paused = false;
        loop_playback = false;
        dummy_enabled = false;
        lvgl_paused = false;
        touch_active = false;
        avi_file_list = nullptr;
        avi_file_count = 0;
        last_video_width = 0;
        last_video_height = 0;
    }

    VideoPlayer::~VideoPlayer()
    {
        close();
    }

    bool VideoPlayer::run(void)
    {
        ESP_UTILS_LOGD("Run");
        is_paused = false;
        if (!sd_mounted) {
            esp_err_t mount_ret = bsp_sdcard_mount();
            sd_mounted = mount_ret == ESP_OK || mount_ret == ESP_ERR_INVALID_STATE;
            if (!sd_mounted) {
                ESP_LOGE(
                    ESP_UTILS_LOG_TAG,
                    "Mount %s failed: %s",
                    BSP_SD_MOUNT_POINT,
                    esp_err_to_name(mount_ret)
                );
            }
        }
        bsp_extra_display_lock(-1);
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);
        status_label = lv_label_create(lv_scr_act());
        lv_label_set_text(status_label, sd_mounted ? "loading files..." : "sd error");
        lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(status_label, DISPLAY_WIDTH);
        lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);
        bsp_extra_display_unlock();

        if (!sd_mounted)
        {
            return true;
        }

        if (getAviFileList(BSP_SD_MOUNT_POINT "/avi") != ESP_OK || avi_file_count == 0)
        {
            bsp_extra_display_lock(-1);
            lv_label_set_text(status_label, "avi error");
            bsp_extra_display_unlock();
            return true;
        }

        bsp_extra_display_lock(-1);
        lv_obj_del(status_label);
        status_label = nullptr;
        bsp_extra_display_unlock();

        return startPlaybackTask();
    }

    bool VideoPlayer::back(void)
    {
        ESP_UTILS_LOGD("Back");
        // If the app needs to exit, call notifyCoreClosed() to notify the core to close the app
        ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
        return true;
    }

    bool VideoPlayer::close()
    {
        ESP_UTILS_LOGD("Close");
        if (!stopPlaybackTask(pdMS_TO_TICKS(2000))) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Playback task did not stop before close");
            return false;
        }

        deinitJpegDecoder();
        releaseJpegOutputBuffer();
        deinitDisplayBypass();

        bsp_extra_display_lock(-1);
        if (status_label)
        {
            lv_obj_del(status_label);
            status_label = nullptr;
        }
        bsp_extra_display_unlock();

        if (avi_file_list)
        {
            for (int i = 0; i < avi_file_count; i++)
            {
                free(avi_file_list[i]);
            }
            free(avi_file_list);
            avi_file_list = nullptr;
            avi_file_count = 0;
        }

        return true;
    }

    bool VideoPlayer::init()
    {
        ESP_UTILS_LOGD("Init");
        // Mount lazily in run() so inserting a card after boot is supported and
        // application installation does not initialize SDMMC unnecessarily.
        return true;
    }

    bool VideoPlayer::deinit()
    {
        ESP_UTILS_LOGD("Deinit");
        return true;
    }

    bool VideoPlayer::pause()
    {
        ESP_UTILS_LOGD("Pause");
        return stopPlaybackTask(pdMS_TO_TICKS(2000));
    }

    bool VideoPlayer::resume()
    {
        ESP_UTILS_LOGD("Resume");
        if (!sd_mounted || !avi_file_list || avi_file_count == 0) {
            return true;
        }
        return startPlaybackTask();
    }

    bool VideoPlayer::startPlaybackTask()
    {
        if (play_task_handle) {
            return true;
        }
        if (!avi_file_list || avi_file_count == 0) {
            return false;
        }

        loop_playback = true;
        is_paused = false;

        // AVI parsing, JPEG decoding, PPA blitting, and PCM output share this task.
        BaseType_t ret = xTaskCreatePinnedToCore(
            playTask,
            "avi_play_task",
            32768,
            this,
            7,
            &play_task_handle,
            0
        );
        if (ret != pdPASS) {
            loop_playback = false;
            play_task_handle = nullptr;
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Create playback task failed");
            return false;
        }

        return true;
    }

    bool VideoPlayer::stopPlaybackTask(TickType_t timeout)
    {
        loop_playback = false;
        is_paused = false;

        if (!play_task_handle) {
            return true;
        }
        if (xTaskGetCurrentTaskHandle() == play_task_handle) {
            return false;
        }

        close_wait_task = xTaskGetCurrentTaskHandle();
        if (!play_task_handle) {
            close_wait_task = nullptr;
            return true;
        }

        if (avi_player_handle) {
            avi_player_play_stop(avi_player_handle);
        }

        bool stopped = ulTaskNotifyTake(pdTRUE, timeout) != 0;
        close_wait_task = nullptr;

        if (!stopped || play_task_handle) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Timed out waiting for playback task");
            return false;
        }

        return true;
    }

    esp_err_t VideoPlayer::getAviFileList(const char *dir_path)
    {
        DIR *dir = opendir(dir_path);
        if (!dir) {
            ESP_LOGW(
                ESP_UTILS_LOG_TAG,
                "Open AVI directory failed: %s (errno=%d)",
                dir_path,
                errno
            );
            return ESP_ERR_NOT_FOUND;
        }

        auto is_regular_avi = [dir_path](const struct dirent *entry) {
            if (!entry) {
                return false;
            }
            const char *ext = strrchr(entry->d_name, '.');
            if (!ext || strcasecmp(ext, ".avi") != 0) {
                return false;
            }
            if (entry->d_type == DT_REG) {
                return true;
            }
            if (entry->d_type != DT_UNKNOWN) {
                return false;
            }

            char path[384] = {};
            int written = snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
            if (written <= 0 || static_cast<size_t>(written) >= sizeof(path)) {
                return false;
            }
            struct stat info = {};
            return stat(path, &info) == 0 && S_ISREG(info.st_mode);
        };

        int count = 0;
        struct dirent *entry = nullptr;
        while ((entry = readdir(dir)) != nullptr) {
            if (is_regular_avi(entry)) {
                ++count;
            }
        }
        if (count == 0) {
            closedir(dir);
            ESP_LOGW(ESP_UTILS_LOG_TAG, "No AVI files found in %s", dir_path);
            return ESP_ERR_NOT_FOUND;
        }

        avi_file_list = static_cast<char **>(calloc(count, sizeof(char *)));
        if (!avi_file_list) {
            closedir(dir);
            return ESP_ERR_NO_MEM;
        }

        int loaded_count = 0;
        rewinddir(dir);
        while ((entry = readdir(dir)) != nullptr && loaded_count < count) {
            if (!is_regular_avi(entry)) {
                continue;
            }
            size_t len = strlen(dir_path) + strlen(entry->d_name) + 2;
            char *full_path = static_cast<char *>(malloc(len));
            if (!full_path) {
                for (int i = 0; i < loaded_count; ++i) {
                    free(avi_file_list[i]);
                }
                free(avi_file_list);
                avi_file_list = nullptr;
                avi_file_count = 0;
                closedir(dir);
                return ESP_ERR_NO_MEM;
            }
            snprintf(full_path, len, "%s/%s", dir_path, entry->d_name);
            avi_file_list[loaded_count++] = full_path;
        }

        closedir(dir);
        avi_file_count = loaded_count;
        ESP_LOGI(
            ESP_UTILS_LOG_TAG,
            "Loaded %d AVI file(s) from %s",
            avi_file_count,
            dir_path
        );
        return avi_file_count > 0 ? ESP_OK : ESP_ERR_NOT_FOUND;
    }

    esp_err_t VideoPlayer::initDisplayBypass()
    {
#ifdef CONFIG_CACHE_L2_CACHE_LINE_SIZE
        data_cache_line_size = CONFIG_CACHE_L2_CACHE_LINE_SIZE;
#else
        data_cache_line_size = 64;
#endif

        display = lv_display_get_default();
        ESP_RETURN_ON_FALSE(display != nullptr, ESP_ERR_INVALID_STATE, ESP_UTILS_LOG_TAG, "LVGL display is not ready");

        display_panel = bsp_display_get_panel_handle();
        ESP_RETURN_ON_FALSE(display_panel != nullptr, ESP_ERR_INVALID_STATE, ESP_UTILS_LOG_TAG, "LCD panel is not ready");

        esp_err_t ret = ESP_OK;
#if CONFIG_BSP_LCD_DPI_BUFFER_NUMS >= 3
        ret = esp_lcd_dpi_panel_get_frame_buffer(
                  display_panel,
                  CONFIG_BSP_LCD_DPI_BUFFER_NUMS,
                  &lcd_buffers[0],
                  &lcd_buffers[1],
                  &lcd_buffers[2]
              );
#elif CONFIG_BSP_LCD_DPI_BUFFER_NUMS == 2
        ret = esp_lcd_dpi_panel_get_frame_buffer(
                  display_panel,
                  CONFIG_BSP_LCD_DPI_BUFFER_NUMS,
                  &lcd_buffers[0],
                  &lcd_buffers[1]
              );
#else
#error "VideoPlayer needs at least two LCD frame buffers"
#endif
        ESP_RETURN_ON_ERROR(ret, ESP_UTILS_LOG_TAG, "Get LCD frame buffers failed");
        current_buf_idx = 0;

        if (ppa_srm_handle == nullptr) {
            ppa_client_config_t ppa_srm_config = {
                .oper_type = PPA_OPERATION_SRM,
            };
            ESP_RETURN_ON_ERROR(
                ppa_register_client(&ppa_srm_config, &ppa_srm_handle),
                ESP_UTILS_LOG_TAG,
                "Register PPA SRM failed"
            );
        }

        bsp_display_cfg_t touch_cfg = {
            .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
            .rotation = ESP_LV_ADAPTER_ROTATE_0,
            .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
            .touch_flags = {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
        };
        ret = bsp_touch_new(&touch_cfg, &touch_handle);
        if (ret != ESP_OK) {
            deinitDisplayBypass();
            return ret;
        }

        ret = esp_lv_adapter_set_dummy_draw(display, true);
        if (ret != ESP_OK) {
            deinitDisplayBypass();
            return ret;
        }
        dummy_enabled = true;

        ret = esp_lv_adapter_pause(-1);
        if (ret != ESP_OK) {
            deinitDisplayBypass();
            return ret;
        }
        lvgl_paused = true;

        ESP_LOGI(
            ESP_UTILS_LOG_TAG,
            "Video display bypass: %dx%d, %d frame buffers",
            DISPLAY_WIDTH,
            DISPLAY_HEIGHT,
            CONFIG_BSP_LCD_DPI_BUFFER_NUMS
        );
        return ESP_OK;
    }

    void VideoPlayer::deinitDisplayBypass()
    {
        if (dummy_enabled && display) {
            esp_lv_adapter_set_dummy_draw(display, false);
            dummy_enabled = false;
        }

        if (lvgl_paused) {
            esp_lv_adapter_resume();
            lvgl_paused = false;
        }

        if (touch_handle) {
            esp_lcd_touch_del(touch_handle);
            touch_handle = nullptr;
        }

        memset(lcd_buffers, 0, sizeof(lcd_buffers));
        display_panel = nullptr;
        display = nullptr;
        current_buf_idx = 0;
        touch_active = false;
        last_video_width = 0;
        last_video_height = 0;
    }

    esp_err_t VideoPlayer::initJpegDecoder()
    {
        if (jpeg_handle != nullptr)
            return ESP_OK;

        jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
        config.output_type = VIDEO_JPEG_PIXEL_FORMAT;

        jpeg_error_t err = jpeg_dec_open(&config, &jpeg_handle);
        return (err == JPEG_ERR_OK) ? ESP_OK : ESP_FAIL;
    }

    void VideoPlayer::deinitJpegDecoder()
    {
        if (jpeg_handle)
        {
            jpeg_dec_close(jpeg_handle);
            jpeg_handle = nullptr;
        }
    }

    esp_err_t VideoPlayer::ensureJpegOutputBuffer(int out_len)
    {
        if (out_len <= 0) {
            return ESP_ERR_INVALID_SIZE;
        }

        if (jpeg_output_buffer && jpeg_output_buffer_size >= out_len) {
            return ESP_OK;
        }

        releaseJpegOutputBuffer();
        size_t alloc_size = ALIGN_UP(static_cast<size_t>(out_len), 16);
        jpeg_output_buffer = static_cast<uint8_t *>(
            heap_caps_aligned_calloc(16, 1, alloc_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!jpeg_output_buffer) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "JPEG output buffer allocation failed: %u bytes", static_cast<unsigned>(alloc_size));
            return ESP_ERR_NO_MEM;
        }

        jpeg_output_buffer_size = static_cast<int>(alloc_size);
        return ESP_OK;
    }

    void VideoPlayer::releaseJpegOutputBuffer()
    {
        if (jpeg_output_buffer) {
            heap_caps_free(jpeg_output_buffer);
            jpeg_output_buffer = nullptr;
            jpeg_output_buffer_size = 0;
        }
    }

    void VideoPlayer::videoCallback(frame_data_t *data, void *arg)
    {
        if (!data || !data->data || data->data_bytes == 0)
            return;

        VideoPlayer *self = static_cast<VideoPlayer *>(arg);
        const int next_buf_idx =
            (self->current_buf_idx + 1) % CONFIG_BSP_LCD_DPI_BUFFER_NUMS;

        if (self->initJpegDecoder() != ESP_OK ||
                !self->lcd_buffers[next_buf_idx] ||
                !self->dummy_enabled || !self->ppa_srm_handle) {
            return;
        }

        // Decode each MJPEG frame into a PSRAM buffer, then use PPA SRM to scale it
        // into the next LCD frame buffer without involving LVGL widgets.
        jpeg_dec_io_t io = {
            .inbuf = data->data,
            .inbuf_len = static_cast<int>(data->data_bytes),
            .outbuf = nullptr,
        };

        jpeg_dec_header_info_t info;
        if (jpeg_dec_parse_header(self->jpeg_handle, &io, &info) != JPEG_ERR_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "JPEG header parse failed");
            return;
        }

        if (info.width == 0 || info.height == 0) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Invalid JPEG frame size: %dx%d", info.width, info.height);
            return;
        }

        int out_len = 0;
        if (jpeg_dec_get_outbuf_len(self->jpeg_handle, &out_len) != JPEG_ERR_OK ||
            self->ensureJpegOutputBuffer(out_len) != ESP_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Prepare JPEG output buffer failed");
            return;
        }

        io.outbuf = self->jpeg_output_buffer;
        if (jpeg_dec_process(self->jpeg_handle, &io) != JPEG_ERR_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "JPEG decode process failed");
            return;
        }

        uint32_t crop_x = 0;
        uint32_t crop_y = 0;
        uint32_t crop_w = info.width;
        uint32_t crop_h = info.height;

        if (static_cast<uint64_t>(info.width) * DISPLAY_HEIGHT >
                static_cast<uint64_t>(info.height) * DISPLAY_WIDTH) {
            crop_w = static_cast<uint32_t>(
                (static_cast<uint64_t>(info.height) * DISPLAY_WIDTH) / DISPLAY_HEIGHT
            );
            crop_x = (info.width - crop_w) / 2;
        } else {
            crop_h = static_cast<uint32_t>(
                (static_cast<uint64_t>(info.width) * DISPLAY_HEIGHT) / DISPLAY_WIDTH
            );
            crop_y = (info.height - crop_h) / 2;
        }

        crop_w = crop_w ? crop_w : 1;
        crop_h = crop_h ? crop_h : 1;
        float scale_x = static_cast<float>(DISPLAY_WIDTH) / static_cast<float>(crop_w);
        float scale_y = static_cast<float>(DISPLAY_HEIGHT) / static_cast<float>(crop_h);

        if (self->last_video_width != info.width || self->last_video_height != info.height) {
            ESP_LOGI(ESP_UTILS_LOG_TAG, "Cover video frame: %dx%d, crop %u,%u %ux%u",
                     info.width, info.height, crop_x, crop_y, crop_w, crop_h);
            self->last_video_width = info.width;
            self->last_video_height = info.height;
        }

        void *lcd_buffer = self->lcd_buffers[next_buf_idx];

        ppa_srm_oper_config_t srm_config = {};
        srm_config.in.buffer = self->jpeg_output_buffer;
        srm_config.in.pic_w = info.width;
        srm_config.in.pic_h = info.height;
        srm_config.in.block_w = crop_w;
        srm_config.in.block_h = crop_h;
        srm_config.in.block_offset_x = crop_x;
        srm_config.in.block_offset_y = crop_y;
        srm_config.in.srm_cm = VIDEO_PPA_COLOR_MODE;

        srm_config.out.buffer = lcd_buffer;
        srm_config.out.buffer_size = ALIGN_UP(DISPLAY_WIDTH * DISPLAY_HEIGHT * VIDEO_BYTES_PER_PIXEL, self->data_cache_line_size);
        srm_config.out.pic_w = DISPLAY_WIDTH;
        srm_config.out.pic_h = DISPLAY_HEIGHT;
        srm_config.out.block_offset_x = 0;
        srm_config.out.block_offset_y = 0;
        srm_config.out.srm_cm = VIDEO_PPA_COLOR_MODE;

        srm_config.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
        srm_config.scale_x = scale_x;
        srm_config.scale_y = scale_y;
        srm_config.mirror_x = 0;
        srm_config.mirror_y = 0;
        srm_config.rgb_swap = 0;
        srm_config.byte_swap = 0;
        srm_config.mode = PPA_TRANS_MODE_BLOCKING;

        esp_err_t ret = ppa_do_scale_rotate_mirror(self->ppa_srm_handle, &srm_config);
        if (ret != ESP_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "PPA scale failed: %s", esp_err_to_name(ret));
            return;
        }

        ret = esp_lv_adapter_dummy_draw_blit(
                  self->display,
                  0,
                  0,
                  DISPLAY_WIDTH,
                  DISPLAY_HEIGHT,
                  lcd_buffer,
                  false
              );
        if (ret != ESP_OK) {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "Dummy draw blit failed: %s", esp_err_to_name(ret));
            return;
        }
        self->current_buf_idx = next_buf_idx;
    }

    void VideoPlayer::audioCallback(frame_data_t *data, void *arg)
    {
        VideoPlayer *self = static_cast<VideoPlayer *>(arg);
        if (!self || !self->audio_ready || !self->loop_playback ||
                !data || data->type != FRAME_TYPE_AUDIO || !data->data || data->data_bytes == 0) {
            return;
        }

        size_t bytes_written = 0;
        esp_err_t ret = bsp_extra_i2s_write(
            data->data,
            data->data_bytes,
            &bytes_written,
            1000
        );
        if (ret != ESP_OK || bytes_written != data->data_bytes) {
            ESP_LOGW(
                ESP_UTILS_LOG_TAG,
                "Audio write failed: %s, %u/%u bytes",
                esp_err_to_name(ret),
                static_cast<unsigned>(bytes_written),
                static_cast<unsigned>(data->data_bytes)
            );
        }
    }

    void VideoPlayer::audioSetClockCallback(uint32_t rate, uint32_t bits, uint32_t ch, void *arg)
    {
        VideoPlayer *self = static_cast<VideoPlayer *>(arg);
        if (!self || !self->audio_ready || !self->loop_playback) {
            return;
        }

        const uint32_t sample_rate = rate ? rate : CODEC_DEFAULT_SAMPLE_RATE;
        const uint32_t bit_width = bits ? bits : CODEC_DEFAULT_BIT_WIDTH;
        const i2s_slot_mode_t mode = (ch <= 1) ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO;
        esp_err_t ret = bsp_extra_codec_set_fs(sample_rate, bit_width, mode);
        if (ret != ESP_OK) {
            ESP_LOGW(ESP_UTILS_LOG_TAG, "Audio clock setup failed: %s", esp_err_to_name(ret));
        }
    }

    void VideoPlayer::playEndCallback(void *arg)
    {
        VideoPlayer *self = static_cast<VideoPlayer *>(arg);
        self->is_playing = false;
    }

    bool VideoPlayer::shouldExitBySwipe()
    {
        if (!touch_handle) {
            return false;
        }

        esp_lcd_touch_read_data(touch_handle);

        esp_lcd_touch_point_data_t points[1] = {};
        uint8_t point_num = 0;
        esp_err_t ret = esp_lcd_touch_get_data(touch_handle, points, &point_num, 1);

        if (ret != ESP_OK || point_num == 0) {
            touch_active = false;
            return false;
        }

        if (!touch_active) {
            touch_active = true;
            touch_start_x = points[0].x;
            touch_start_y = points[0].y;
            return false;
        }

        int dx = abs((int)points[0].x - (int)touch_start_x);
        int dy = abs((int)points[0].y - (int)touch_start_y);
        return dx > SWIPE_EXIT_THRESHOLD || dy > SWIPE_EXIT_THRESHOLD;
    }

    void VideoPlayer::playTask(void *arg)
    {
        VideoPlayer *self = static_cast<VideoPlayer *>(arg);
        bool request_close = false;
        self->audio_ready = (bsp_extra_codec_init() == ESP_OK);
        if (self->audio_ready) {
            esp_err_t volume_ret = bsp_extra_codec_volume_set(CODEC_DEFAULT_VOLUME, nullptr);
            if (volume_ret != ESP_OK) {
                ESP_LOGW(ESP_UTILS_LOG_TAG, "Set video volume failed: %s", esp_err_to_name(volume_ret));
            }
        } else {
            ESP_LOGW(ESP_UTILS_LOG_TAG, "Audio codec unavailable; continue video-only playback");
        }


        // The AVI reader keeps a larger buffer because high-resolution MJPEG frames
        // can otherwise underflow while the PPA path is preparing the next frame.
        avi_player_config_t cfg = {
            .buffer_size = 2 * 1024 * 1024,
            .video_cb = videoCallback,
            .audio_cb = self->audio_ready ? audioCallback : nullptr,
            .audio_set_clock_cb = self->audio_ready ? audioSetClockCallback : nullptr,
            .avi_play_end_cb = playEndCallback,
            .priority = 7,
            .coreID = 0,
            .user_data = self,
            .stack_size = 32768,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
            .stack_in_psram = true,
#endif
        };

        if (avi_player_init(cfg, &self->avi_player_handle) != ESP_OK)
        {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "avi_player_init failed");
            (void)bsp_extra_codec_output_stop();
            self->audio_ready = false;
            self->play_task_handle = nullptr;
            if (self->close_wait_task) {
                xTaskNotifyGive(self->close_wait_task);
            }
            vTaskDelete(NULL);
            return;
        }

        if (self->initDisplayBypass() != ESP_OK)
        {
            ESP_LOGE(ESP_UTILS_LOG_TAG, "initDisplayBypass failed");
            avi_player_deinit(self->avi_player_handle);
            self->avi_player_handle = nullptr;
            self->deinitDisplayBypass();
            (void)bsp_extra_codec_output_stop();
            self->audio_ready = false;
            self->play_task_handle = nullptr;
            if (self->close_wait_task) {
                xTaskNotifyGive(self->close_wait_task);
            }
            vTaskDelete(NULL);
            return;
        }

        // Loop through every AVI file discovered on the SD card until the app closes.
        while (self->loop_playback)
        {
            for (int i = 0; i < self->avi_file_count && self->loop_playback; i++)
            {
                while (self->is_paused && self->loop_playback)
                {
                    if (self->shouldExitBySwipe()) {
                        request_close = true;
                        self->loop_playback = false;
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                if (!self->loop_playback)
                    break;

                ESP_LOGI(ESP_UTILS_LOG_TAG, "Playing AVI file: %s", self->avi_file_list[i]);
                self->is_playing = true;
                if (avi_player_play_from_file(self->avi_player_handle, self->avi_file_list[i]) != ESP_OK) {
                    ESP_LOGE(ESP_UTILS_LOG_TAG, "Failed to play file: %s", self->avi_file_list[i]);
                    self->is_playing = false;
                    continue;
                }

                while (self->is_playing && self->loop_playback)
                {
                    if (self->shouldExitBySwipe()) {
                        request_close = true;
                        self->loop_playback = false;
                        avi_player_play_stop(self->avi_player_handle);
                        self->is_playing = false;
                        break;
                    }

                    if (self->is_paused)
                    {
                        avi_player_play_stop(self->avi_player_handle);
                        self->is_playing = false;
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(30));
                }
            }

            vTaskDelay(pdMS_TO_TICKS(500));
        }

        avi_player_play_stop(self->avi_player_handle);
        avi_player_deinit(self->avi_player_handle);
        self->avi_player_handle = nullptr;
        (void)bsp_extra_codec_output_stop();
        self->audio_ready = false;
        self->deinitDisplayBypass();

        self->play_task_handle = nullptr;
        if (self->close_wait_task)
        {
            xTaskNotifyGive(self->close_wait_task);
        }

        if (request_close)
        {
            self->notifyCoreClosed();
        }

        vTaskDelete(NULL);
    }

} // namespace esp_brookesia::apps
