#include "no_codec.h"


// 检查对象有效性
#define CHECK_VALID_HANDLE(h, ret_val) \
    do { \
        if ((h) == NULL || (h)->priv == NULL) { \
            ESP_LOGE(TAG, "Invalid parameter"); \
            return (ret_val); \
        } \
    } while(0)

// 检查是否具备发送能力
#define CHECK_TX_SUPPORT(priv, ret_val) \
    do { \
        if ((priv)->tx_handle == NULL) { \
            ESP_LOGE(TAG, "TX not supported"); \
            return (ret_val); \
        } \
    } while(0)

// 检查是否具备接收能力
#define CHECK_RX_SUPPORT(priv, ret_val) \
    do { \
        if ((priv)->rx_handle == NULL) { \
            ESP_LOGE(TAG, "RX not supported"); \
            return (ret_val); \
        } \
    } while(0)

#define CHECK_ARG(cond)                                     \
    ESP_RETURN_ON_FALSE(cond, ESP_ERR_INVALID_ARG, TAG, "Invalid argument: " #cond)

// 场景 2: 专门检查是否已初始化 (自动返回 ESP_ERR_INVALID_STATE)
#define CHECK_INIT(priv)                                    \
    ESP_RETURN_ON_FALSE(priv, ESP_ERR_INVALID_STATE, TAG, "Driver not initialized")

// 场景 3: 专门检查内存分配 (自动返回 ESP_ERR_NO_MEM)
#define CHECK_MEM(ptr)                                      \
    ESP_RETURN_ON_FALSE(ptr, ESP_ERR_NO_MEM, TAG, "Memory allocation failed")

// 场景 4: 专门检查 TX 能力 (自动返回 ESP_ERR_NOT_SUPPORTED)
#define CHECK_TX(priv)                                      \
    ESP_RETURN_ON_FALSE(priv->tx_handle, ESP_ERR_NOT_SUPPORTED, TAG, "TX not supported (RX only)")

static const char *TAG = "no_codec";

typedef struct {
    i2s_chan_handle_t tx_handle;            /*!< I2S TX channel handle */
    i2s_chan_handle_t rx_handle;            /*!< I2S RX channel handle */
    bsp_audio_no_codec_config_t config;     /*!< configuration of the codec */
    int volume;                             /*!< volume of the codec */
} no_codec_data_t;

static esp_err_t _init(bsp_audio_handle_t self)
{
    esp_err_t ret = ESP_OK;

    no_codec_data_t *priv = (no_codec_data_t *)self->priv;
    ESP_LOGI(TAG, "Initializing I2S for No-Codec device...");

        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

    // if duplex mode must enable both tx and rx
    if (priv->config.mode == NO_CODEC_MODE_DUPLEX) ret = i2s_new_channel(&chan_cfg, &priv->tx_handle, &priv->rx_handle);
    // if simplex mode enable either tx or rx
    else {
        if (priv->config.flag.enable_spk) ret = i2s_new_channel(&chan_cfg, &priv->tx_handle, NULL);
        if (priv->config.flag.enable_mic) ret = i2s_new_channel(&chan_cfg, NULL, &priv->rx_handle);
    }
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to create I2S channels");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(priv->config.sample_rate),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(AUDIO_DATA_BIT_WIDTH, AUDIO_SLOT_MODE),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .invert_flags = { 0 }
        }
    };

    if (priv->tx_handle) {
        std_cfg.gpio_cfg.bclk = priv->config.spk_bclk;
        std_cfg.gpio_cfg.ws = priv->config.spk_ws;
        std_cfg.gpio_cfg.dout = priv->config.spk_dout;
        std_cfg.gpio_cfg.din = I2S_GPIO_UNUSED;
        ret = i2s_channel_init_std_mode(priv->tx_handle, &std_cfg);
        ESP_RETURN_ON_ERROR(ret, TAG, "Failed to init I2S TX channel");
    }

    if (priv->rx_handle) { 
        std_cfg.gpio_cfg.bclk = priv->config.mic_bclk;
        std_cfg.gpio_cfg.ws = priv->config.mic_ws;
        std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
        std_cfg.gpio_cfg.din = priv->config.mic_din;
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
    ESP_ERROR_ON_FALSE(self != NULL && self->priv != NULL, ESP_FAIL, "self or self->priv is NULL");
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
    no_codec_data_t *data = (no_codec_data_t *)self->user_data;
    data->volume = volume;
    ESP_LOGI(TAG, "Software volume set to %d (Hardware has no volume control)", volume);
    // 因为没有硬件芯片，这里可能需要设置一个标志位，在 write 函数里通过软件算法缩放 PCM 数据
    return ESP_OK;
}

static esp_err_t _write(bsp_audio_handle_t self, void *src, size_t len, size_t *bytes_written, uint32_t timeout_ms) {
    no_codec_data_t *data = (no_codec_data_t *)self->user_data;
    // 如果需要软件音量处理，在这里对 src 进行修改
    return i2s_channel_write(data->tx_handle, src, len, bytes_written, timeout_ms);
}

static esp_err_t _set_format(bsp_audio_handle_t self, uint32_t sample_rate, uint32_t bits_per_sample, uint32_t channels)
{
    ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    CHECK_VALID_HANDLE(self, ESP_ERR_INVALID_ARG);

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
    .init       = _init,
    .deinit     = _deinit,
    .write      = _write,
    .set_volume = _set_volume,
};

esp_err_t bsp_audio_new_no_codec(const bsp_audio_no_codec_config_t *config, bsp_audio_handle_t *ret_handle)
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

    _handle->ops->init(_handle);
    *ret_handle = _handle;

    return ret;

err:
    if (_handle) free(_handle);
    return ret;
}