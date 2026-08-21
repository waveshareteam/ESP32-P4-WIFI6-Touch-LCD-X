/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 *
 * ES7210 microphone capture demo for the Waveshare ESP32-P4-WIFI6-Touch-LCD-X.
 *
 * Captures physical MIC1 and MIC2 through the ES7210 ADC as legacy I2S stereo
 * PCM. MIC3 is the echo-reference path and MIC4 is not enabled by this sketch.
 * Every 30 ms it reports the left/MIC1, right/MIC2, and combined statistics.
 *
 * I2C: SDA=GPIO7, SCL=GPIO8.
 * I2S: MCLK=GPIO13, BCLK=GPIO12, LRCK=GPIO10, DIN=GPIO11.
 */
#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <math.h>

#include "audio_hal.h"
#include "es7210.h"

namespace {

constexpr int kI2sMclkPin = 13;
constexpr int kI2sBclkPin = 12;
constexpr int kI2sLrckPin = 10;
constexpr int kI2sDataInPin = 11;
constexpr uint32_t kSampleRate = 16000;
constexpr uint32_t kFrameLengthMs = 30;
constexpr size_t kFramesPerBuffer = kFrameLengthMs * kSampleRate / 1000U;
constexpr size_t kStereoChannelCount = 2;
constexpr size_t kInterleavedSamples = kFramesPerBuffer * kStereoChannelCount;
constexpr i2s_port_t kI2sPort = I2S_NUM_0;

int16_t *g_frame = nullptr;
bool g_capture_ready = false;

bool check_result(esp_err_t result, const char *step)
{
  if (result == ESP_OK) {
    return true;
  }
  Serial.printf("%s failed: %s\n", step, esp_err_to_name(result));
  return false;
}

bool init_codec()
{
  audio_hal_codec_config_t config = {
      .adc_input = AUDIO_HAL_ADC_INPUT_ALL,
      .codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE,
      .i2s_iface = {
          .mode = AUDIO_HAL_MODE_SLAVE,
          .fmt = AUDIO_HAL_I2S_NORMAL,
          .samples = AUDIO_HAL_16K_SAMPLES,
          .bits = AUDIO_HAL_BIT_LENGTH_16BITS,
      },
  };

  if (!check_result(es7210_adc_init(&Wire, &config), "ES7210 init") ||
      !check_result(es7210_adc_config_i2s(config.codec_mode, &config.i2s_iface), "ES7210 I2S configuration") ||
      !check_result(es7210_mic_select(static_cast<es7210_input_mics_t>(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2)), "ES7210 MIC1/2 selection") ||
      !check_result(es7210_adc_set_gain(static_cast<es7210_input_mics_t>(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2), GAIN_24DB), "ES7210 MIC1/2 gain") ||
      !check_result(es7210_adc_ctrl_state(config.codec_mode, AUDIO_HAL_CTRL_START), "ES7210 start")) {
    es7210_adc_deinit();
    return false;
  }
  return true;
}

bool init_i2s()
{
  const i2s_config_t config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = kSampleRate,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0,
      .mclk_multiple = I2S_MCLK_MULTIPLE_256,
      .bits_per_chan = I2S_BITS_PER_CHAN_16BIT,
  };
  const i2s_pin_config_t pins = {
      .mck_io_num = kI2sMclkPin,
      .bck_io_num = kI2sBclkPin,
      .ws_io_num = kI2sLrckPin,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = kI2sDataInPin,
  };
  if (!check_result(i2s_driver_install(kI2sPort, &config, 0, nullptr), "I2S driver install")) {
    return false;
  }
  if (!check_result(i2s_set_pin(kI2sPort, &pins), "I2S pin configuration")) {
    i2s_driver_uninstall(kI2sPort);
    return false;
  }
  if (!check_result(i2s_zero_dma_buffer(kI2sPort), "I2S DMA clear")) {
    i2s_driver_uninstall(kI2sPort);
    return false;
  }
  return true;
}

struct ChannelStatistics {
  int32_t peak;
  int32_t rms;
  int64_t sum_squares;
};

ChannelStatistics calculate_channel_statistics(size_t channel)
{
  int32_t peak = 0;
  int64_t sum_squares = 0;
  for (size_t frame = 0; frame < kFramesPerBuffer; ++frame) {
    const int16_t sample = g_frame[frame * kStereoChannelCount + channel];
    int32_t value = sample;
    if (value < 0) {
      value = -value;
    }
    if (value > peak) {
      peak = value;
    }
    sum_squares += static_cast<int64_t>(sample) * sample;
  }
  return {
      .peak = peak,
      .rms = static_cast<int32_t>(sqrtf(static_cast<float>(sum_squares) / kFramesPerBuffer)),
      .sum_squares = sum_squares,
  };
}

void print_frame_statistics()
{
  const ChannelStatistics left = calculate_channel_statistics(0);
  const ChannelStatistics right = calculate_channel_statistics(1);
  const int32_t combined_peak = max(left.peak, right.peak);
  const int32_t combined_rms = static_cast<int32_t>(sqrtf(
      static_cast<float>(left.sum_squares + right.sum_squares) / (kFramesPerBuffer * 2U)));

  Serial.printf("%lu left(MIC1) peak=%ld rms=%ld right(MIC2) peak=%ld rms=%ld both peak=%ld rms=%ld\n",
                millis(),
                static_cast<long>(left.peak), static_cast<long>(left.rms),
                static_cast<long>(right.peak), static_cast<long>(right.rms),
                static_cast<long>(combined_peak), static_cast<long>(combined_rms));
}

}  // namespace

void setup()
{
  Serial.begin(115200);
  delay(200);
  Wire.begin(7, 8, 100000);

  if (!init_i2s()) {
    return;
  }
  if (!init_codec()) {
    i2s_driver_uninstall(kI2sPort);
    return;
  }

  g_frame = static_cast<int16_t *>(malloc(kInterleavedSamples * sizeof(*g_frame)));
  if (g_frame == nullptr) {
    Serial.println("Microphone frame buffer allocation failed");
    es7210_adc_deinit();
    i2s_driver_uninstall(kI2sPort);
    return;
  }

  g_capture_ready = true;
  Serial.println("ES7210 ready; reading MIC1/MIC2 stereo frame statistics");
}

void loop()
{
  if (!g_capture_ready || g_frame == nullptr) {
    delay(1000);
    return;
  }

  size_t bytes_read = 0;
  const size_t requested = kInterleavedSamples * sizeof(*g_frame);
  const esp_err_t result = i2s_read(kI2sPort, g_frame, requested, &bytes_read, portMAX_DELAY);
  if (!check_result(result, "I2S read") || bytes_read != requested) {
    Serial.printf("I2S short read: requested %u bytes, received %u bytes\n", static_cast<unsigned>(requested), static_cast<unsigned>(bytes_read));
    delay(5);
    return;
  }
  print_frame_statistics();
}
