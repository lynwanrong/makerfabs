#pragma once

#include "bsp_audio.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    NO_CODEC_MODE_SIMPLEX,
    NO_CODEC_MODE_DUPLEX
} no_codec_mode_t;

typedef struct {
    no_codec_mode_t mode;
    
    int input_sample_rate;
    int output_sample_rate;
    
    gpio_num_t spk_bclk;
    gpio_num_t spk_ws;
    gpio_num_t spk_dout;
    
    gpio_num_t mic_bclk;
    gpio_num_t mic_ws;
    gpio_num_t mic_din;

    i2s_std_slot_mask_t spk_slot_mask;
    i2s_std_slot_mask_t mic_slot_mask;



    struct {
        uint8_t enable_mic :1;
        uint8_t enable_spk :1;
    } flag;
} bsp_audio_no_codec_config_t;

/**
 * @brief Initialize the No-Codec audio device
 *
 * @param[in]  config           Configuration structure
 * @param[out] ret_handle       Pointer to the returned audio handle
 * @return
 * - ESP_OK: Success
 */
esp_err_t no_audio_codec_simplex(const bsp_audio_no_codec_config_t *config, bsp_audio_handle_t *ret_handle);

#ifdef __cplusplus
}
#endif