/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sys/cdefs.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_codec_dev.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "audio_player.h"
#include "file_iterator.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CODEC_DEFAULT_SAMPLE_RATE           (16000)
#define CODEC_VOICE_SAMPLE_RATE             (24000)
#define CODEC_DEFAULT_BIT_WIDTH             (16)
#define CODEC_DEFAULT_ADC_VOLUME            (24.0)
#define CODEC_DEFAULT_CHANNEL               (2)
#define CODEC_VOICE_INPUT_CHANNELS          (4)
#define CODEC_DEFAULT_VOLUME                (80)

/*
 * The board schematic and ES7210 framing use MIC1, MIC3 (echo reference), MIC2,
 * MIC4 serialized order. TDM slot masks and physical MIC masks are different
 * namespaces.
 */
#define BSP_EXTRA_ES7210_TDM_SLOT_MIC1_MASK            (1U << 0)
#define BSP_EXTRA_ES7210_TDM_SLOT_MIC3_ECHO_MASK       (1U << 1)
#define BSP_EXTRA_ES7210_TDM_SLOT_MIC2_MASK            (1U << 2)
#define BSP_EXTRA_ES7210_TDM_SLOT_MIC4_MASK            (1U << 3)
#define BSP_EXTRA_ES7210_TDM_ALL_SLOTS_MASK            \
    (BSP_EXTRA_ES7210_TDM_SLOT_MIC1_MASK |             \
     BSP_EXTRA_ES7210_TDM_SLOT_MIC3_ECHO_MASK |        \
     BSP_EXTRA_ES7210_TDM_SLOT_MIC2_MASK |             \
     BSP_EXTRA_ES7210_TDM_SLOT_MIC4_MASK)
#define BSP_EXTRA_ES7210_PHYSICAL_MIC1_MASK            (1U << 0)
#define BSP_EXTRA_ES7210_PHYSICAL_MIC2_MASK            (1U << 1)
#define BSP_EXTRA_ES7210_PHYSICAL_MIC3_MASK            (1U << 2)
#define BSP_EXTRA_ES7210_PHYSICAL_FRONT_MIC_MASK       \
    (BSP_EXTRA_ES7210_PHYSICAL_MIC1_MASK | BSP_EXTRA_ES7210_PHYSICAL_MIC2_MASK)
#define BSP_EXTRA_ES7210_PHYSICAL_CONNECTED_MIC_MASK   \
    (BSP_EXTRA_ES7210_PHYSICAL_FRONT_MIC_MASK | BSP_EXTRA_ES7210_PHYSICAL_MIC3_MASK)

#define BSP_LCD_BACKLIGHT_BRIGHTNESS_MAX    (95)
#define BSP_LCD_BACKLIGHT_BRIGHTNESS_MIN    (0)
#define LCD_LEDC_CH                         (CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH)

/**************************************************************************************************
 * BSP Extra interface
 * Mainly provided display-lock and I2S Codec interfaces.
 **************************************************************************************************/
/**
 * @brief Lock LVGL through esp_lv_adapter.
 *
 * The local BSP returns esp_err_t directly from a bool function, which
 * inverts successful ESP_OK results. Use this compatibility wrapper instead.
 *
 * @param timeout_ms Maximum wait in milliseconds; negative waits forever.
 * @return true when the lock was acquired.
 */
bool bsp_extra_display_lock(int32_t timeout_ms);

/** @brief Release the LVGL lock acquired by bsp_extra_display_lock(). */
void bsp_extra_display_unlock(void);

/**
 * @brief Get display brightness.
 *
 * @return
 *   - brightness in percent
 */
int bsp_display_brightness_get(void);

/**
 * @brief Player set mute.
 *
 * @param enable: true or false
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_mute_set(bool enable);

/**
 * @brief Player set volume.
 *
 * @param volume: volume set
 * @param volume_set: volume set response
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_volume_set(int volume, int *volume_set);

/**
 * @brief Player get volume.
 *
 * @return
 *   - volume: volume get
 */
int bsp_extra_codec_volume_get(void);

/**
 * @brief Stop I2S function.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_dev_stop(void);

/**
 * @brief Stop only the playback codec session.
 *
 * The ES7210 capture session is left untouched so a voice consumer can keep
 * running while a media app releases its ES8311 output session.
 */
esp_err_t bsp_extra_codec_output_stop(void);

