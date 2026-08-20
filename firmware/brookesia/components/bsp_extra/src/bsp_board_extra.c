/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_codec_dev_defaults.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_vfs_fat.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/semphr.h"

#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"

static const char *TAG = "bsp_extra_board";

static esp_codec_dev_handle_t play_dev_handle;
static esp_codec_dev_handle_t record_dev_handle;
static i2s_chan_handle_t i2s_tx_chan;
static i2s_chan_handle_t i2s_rx_chan;
static const audio_codec_data_if_t *i2s_data_if;
static const audio_codec_gpio_if_t *speaker_gpio_if;
static const audio_codec_ctrl_if_t *speaker_ctrl_if;
static const audio_codec_if_t *speaker_codec_if;
static const audio_codec_ctrl_if_t *microphone_ctrl_if;
static const audio_codec_if_t *microphone_codec_if;

static bool _is_audio_init = false;
static bool _is_player_init = false;
static int _vloume_intensity = CODEC_DEFAULT_VOLUME;
static SemaphoreHandle_t audio_mutex;

static audio_player_cb_t audio_idle_callback = NULL;
static void *audio_idle_cb_user_data = NULL;
static char audio_file_path[128];

static esp_err_t audio_lock(void);
static void audio_unlock(void);

#define BSP_EXTRA_ES7210_CODEC_ADDR      ES7210_CODEC_DEFAULT_ADDR

#define BSP_EXTRA_I2S_GPIO_CFG  \
    {                           \
        .mclk = BSP_I2S_MCLK,   \
        .bclk = BSP_I2S_SCLK,   \
        .ws = BSP_I2S_LCLK,     \
        .dout = BSP_I2S_DOUT,   \
        .din = BSP_I2S_DSIN,    \
        .invert_flags = {       \
            .mclk_inv = false,  \
            .bclk_inv = false,  \
            .ws_inv = false,    \
        },                      \
    }

/**************************************************************************************************
 *
 * Extra Board Function
 *
 **************************************************************************************************/

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    // Volume saved when muting and restored when unmuting. Restoring volume is necessary
    // as es8311_set_voice_mute(true) results in voice volume (REG32) being set to zero.

    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock player mute failed");
    esp_err_t ret = bsp_extra_codec_mute_set(setting == AUDIO_PLAYER_MUTE ? true : false);

    // restore the voice volume upon unmuting
    if (ret == ESP_OK && setting == AUDIO_PLAYER_UNMUTE) {
        ret = esp_codec_dev_set_out_vol(play_dev_handle, _vloume_intensity);
    }
    audio_unlock();

    return ret;
}

static void audio_callback(audio_player_cb_ctx_t *ctx)
{
    if (audio_idle_callback) {
        ctx->user_ctx = audio_idle_cb_user_data;
        audio_idle_callback(ctx);
    }
}

bool bsp_extra_display_lock(int32_t timeout_ms)
{
    return esp_lv_adapter_lock(timeout_ms) == ESP_OK;
}

void bsp_extra_display_unlock(void)
{
    esp_lv_adapter_unlock();
}

static esp_err_t audio_mutex_init(void)
{
    if (!audio_mutex) {
        audio_mutex = xSemaphoreCreateRecursiveMutex();
    }
    return audio_mutex ? ESP_OK : ESP_ERR_NO_MEM;
}

static esp_err_t audio_lock(void)
{
    ESP_RETURN_ON_ERROR(audio_mutex_init(), TAG, "Create audio mutex failed");
    return xSemaphoreTakeRecursive(audio_mutex, portMAX_DELAY) == pdTRUE
           ? ESP_OK : ESP_ERR_TIMEOUT;
}

static void audio_unlock(void)
{
    if (audio_mutex) {
        xSemaphoreGiveRecursive(audio_mutex);
    }
}

static void update_codec_result(esp_err_t *result, esp_err_t codec_result)
{
    if (*result == ESP_OK && codec_result != ESP_OK) {
        *result = codec_result;
    }
}

