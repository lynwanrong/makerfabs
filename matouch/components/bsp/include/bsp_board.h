#include <esp_err.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <driver/i2c_master.h>
#include <driver/i2s_std.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <esp_check.h>
#include <esp_heap_caps.h>
#include <time.h>

#define AUDIO_DATA_BIT_WIDTH        (16)
#define AUDIO_SLOT_MODE             (1)
#define AUDIO_SAMPLES               (1024)

#define I2S_HW_VERSION_2


#if CONFIG_AUDIO_ENABLE
#include "../audio/no_audio_codec.h"
#include "../audio/bsp_wav.h"
#endif


#if CONFIG_SPIFFS_ENABLE
#include "../spiffs/bsp_spiffs.h"
#endif

#if CONFIG_QMI8658_ENABLE
#include "../qmi8658/bsp_qmi8658.h"
#endif

#if CONFIG_SDCARD_ENABLE
#include "../sdcard/bsp_sdcard.h"
#endif

#if CONFIG_PCF85063A_ENABLE
#include "../pcf85063a/bsp_pcf85063a.h"
#endif

#if CONFIG_PCF8563_ENABLE
#include "../pcf8563/bsp_pcf8563.h"
#endif