/**
 * @brief Resume I2S function.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_dev_resume(void);

/**
 * @brief Configure and open the output codec only.
 *
 * Any active input and output codec sessions are closed first. This function is
 * used by ordinary audio players, whose source sample rate must not reconfigure
 * or start the TDM microphone input.
 *
 * @param rate Sample rate
 * @param bits_cfg Bits per sample
 * @param ch Output channel mode
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);

/**
 * @brief Configure full-duplex voice audio with STD output and TDM input.
 *
 * Output is always stereo. record_channels is the physical TDM slot count.
 * record_tdm_slot_mask selects serialized slots returned by reads, while
 * record_mic_gain_mask selects physical ES7210 MIC inputs for gain control.
 * These masks must not be reused as each other.
 *
 * @param rate Shared physical sample rate
 * @param bits_cfg Bits per sample
 * @param record_channels Physical TDM slot count
 * @param record_tdm_slot_mask Selected serialized TDM slot mask
 * @param record_mic_gain_mask Selected physical ES7210 MIC gain mask
 *
 * @return
 *    - ESP_OK: Success
 *    - ESP_ERR_INVALID_ARG: Invalid channel selection
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_set_voice_fs(uint32_t rate, uint32_t bits_cfg,
                                       uint8_t record_channels,
                                       uint16_t record_tdm_slot_mask,
                                       uint16_t record_mic_gain_mask);

/**
 * @brief Read data from recoder.
 *
 * @param audio_buffer: The pointer of receiving data buffer
 * @param len: Max data buffer length
 * @param bytes_read: Byte number that actually be read, can be NULL if not needed
 * @param timeout_ms: Max block time
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief Write data to player.
 *
 * @param audio_buffer: The pointer of sent data buffer
 * @param len: Max data buffer length
 * @param bytes_written: Byte number that actually be sent, can be NULL if not needed
 * @param timeout_ms: Max block time
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);


/**
 * @brief Initialize codec play and record handle.
 *
 * @return
 *      - ESP_OK: Success
 *      - Others: Fail
 */
esp_err_t bsp_extra_codec_init(void);

/**
 * @brief Initialize audio player task.
 *
 * @param path file path
 *
 * @return
 *      - ESP_OK: Success
 *      - Others: Fail
 */
esp_err_t bsp_extra_player_init(void);

/**
 * @brief Delete audio player task.
 *
 * @return
 *      - ESP_OK: Success
 *      - Others: Fail
 */
esp_err_t bsp_extra_player_del(void);

/**
 * @brief Initialize a file iterator instance
 *
 * @param path The file path for the iterator.
 * @param ret_instance A pointer to the file iterator instance to be returned.
 * @return
 *     - ESP_OK: Successfully initialized the file iterator instance.
 *     - ESP_FAIL: Failed to initialize the file iterator instance due to invalid parameters or memory allocation failure.
 */
esp_err_t bsp_extra_file_instance_init(const char *path, file_iterator_instance_t **ret_instance);

/**
 * @brief Play the audio file at the specified index in the file iterator
 *
 * @param instance The file iterator instance.
 * @param index The index of the file to play within the iterator.
 * @return
 *     - ESP_OK: Successfully started playing the audio file.
 *     - ESP_FAIL: Failed to play the audio file due to invalid parameters or file access issues.
 */
esp_err_t bsp_extra_player_play_index(file_iterator_instance_t *instance, int index);

/**
 * @brief Play the audio file specified by the file path
 *
 * @param file_path The path to the audio file to be played.
 * @return
 *     - ESP_OK: Successfully started playing the audio file.
 *     - ESP_FAIL: Failed to play the audio file due to file access issues.
 */
esp_err_t bsp_extra_player_play_file(const char *file_path);

/**
 * @brief Register a callback function for the audio player
 *
 * @param cb The callback function to be registered.
 * @param user_data User data to be passed to the callback function.
 */
void bsp_extra_player_register_callback(audio_player_cb_t cb, void *user_data);

/**
 * @brief Check if the specified audio file is currently playing
 *
 * @param file_path The path to the audio file to check.
 * @return
 *     - true: The specified audio file is currently playing.
 *     - false: The specified audio file is not currently playing.
 */
bool bsp_extra_player_is_playing_by_path(const char *file_path);

/**
 * @brief Check if the audio file at the specified index is currently playing
 *
 * @param instance The file iterator instance.
 * @param index The index of the file to check.
 * @return
 *     - true: The audio file at the specified index is currently playing.
 *     - false: The audio file at the specified index is not currently playing.
 */
bool bsp_extra_player_is_playing_by_index(file_iterator_instance_t *instance, int index);

#ifdef __cplusplus
}
#endif