static void update_codec_interface_result(esp_err_t *result, int codec_result)
{
    if (*result == ESP_OK && codec_result != ESP_CODEC_DEV_OK) {
        *result = ESP_FAIL;
    }
}

static esp_err_t delete_i2s_channel(i2s_chan_handle_t *channel)
{
    if (!channel || !*channel) {
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;
    esp_err_t err = i2s_channel_disable(*channel);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ret = err;
    }
    update_codec_result(&ret, i2s_del_channel(*channel));
    *channel = NULL;
    return ret;
}

static esp_err_t delete_speaker_codec(void)
{
    esp_err_t ret = ESP_OK;

    if (play_dev_handle) {
        update_codec_result(&ret, esp_codec_dev_close(play_dev_handle));
        esp_codec_dev_delete(play_dev_handle);
        play_dev_handle = NULL;
    }
    if (speaker_codec_if) {
        update_codec_interface_result(&ret, audio_codec_delete_codec_if(speaker_codec_if));
        speaker_codec_if = NULL;
    }
    if (speaker_ctrl_if) {
        update_codec_interface_result(&ret, audio_codec_delete_ctrl_if(speaker_ctrl_if));
        speaker_ctrl_if = NULL;
    }
    if (speaker_gpio_if) {
        update_codec_interface_result(&ret, audio_codec_delete_gpio_if(speaker_gpio_if));
        speaker_gpio_if = NULL;
    }
    return ret;
}

static esp_err_t delete_microphone_codec(void)
{
    esp_err_t ret = ESP_OK;

    if (record_dev_handle) {
        update_codec_result(&ret, esp_codec_dev_close(record_dev_handle));
        esp_codec_dev_delete(record_dev_handle);
        record_dev_handle = NULL;
    }
    if (microphone_codec_if) {
        update_codec_interface_result(&ret, audio_codec_delete_codec_if(microphone_codec_if));
        microphone_codec_if = NULL;
    }
    if (microphone_ctrl_if) {
        update_codec_interface_result(&ret, audio_codec_delete_ctrl_if(microphone_ctrl_if));
        microphone_ctrl_if = NULL;
    }
    return ret;
}

static esp_err_t bsp_extra_audio_deinit(void)
{
    esp_err_t ret = ESP_OK;

    update_codec_result(&ret, delete_microphone_codec());
    update_codec_result(&ret, delete_speaker_codec());
    if (i2s_data_if) {
        update_codec_interface_result(&ret, audio_codec_delete_data_if(i2s_data_if));
        i2s_data_if = NULL;
    }
    update_codec_result(&ret, delete_i2s_channel(&i2s_rx_chan));
    update_codec_result(&ret, delete_i2s_channel(&i2s_tx_chan));
    _is_audio_init = false;
    return ret;
}

