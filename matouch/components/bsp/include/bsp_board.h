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

#if CONFIG_AUDIO_ENABLE
#include "../audio/codecs/no_audio_codec.h"
#include "../audio/bsp_audio.h"
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

