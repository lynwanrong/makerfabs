#include "no_audio_codec.h"

static const char *TAG = "no_codec";

typedef struct {
    i2s_chan_handle_t tx_handle;            /*!< I2S TX channel handle */
    i2s_chan_handle_t rx_handle;            /*!< I2S RX channel handle */
    no_audio_codec_config_t config;     /*!< configuration of the codec */

    int volume;                             /*!< volume of the codec */
    bool is_muted;                          /*!< mute status of the codec */

    bool duplex;
    bool input_enable;
    bool output_enable;

    int input_sample_rate;
    int output_sample_rate;
} no_codec_data_t;


static esp_err_t _simplex_init(const no_audio_codec_config_t *config, bsp_audio_handle_t self)
{
    esp_err_t ret = ESP_OK;

    no_codec_data_t *priv = (no_codec_data_t *)self->priv;
    priv->duplex = false;

    i2s_chan_config_t chan_cfg = {
        .id = (i2s_port_t)0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
        .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &priv->tx_handle, NULL), TAG, "Failed to create I2S channel");

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)priv->output_sample_rate,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
			#ifdef   I2S_HW_VERSION_2
				.ext_clk_freq_hz = 0,
			#endif

        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
            .ws_pol = false,
            .bit_shift = true,
            #ifdef   I2S_HW_VERSION_2
                .left_align = true,
                .big_endian = false,
                .bit_order_lsb = false
            #endif

        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = config->spk_bclk,
            .ws = config->spk_ws,
            .dout = config->spk_dout,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };
    ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(priv->tx_handle, &std_cfg), err, TAG, "Failed to init I2S TX channel");

    // Create a new channel for MIC
    chan_cfg.id = I2S_NUM_1;
    ESP_GOTO_ON_ERROR(i2s_new_channel(&chan_cfg, NULL, &priv->rx_handle), err, TAG, "Failed to create I2S channel");
    std_cfg.clk_cfg.sample_rate_hz = (uint32_t)config->input_sample_rate;
    std_cfg.gpio_cfg.bclk = config->mic_bclk;
    std_cfg.gpio_cfg.ws = config->mic_ws;
    std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
    std_cfg.gpio_cfg.din = config->mic_din;
    ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(priv->rx_handle, &std_cfg), err, TAG, "Failed to init I2S RX channel");
    ESP_LOGI(TAG, "Simplex channels created");

    return ret;

err:
    if (priv->rx_handle) {
        i2s_del_channel(priv->rx_handle);
        priv->rx_handle = NULL;
    }
    if (priv->tx_handle) {
        i2s_del_channel(priv->tx_handle);
        priv->tx_handle = NULL;
    } 
    return ret;
}

static esp_err_t _init(const no_audio_codec_config_t *config, bsp_audio_handle_t self)
{
    esp_err_t ret = ESP_OK;

    no_codec_data_t *priv = (no_codec_data_t *)self->priv;
    ESP_LOGI(TAG, "Initializing I2S for No-Codec device...");

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

    if (config->duplex) {
        ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &priv->tx_handle, &priv->rx_handle), TAG, "Failed to create I2S channel");
    } else {
        if (config->mic_din != GPIO_NUM_NC && config->mic_bclk != GPIO_NUM_NC && config->mic_ws != GPIO_NUM_NC) {
            ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, NULL, &priv->rx_handle), TAG, "Failed to create RX channel");
        }
        if (config->spk_dout != GPIO_NUM_NC && config->spk_bclk != GPIO_NUM_NC && config->spk_ws != GPIO_NUM_NC) {
            chan_cfg.id = I2S_NUM_1;
            ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &priv->tx_handle, NULL), TAG, "Failed to create TX channel");
        }
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(config->input_sample_rate),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(AUDIO_DATA_BIT_WIDTH, AUDIO_SLOT_MODE),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .invert_flags = { 0 }
        }
    };

    if (priv->tx_handle) {
        std_cfg.gpio_cfg.bclk = config->spk_bclk;
        std_cfg.gpio_cfg.ws = config->spk_ws;
        std_cfg.gpio_cfg.dout = config->spk_dout;
        std_cfg.gpio_cfg.din = I2S_GPIO_UNUSED;
        ret = i2s_channel_init_std_mode(priv->tx_handle, &std_cfg);
        ESP_RETURN_ON_ERROR(ret, TAG, "Failed to init I2S TX channel");
    }

    if (priv->rx_handle) { 
        std_cfg.gpio_cfg.bclk = config->mic_bclk;
        std_cfg.gpio_cfg.ws = config->mic_ws;
        std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
        std_cfg.gpio_cfg.din = config->mic_din;
        ret = i2s_channel_init_std_mode(priv->rx_handle, &std_cfg);
        ESP_RETURN_ON_ERROR(ret, TAG, "Failed to init I2S RX channel");
    }

    return ret;
}

static esp_err_t _deinit(bsp_audio_handle_t self)
{
    no_codec_data_t *priv = (no_codec_data_t *)self->priv;
    if (priv->tx_handle) {
        i2s_channel_disable(priv->tx_handle);
        i2s_del_channel(priv->tx_handle);
        priv->tx_handle = NULL;
    }
    if (priv->rx_handle) {
        i2s_channel_disable(priv->rx_handle);
        i2s_del_channel(priv->rx_handle);
        priv->rx_handle = NULL;
    }
    return ESP_OK;
}