static esp_err_t bsp_extra_audio_bus_init(void)
{
    ESP_RETURN_ON_FALSE(
        !i2s_tx_chan && !i2s_rx_chan && !i2s_data_if,
        ESP_ERR_INVALID_STATE,
        TAG,
        "Audio bus is already initialized"
    );

    esp_err_t ret = ESP_OK;
    i2s_chan_config_t channel_config =
        I2S_CHANNEL_DEFAULT_CONFIG(CONFIG_BSP_I2S_NUM, I2S_ROLE_MASTER);
    channel_config.auto_clear = true;

    i2s_std_config_t tx_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(CODEC_VOICE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO
        ),
        .gpio_cfg = BSP_EXTRA_I2S_GPIO_CFG,
    };
    i2s_tdm_config_t rx_config = {
        .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(CODEC_VOICE_SAMPLE_RATE),
        .slot_cfg = I2S_TDM_PHILIP_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT,
            I2S_SLOT_MODE_STEREO,
            I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3
        ),
        .gpio_cfg = BSP_EXTRA_I2S_GPIO_CFG,
    };
    tx_config.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    tx_config.gpio_cfg.din = I2S_GPIO_UNUSED;
    rx_config.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    rx_config.clk_cfg.bclk_div = 8;
    rx_config.slot_cfg.total_slot = CODEC_VOICE_INPUT_CHANNELS;
    rx_config.gpio_cfg.dout = I2S_GPIO_UNUSED;

    ESP_GOTO_ON_ERROR(
        i2s_new_channel(&channel_config, &i2s_tx_chan, &i2s_rx_chan),
        fail,
        TAG,
        "Create I2S channels failed"
    );
    ESP_GOTO_ON_ERROR(
        i2s_channel_init_std_mode(i2s_tx_chan, &tx_config),
        fail,
        TAG,
        "Initialize I2S TX failed"
    );
    ESP_GOTO_ON_ERROR(
        i2s_channel_init_tdm_mode(i2s_rx_chan, &rx_config),
        fail,
        TAG,
        "Initialize I2S TDM RX failed"
    );
    ESP_GOTO_ON_ERROR(
        i2s_channel_enable(i2s_tx_chan), fail, TAG, "Enable I2S TX failed"
    );
    ESP_GOTO_ON_ERROR(
        i2s_channel_enable(i2s_rx_chan), fail, TAG, "Enable I2S RX failed"
    );

    audio_codec_i2s_cfg_t i2s_config = {
        .port = CONFIG_BSP_I2S_NUM,
        .rx_handle = i2s_rx_chan,
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&i2s_config);
    ESP_GOTO_ON_FALSE(
        i2s_data_if, ESP_ERR_NO_MEM, fail, TAG, "Create codec I2S data interface failed"
    );
    return ESP_OK;

fail:
    {
        esp_err_t cleanup_ret = bsp_extra_audio_deinit();
        if (cleanup_ret != ESP_OK) {
            ESP_LOGE(TAG, "Clean up audio bus failed: %s", esp_err_to_name(cleanup_ret));
        }
    }
    return ret;
}

static esp_codec_dev_handle_t create_speaker_codec(void)
{
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize codec I2C bus failed: %s", esp_err_to_name(ret));
        return NULL;
    }
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle();
    ESP_RETURN_ON_FALSE(i2c_bus, NULL, TAG, "Codec I2C bus is unavailable");

    speaker_gpio_if = audio_codec_new_gpio();
    if (!speaker_gpio_if) {
        goto fail;
    }

    audio_codec_i2c_cfg_t i2c_config = {
        .port = BSP_I2C_NUM,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus,
    };
    speaker_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_config);
    if (!speaker_ctrl_if) {
        goto fail;
    }

    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,
        .codec_dac_voltage = 3.3,
    };
    es8311_codec_cfg_t codec_config = {
        .ctrl_if = speaker_ctrl_if,
        .gpio_if = speaker_gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = BSP_POWER_AMP_IO,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = gain,
    };
    speaker_codec_if = es8311_codec_new(&codec_config);
    if (!speaker_codec_if) {
        goto fail;
    }

    esp_codec_dev_cfg_t device_config = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = speaker_codec_if,
        .data_if = i2s_data_if,
    };
    play_dev_handle = esp_codec_dev_new(&device_config);
    if (!play_dev_handle) {
        goto fail;
    }
    return play_dev_handle;

fail:
    (void)delete_speaker_codec();
    return NULL;
}

static esp_codec_dev_handle_t create_microphone_codec(void)
{
    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize codec I2C bus failed: %s", esp_err_to_name(ret));
        return NULL;
    }
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle();
    ESP_RETURN_ON_FALSE(i2c_bus, NULL, TAG, "Codec I2C bus is unavailable");

    audio_codec_i2c_cfg_t i2c_config = {
        .port = BSP_I2C_NUM,
        .addr = BSP_EXTRA_ES7210_CODEC_ADDR,
        .bus_handle = i2c_bus,
    };
    microphone_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_config);
    if (!microphone_ctrl_if) {
        goto fail;
    }

    es7210_codec_cfg_t codec_config = {
        .ctrl_if = microphone_ctrl_if,
        .mic_selected = BSP_EXTRA_ES7210_PHYSICAL_CONNECTED_MIC_MASK,
    };
    microphone_codec_if = es7210_codec_new(&codec_config);
    if (!microphone_codec_if) {
        goto fail;
    }

    esp_codec_dev_cfg_t device_config = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = microphone_codec_if,
        .data_if = i2s_data_if,
    };
    record_dev_handle = esp_codec_dev_new(&device_config);
    if (!record_dev_handle) {
        goto fail;
    }
    return record_dev_handle;

