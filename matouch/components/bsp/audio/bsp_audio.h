#pragma once

#include "bsp_board.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bsp_audio_t *bsp_audio_handle_t;

/**
 * @brief 音频设备接口定义 (Vtable)
 * 所有的具体驱动（NoCodec, ES8311, etc.）都要实现这组函数
 */
typedef struct {
    // 初始化/去初始化
    esp_err_t (*init)(bsp_audio_handle_t self);
    esp_err_t (*deinit)(bsp_audio_handle_t self);

    // 控制接口
    esp_err_t (*set_volume)(bsp_audio_handle_t self, int volume); // 0-100
    esp_err_t (*get_volume)(bsp_audio_handle_t self, int *volume);
    esp_err_t (*mute)(bsp_audio_handle_t self, bool mute);

    // 数据流接口 (通常是对 I2S 的封装)
    esp_err_t (*write)(bsp_audio_handle_t self, void *src, size_t len, size_t *bytes_written, uint32_t timeout_ms);
    int       (*read)(bsp_audio_handle_t self, void *dest, size_t len, uint32_t timeout_ms);
    
    // 格式配置
    esp_err_t (*set_format)(bsp_audio_handle_t self, uint32_t sample_rate, uint32_t bits_per_sample, uint32_t channels);
} audio_ops_t;

/**
 * @brief 音频设备基类
 * 类似于 C++ 的 class AudioDevice { AudioOps* vptr; void* priv; }
 */
struct bsp_audio_t {
    const audio_ops_t *ops;  // 虚函数表指针
    void *priv;         // 私有数据指针 (指向具体的 no_codec_t 或 es8311_t)
};

/* =======================================================
 * 公共 API 封装 (Helper functions)
 * 上层应用调用这些函数，它们会自动跳转到 ops->func
 * ======================================================= */
static inline esp_err_t audio_hal_set_volume(bsp_audio_handle_t dev, int vol) {
    if (dev && dev->ops && dev->ops->set_volume) return dev->ops->set_volume(dev, vol);
    return ESP_ERR_NOT_SUPPORTED;
}

static inline esp_err_t audio_hal_write(bsp_audio_handle_t dev, void *src, size_t len, size_t *written, uint32_t timeout) {
    if (dev && dev->ops && dev->ops->write) return dev->ops->write(dev, src, len, written, timeout);
    return ESP_FAIL;
}

// ... 其他 helper 函数类似 ...

#ifdef __cplusplus
}
#endif