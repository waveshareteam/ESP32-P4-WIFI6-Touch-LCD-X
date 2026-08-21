/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 *
 * ES8311 melody player for the Waveshare ESP32-P4-WIFI6-Touch-LCD-X.
 *
 * Plays the opening of "Für Elise" (Beethoven) through the ES8311 codec and
 * the on-board speaker. The sketch synthesizes each note with a sine wave and
 * a simple decay envelope.
 *
 * I2C: SDA=GPIO7, SCL=GPIO8 (ES8311 address 0x18).
 * I2S: MCLK=GPIO13, BCLK=GPIO12, LRCK=GPIO10, DOUT=GPIO9.
 * Speaker amplifier: GPIO53 (high = enabled).
 */
#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>

#include "es8311.h"

namespace {

constexpr int kPowerAmplifierPin = 53;
constexpr int kI2sMclkPin = 13;
constexpr int kI2sBclkPin = 12;
constexpr int kI2sLrckPin = 10;
constexpr int kI2sDataOutPin = 9;
constexpr uint32_t kSampleRate = 16000;
constexpr size_t kToneFrame = 64;
constexpr int16_t kNoteAmplitude = 8000;
// Keep the Arduino demo at the repository's conservative playback level.
constexpr int kMelodyVolume = 60;
constexpr i2s_port_t kI2sPort = I2S_NUM_0;

struct Note {
  float frequency;
  uint16_t duration_ms;
};

// Für Elise, opening section (A). A frequency of zero would be a rest.
constexpr Note kFurElise[] = {
    {659.25f, 350}, {622.25f, 350}, {659.25f, 350}, {622.25f, 350}, {659.25f, 350},
    {493.88f, 350}, {587.33f, 350}, {523.25f, 350}, {440.00f, 700},
    {261.63f, 350}, {329.63f, 350}, {440.00f, 350}, {493.88f, 700},
    {329.63f, 350}, {415.30f, 350}, {493.88f, 350}, {523.25f, 700},
    {329.63f, 350}, {659.25f, 350}, {622.25f, 350}, {659.25f, 350}, {622.25f, 350},
    {659.25f, 350}, {493.88f, 350}, {587.33f, 350}, {523.25f, 350}, {440.00f, 900},
};

es8311_handle_t g_codec = nullptr;
bool g_audio_ready = false;

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
  g_codec = es8311_create(0, ES8311_ADDRRES_0);
  if (g_codec == nullptr) {
    Serial.println("ES8311 create failed");
    return false;
  }

  const es8311_clock_config_t clock_config = {
      .mclk_inverted = false,
      .sclk_inverted = false,
      .mclk_from_mclk_pin = true,
      .mclk_frequency = static_cast<int>(kSampleRate * 256U),
      .sample_frequency = static_cast<int>(kSampleRate),
  };
  if (!check_result(es8311_init(g_codec, &clock_config, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16), "ES8311 init") ||
      !check_result(es8311_sample_frequency_config(g_codec, clock_config.mclk_frequency, clock_config.sample_frequency), "ES8311 sample rate") ||
      !check_result(es8311_microphone_config(g_codec, false), "ES8311 microphone mode") ||
      !check_result(es8311_voice_volume_set(g_codec, kMelodyVolume, nullptr), "ES8311 volume")) {
    es8311_delete(g_codec);
    g_codec = nullptr;
    return false;
  }
  return true;
}

bool play_note(float frequency, uint32_t duration_ms)
{
  const uint32_t sample_count = static_cast<uint32_t>((static_cast<uint64_t>(kSampleRate) * duration_ms) / 1000U);
  uint32_t samples_played = 0;
  float phase = 0.0f;
  const float phase_step = frequency * 2.0f * static_cast<float>(PI) / static_cast<float>(kSampleRate);
  int16_t frame[kToneFrame * 2] = {};

  while (samples_played < sample_count) {
    const uint32_t chunk = min(sample_count - samples_played, static_cast<uint32_t>(kToneFrame));
    for (uint32_t i = 0; i < chunk; ++i) {
      const float progress = static_cast<float>(samples_played + i) / static_cast<float>(sample_count);
      const float envelope = progress < 0.15f ? progress / 0.15f
                                              : 1.0f - (progress - 0.15f) / 0.85f;
      const int16_t sample = static_cast<int16_t>(sinf(phase) * kNoteAmplitude * envelope);
      frame[i * 2] = sample;
      frame[i * 2 + 1] = sample;
      phase += phase_step;
      if (phase > 2.0f * static_cast<float>(PI)) {
        phase -= 2.0f * static_cast<float>(PI);
      }
    }

    size_t written = 0;
    const size_t requested = static_cast<size_t>(chunk) * 2U * sizeof(int16_t);
    if (!check_result(i2s_write(kI2sPort, frame, requested, &written, portMAX_DELAY), "I2S write") || written != requested) {
      Serial.printf("I2S short write: requested %u bytes, wrote %u bytes\n", static_cast<unsigned>(requested), static_cast<unsigned>(written));
      return false;
    }
    samples_played += chunk;
  }
  return true;
}

bool init_i2s()
{
  const i2s_config_t config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
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
      .data_out_num = kI2sDataOutPin,
      .data_in_num = I2S_PIN_NO_CHANGE,
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

void stop_audio()
{
  g_audio_ready = false;
  digitalWrite(kPowerAmplifierPin, LOW);
  if (g_codec != nullptr) {
    (void)es8311_voice_mute(g_codec, true);
    es8311_delete(g_codec);
    g_codec = nullptr;
  }
  i2s_driver_uninstall(kI2sPort);
}

}  // namespace

void setup()
{
  Serial.begin(115200);
  delay(200);

  pinMode(kPowerAmplifierPin, OUTPUT);
  digitalWrite(kPowerAmplifierPin, HIGH);
  Wire.begin(7, 8, 100000);

  if (!init_i2s()) {
    digitalWrite(kPowerAmplifierPin, LOW);
    return;
  }
  if (!init_codec()) {
    i2s_driver_uninstall(kI2sPort);
    digitalWrite(kPowerAmplifierPin, LOW);
    return;
  }

  g_audio_ready = true;
  Serial.println("ES8311 ready; playing Fuer Elise");
}

void loop()
{
  if (!g_audio_ready) {
    delay(1000);
    return;
  }

  for (const Note &note : kFurElise) {
    if (!play_note(note.frequency, note.duration_ms)) {
      Serial.println("Audio playback stopped after an I2S error");
      stop_audio();
      return;
    }
    delay(60);
  }
  delay(1200);
}