fail:
    (void)delete_microphone_codec();
    return NULL;
}

esp_err_t bsp_extra_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (bytes_read) {
        *bytes_read = 0;
    }
    ESP_RETURN_ON_FALSE(record_dev_handle && audio_buffer, ESP_ERR_INVALID_STATE, TAG, "Recorder is not initialized");

    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock audio input failed");
    esp_err_t ret = esp_codec_dev_read(record_dev_handle, audio_buffer, len);
    if (ret == ESP_OK && bytes_read) {
        *bytes_read = len;
    }
    audio_unlock();
    return ret;
}

esp_err_t bsp_extra_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (bytes_written) {
        *bytes_written = 0;
    }
    ESP_RETURN_ON_FALSE(play_dev_handle && audio_buffer, ESP_ERR_INVALID_STATE, TAG, "Player is not initialized");

    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock audio output failed");
    esp_err_t ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    if (ret == ESP_OK && bytes_written) {
        *bytes_written = len;
    }
    audio_unlock();
    return ret;
}

esp_err_t bsp_extra_codec_output_stop(void)
{
    if (!play_dev_handle) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock playback codec failed");
    esp_err_t ret = esp_codec_dev_close(play_dev_handle);
    audio_unlock();
    return ret == ESP_CODEC_DEV_OK ? ESP_OK : ret;
}

