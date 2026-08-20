#pragma once

#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "esp_dsp.h"
#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"

namespace esp_brookesia::apps
{
    class SpecAnalyzer : public systems::phone::App
    {
    public:
        static SpecAnalyzer *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
        ~SpecAnalyzer();

        using systems::phone::App::endRecordResource;
        using systems::phone::App::startRecordResource;

    protected:
        SpecAnalyzer(bool use_status_bar, bool use_navigation_bar);

        bool run(void) override;
        bool back(void) override;
        bool close(void) override;
        bool init(void) override;
        bool deinit(void) override;
        bool pause(void) override;
        bool resume(void) override;

    private:
        static SpecAnalyzer *_instance;

        // Capture the complete ES7210 frame in its board-observed
        // [MIC1, MIC3 (echo), MIC2, MIC4] order. The two front microphones are
        // extracted in software so slot filtering cannot change the DMA layout.
        static constexpr uint16_t N_SAMPLES = 1024;
        static constexpr uint16_t TDM_SLOT_COUNT = 4;
        static constexpr uint16_t CAPTURE_TDM_SLOT_MASK = BSP_EXTRA_ES7210_TDM_ALL_SLOTS_MASK;
        static constexpr uint16_t CAPTURE_CHANNELS = TDM_SLOT_COUNT;
        static constexpr uint16_t MIC_COUNT = 2;
        static constexpr uint16_t PHYSICAL_MIC1_TDM_SLOT_INDEX = 0;
        static constexpr uint16_t PHYSICAL_MIC2_TDM_SLOT_INDEX = 2;
        static constexpr uint16_t MIC_GAIN_MASK =
            BSP_EXTRA_ES7210_PHYSICAL_FRONT_MIC_MASK;
        static constexpr uint16_t STRIPE_COUNT = 48;
        static constexpr uint16_t CANVAS_WIDTH = BSP_LCD_H_RES;
        static constexpr uint16_t CANVAS_HEIGHT = (BSP_LCD_V_RES >= 720) ? 360 : (BSP_LCD_V_RES / 2);

        lv_obj_t *_canvas;
        lv_obj_t *_mic_label;
        lv_timer_t *_timer;
        TaskHandle_t _audio_task_handle;
        bool _audio_task_running;

        // Dual-microphone visualization buffers.
        __attribute__((aligned(16))) int16_t _raw_data[N_SAMPLES * CAPTURE_CHANNELS]; // Packed microphone samples
        __attribute__((aligned(16))) float _audio_buffer[MIC_COUNT][N_SAMPLES];   // Normalized microphone samples
        __attribute__((aligned(16))) float _wind[N_SAMPLES];                      // Shared Hann window
        __attribute__((aligned(16))) float _fft_buffer[MIC_COUNT][N_SAMPLES * 2]; // Complex FFT input
        __attribute__((aligned(16))) float _spectrum[MIC_COUNT][N_SAMPLES / 2];   // Spectrum in dB
        float _display_spectrum[MIC_COUNT][STRIPE_COUNT];                         // Mapped spectrum for bars
        float _peak[MIC_COUNT][STRIPE_COUNT];                                     // Peak marker position
        float _smooth_spectrum[MIC_COUNT][STRIPE_COUNT];                          // Smoothed display values

        lv_color_t *_draw_buf; // PSRAM-preferred canvas draw buffer

        static void audio_fft_task(void *pvParameters);
        static void timer_cb(lv_timer_t *timer);
    };
}