static esp_err_t _mute(bsp_audio_handle_t self, bool mute)
{
    ESP_RETURN_ON_FALSE(self != NULL && self->priv != NULL, ESP_FAIL, TAG, "self or self->priv is NULL");
    no_codec_data_t *priv = (no_codec_data_t *)self->priv;
    if (mute) {
        int32_t buffer[128] = {0};
        for (int i = 0; i < 100; i++) {
            i2s_channel_write(priv->tx_handle, buffer, sizeof(buffer), NULL, portMAX_DELAY);
        }
    }
    return ESP_OK;
}

static esp_err_t _set_volume(bsp_audio_handle_t self, int volume) {
    no_codec_data_t *priv = (no_codec_data_t *)self->priv;
    priv->volume = volume;
    ESP_LOGI(TAG, "Software volume set to %d (Hardware has no volume control)", volume);
    // 因为没有硬件芯片，这里可能需要设置一个标志位，在 write 函数里通过软件算法缩放 PCM 数据
    return ESP_OK;
}

static esp_err_t _write(bsp_audio_handle_t self, void *src, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(self != NULL && self->priv != NULL, ESP_FAIL, TAG, "self or self->priv is NULL");

    no_codec_data_t *priv = (no_codec_data_t *)self->priv;
    ESP_RETURN_ON_FALSE(priv->tx_handle != NULL, ESP_FAIL, TAG, "TX channel is not initialized");

    if (priv->is_muted) {
        // 如果静音，写入静音数据
        int32_t buffer[128] = {0};
        size_t total_written = 0;
        while (total_written < len) {
            size_t to_write = (len - total_written) > sizeof(buffer) ? sizeof(buffer) : (len - total_written);
            size_t written = 0;
            i2s_channel_write(priv->tx_handle, buffer, to_write, &written, timeout_ms);
            total_written += written;
            if (written == 0) break; // 防止死循环
        }
        if (bytes_written) *bytes_written = total_written;
        return ESP_OK;
    }
    // 如果需要软件音量处理，在这里对 src 进行修改
    return i2s_channel_write(priv->tx_handle, src, len, bytes_written, timeout_ms);
}

static esp_err_t _set_format(bsp_audio_handle_t self, uint32_t sample_rate, uint32_t bits_per_sample, uint32_t channels)
{
    ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    esp_err_t ret = ESP_OK;

    no_codec_data_t *priv = (no_codec_data_t *)self->priv;
    ESP_RETURN_ON_FALSE(priv != NULL && priv->tx_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    ESP_RETURN_ON_FALSE(channels >= 1 && channels <= 2, ESP_ERR_INVALID_ARG, TAG, "Unsupported channel count %u", (unsigned)channels);
    ESP_RETURN_ON_FALSE(bits_per_sample == 16 || bits_per_sample == 24 || bits_per_sample == 32, ESP_ERR_INVALID_ARG, TAG, "Unsupported bit depth %u", (unsigned)bits_per_sample);
    ESP_RETURN_ON_FALSE(sample_rate >= 8000 && sample_rate <= 48000, ESP_ERR_INVALID_ARG, TAG, "Unsupported sample rate %u", (unsigned)sample_rate);

    i2s_std_config_t std_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG((i2s_data_bit_width_t)bits_per_sample, (i2s_slot_mode_t)channels),
    };

    ret |= i2s_channel_disable(priv->tx_handle);
    ret |= i2s_channel_reconfig_std_clock(priv->tx_handle, &std_config.clk_cfg);
    ret |= i2s_channel_reconfig_std_slot(priv->tx_handle, &std_config.slot_cfg);
    ret |= i2s_channel_enable(priv->tx_handle);

    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to set audio format");
    return ret;
}

// 3. 定义虚函数表 (Vtable)
static const audio_ops_t _ops = {
    // .init       = _init,
    .deinit     = _deinit,
    .write      = _write,
    .set_volume = _set_volume,
};

esp_err_t no_audio_codec_init(const no_audio_codec_config_t *config, bsp_audio_handle_t *ret_handle)
{
    ESP_RETURN_ON_FALSE(config != NULL && ret_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    esp_err_t ret = ESP_OK;

    bsp_audio_handle_t _handle = calloc(1, sizeof(struct bsp_audio_t));
    ESP_GOTO_ON_FALSE(_handle != NULL, ESP_ERR_NO_MEM, err, TAG, "Failed to allocate memory for audio handle");

    no_codec_data_t *priv = calloc(1, sizeof(no_codec_data_t));
    ESP_GOTO_ON_FALSE(priv != NULL, ESP_ERR_NO_MEM, err, TAG, "Failed to allocate memory for no_codec data");

    priv->config = *config;
    _handle->ops = &_ops;
    _handle->priv = priv;

    _init(config, _handle);
    *ret_handle = _handle;

    return ret;

err:
    if (_handle) free(_handle);
    return ret;
}