esp_err_t bsp_extra_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    ESP_RETURN_ON_FALSE(play_dev_handle && record_dev_handle, ESP_ERR_INVALID_STATE, TAG, "Codec is not initialized");
    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock playback reconfiguration failed");
    // Ordinary media/output reconfiguration must not close the ES7210 voice
    // capture session. Voice mode uses bsp_extra_codec_set_voice_fs(), which
    // intentionally owns both sides of the shared bus.
    esp_err_t ret = bsp_extra_codec_output_stop();
    if (ret != ESP_OK) {
        audio_unlock();
        ESP_LOGE(TAG, "Close playback codec failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_codec_dev_sample_info_t output_fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    ret = esp_codec_dev_open(play_dev_handle, &output_fs);
    if (ret == ESP_OK) {
        ret = esp_codec_dev_set_out_mute(play_dev_handle, false);
    }
    if (ret != ESP_OK) {
        (void)esp_codec_dev_close(play_dev_handle);
    }
    audio_unlock();
    return ret;
}

esp_err_t bsp_extra_codec_set_voice_fs(uint32_t rate, uint32_t bits_cfg,
                                       uint8_t record_channels,
                                       uint16_t record_tdm_slot_mask,
                                       uint16_t record_mic_gain_mask)
{
    ESP_RETURN_ON_FALSE(play_dev_handle && record_dev_handle, ESP_ERR_INVALID_STATE, TAG, "Codec is not initialized");
    ESP_RETURN_ON_FALSE(record_channels > 0 && record_channels <= 16 &&
                        record_tdm_slot_mask != 0 &&
                        (record_tdm_slot_mask >> record_channels) == 0 &&
                        record_mic_gain_mask != 0 &&
                        (record_mic_gain_mask >> 4) == 0,
                        ESP_ERR_INVALID_ARG, TAG, "Invalid record channel selection");
    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock voice codec reconfiguration failed");
    esp_err_t ret = bsp_extra_codec_dev_stop();
    if (ret != ESP_OK) {
        audio_unlock();
        ESP_LOGE(TAG, "Close codec devices failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_codec_dev_sample_info_t output_fs = {
        .sample_rate = rate,
        .channel = I2S_SLOT_MODE_STEREO,
        .bits_per_sample = bits_cfg,
    };
    esp_codec_dev_sample_info_t input_fs = {
        .sample_rate = rate,
        .channel = record_channels,
        .channel_mask = record_tdm_slot_mask,
        .bits_per_sample = bits_cfg,
    };

    ret = esp_codec_dev_open(play_dev_handle, &output_fs);
    if (ret == ESP_OK) {
        ret = esp_codec_dev_set_out_mute(play_dev_handle, false);
    }
    if (ret != ESP_OK) {
        (void)bsp_extra_codec_dev_stop();
        audio_unlock();
        return ret;
    }

    ret = esp_codec_dev_open(record_dev_handle, &input_fs);
    if (ret == ESP_OK) {
        ret = esp_codec_dev_set_in_channel_gain(
                  record_dev_handle, record_mic_gain_mask, CODEC_DEFAULT_ADC_VOLUME
              );
    }
    if (ret != ESP_OK) {
        (void)bsp_extra_codec_dev_stop();
    }
    audio_unlock();
    return ret;
}

esp_err_t bsp_extra_codec_volume_set(int volume, int *volume_set)
{
    (void)volume_set;
    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock codec volume failed");
    _vloume_intensity = volume;
    esp_err_t ret = esp_codec_dev_set_out_vol(play_dev_handle, volume);
    audio_unlock();
    ESP_RETURN_ON_ERROR(ret, TAG, "Set Codec volume failed");

    ESP_LOGI(TAG, "Setting volume: %d", volume);

    return ESP_OK;
}

int bsp_extra_codec_volume_get(void)
{
    return _vloume_intensity;
}

esp_err_t bsp_extra_codec_mute_set(bool enable)
{
    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock codec mute failed");
    esp_err_t ret = esp_codec_dev_set_out_mute(play_dev_handle, enable);
    audio_unlock();
    return ret;
}

esp_err_t bsp_extra_codec_dev_stop(void)
{
    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock codec stop failed");
    esp_err_t ret = ESP_OK;

    if (record_dev_handle) {
        update_codec_result(&ret, esp_codec_dev_close(record_dev_handle));
    }
    if (play_dev_handle) {
        update_codec_result(&ret, esp_codec_dev_close(play_dev_handle));
    }

    audio_unlock();
    return ret;
}

esp_err_t bsp_extra_codec_dev_resume(void)
{
    return bsp_extra_codec_set_fs(CODEC_DEFAULT_SAMPLE_RATE, CODEC_DEFAULT_BIT_WIDTH, CODEC_DEFAULT_CHANNEL);
}

esp_err_t bsp_extra_codec_init(void)
{
    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock codec initialization failed");
    if (_is_audio_init) {
        audio_unlock();
        return ESP_OK;
    }

    esp_err_t ret = bsp_extra_audio_bus_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize mixed STD/TDM audio bus failed: %s", esp_err_to_name(ret));
        audio_unlock();
        return ret;
    }

    if (!create_speaker_codec() || !create_microphone_codec()) {
        ret = ESP_FAIL;
        goto cleanup;
    }

    ret = bsp_extra_codec_set_fs(
              CODEC_DEFAULT_SAMPLE_RATE,
              CODEC_DEFAULT_BIT_WIDTH,
              CODEC_DEFAULT_CHANNEL
          );
    if (ret != ESP_OK) {
        goto cleanup;
    }
    _is_audio_init = true;
    audio_unlock();
    return ESP_OK;

cleanup:
    {
        esp_err_t cleanup_ret = bsp_extra_audio_deinit();
        if (cleanup_ret != ESP_OK) {
            ESP_LOGE(TAG, "Clean up codec initialization failed: %s", esp_err_to_name(cleanup_ret));
        }
    }
    audio_unlock();
    return ret;
}

esp_err_t bsp_extra_player_init(void)
{
    if (_is_player_init) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(bsp_extra_codec_init(), TAG, "Initialize codec failed");
    ESP_RETURN_ON_ERROR(bsp_extra_codec_dev_resume(), TAG, "Resume output codec failed");
    audio_player_config_t config = { .mute_fn = audio_mute_function,
                                     .write_fn = bsp_extra_i2s_write,
                                     .clk_set_fn = bsp_extra_codec_set_fs,
                                     .priority = 5
                                   };
    ESP_RETURN_ON_ERROR(audio_player_new(config), TAG, "audio_player_init failed");
    audio_player_callback_register(audio_callback, NULL);

    _is_player_init = true;

    return ESP_OK;
}

esp_err_t bsp_extra_player_del(void)
{
    esp_err_t ret = audio_player_delete();
    ESP_RETURN_ON_ERROR(ret, TAG, "Delete audio player failed");
    _is_player_init = false;
    ESP_RETURN_ON_ERROR(bsp_extra_codec_output_stop(), TAG, "Stop playback codec failed");

    return ESP_OK;
}

esp_err_t bsp_extra_file_instance_init(const char *path, file_iterator_instance_t **ret_instance)
{
    ESP_RETURN_ON_FALSE(path, ESP_FAIL, TAG, "path is NULL");
    ESP_RETURN_ON_FALSE(ret_instance, ESP_FAIL, TAG, "ret_instance is NULL");

    file_iterator_instance_t *file_iterator = file_iterator_new(path);
    ESP_RETURN_ON_FALSE(file_iterator, ESP_FAIL, TAG, "file_iterator_new failed, %s", path);

    *ret_instance = file_iterator;

    return ESP_OK;
}

esp_err_t bsp_extra_player_play_index(file_iterator_instance_t *instance, int index)
{
    ESP_RETURN_ON_FALSE(instance, ESP_FAIL, TAG, "instance is NULL");

    ESP_LOGI(TAG, "play_index(%d)", index);
    char filename[128];
    int retval = file_iterator_get_full_path_from_index(instance, index, filename, sizeof(filename));
    ESP_RETURN_ON_FALSE(retval != 0, ESP_FAIL, TAG, "file_iterator_get_full_path_from_index failed");

    ESP_LOGI(TAG, "opening file '%s'", filename);
    FILE *fp = fopen(filename, "rb");
    ESP_RETURN_ON_FALSE(fp, ESP_FAIL, TAG, "unable to open file");

    ESP_LOGI(TAG, "Playing '%s'", filename);
    esp_err_t ret = audio_player_play(fp);
    if (ret != ESP_OK) {
        fclose(fp);
        ESP_LOGE(TAG, "audio_player_play failed: %s", esp_err_to_name(ret));
        return ret;
    }

    file_iterator_set_index(instance, index);
    memcpy(audio_file_path, filename, sizeof(audio_file_path));

    return ESP_OK;
}

esp_err_t bsp_extra_player_play_file(const char *file_path)
{
    ESP_LOGI(TAG, "opening file '%s'", file_path);
    FILE *fp = fopen(file_path, "rb");
    ESP_RETURN_ON_FALSE(fp, ESP_FAIL, TAG, "unable to open file");

    ESP_LOGI(TAG, "Playing '%s'", file_path);
    esp_err_t ret = audio_player_play(fp);
    if (ret != ESP_OK) {
        fclose(fp);
        ESP_LOGE(TAG, "audio_player_play failed: %s", esp_err_to_name(ret));
        return ret;
    }

    memcpy(audio_file_path, file_path, sizeof(audio_file_path));

    return ESP_OK;
}

void bsp_extra_player_register_callback(audio_player_cb_t cb, void *user_data)
{
    audio_idle_callback = cb;
    audio_idle_cb_user_data = user_data;
}

bool bsp_extra_player_is_playing_by_path(const char *file_path)
{
    return (strcmp(audio_file_path, file_path) == 0);
}

bool bsp_extra_player_is_playing_by_index(file_iterator_instance_t *instance, int index)
{
    return (index == file_iterator_get_index(instance));
}
