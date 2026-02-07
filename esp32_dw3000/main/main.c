#include <stdio.h>

#include "dw3000.h"

#include "examples_defines.h"

#include <esp_log.h>
#include <esp_task_wdt.h>

void app_main(void)
{
    // esp_task_wdt_config_t _config = {
    //     .timeout_ms = 5000,
    //     .idle_core_mask = (1 << 0),
    //     .trigger_panic = true,
    // };
    // esp_task_wdt_add(NULL);

    dw3000_hw_init();

    build_examples();

    // dw3000_hw_init();
    // dw3000_hw_reset();

    // dw3000_spi_speed_fast();

    // if (dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf) != DWT_SUCCESS) {
    //     ESP_LOGI("main", "DWT Probe Failed!");
    //     return ;
    // }

    // // 4. 初始化驱动 (这一步会分配内部内存，并安全地读取 Dev ID)
    // if (dwt_initialise(DWT_LOADLDO) != DWT_SUCCESS) {
    //     ESP_LOGI("main", "DWT Initialise Failed!");
    //     return ;
    // }

    // uint32_t dev_id = dwt_readdevid();
    // if (dev_id != (uint32_t)DWT_DW3000_DEV_ID) {
    //     ESP_LOGI("main", "error dev_id");
    //     return ;
    // }

    // ESP_LOGI("main", "dev_id = 0x%"PRIX32"", dev_id);
}