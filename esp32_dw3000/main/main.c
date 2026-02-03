#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dw3000_hw.h"
#include "dwhw.h"
#include "dwmac.h"
#include "dwphy.h"
#include "dwproto.h"
#include "ranging.h"

// static void twr_done_cb(uint64_t src, uint64_t dst, uint16_t dist,
// 						 uint16_t num)
// {
//   ESP_LOGI("main", "TWR Done %04X: %d cm", (uint16_t)dst, dist);
// }
void app_main(void)
{
    // dw3000_hw_init();
    // dw3000_hw_reset();
    dw3000_hw_init_interrupt();

    // dwhw_init();
    // dwphy_config();
    // dwphy_set_antenna_delay(DWPHY_ANTENNA_DELAY);
    // dwmac_init(1111, 0x0002, dwprot_rx_handler, NULL, NULL);
    // dwmac_set_frame_filter();
    // twr_init(1000, true);
    // // twr_set_observer(twr_done_cb);
    // // twr_start(0x0002);
    // dwmac_set_rx_reenable(true);

    // while (1) {
    //     vTaskDelay(1000);
    // }

    // twr_set_observer(twr_done_cb);
    // two way ranging to 0x0001
    // twr_start(0x0001);

    // if (dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf) != DWT_SUCCESS)
    // {
    //     ESP_LOGI("main", "DWT Probe Failed!");
    //     return;
    // }

    // // 4. 初始化驱动 (这一步会分配内部内存，并安全地读取 Dev ID)
    // if (dwt_initialise(DWT_LOADLDO) != DWT_SUCCESS)
    // {
    //     printf("DWT Initialise Failed!\n");
    //     ESP_LOGI("main", "DWT Initialise Failed!");
    //     return;
    // }

    // uint32_t dev_id = dwt_readdevid();
    // // LOG_INF("DEVID %x", dev_id);
    // ESP_LOGI("main", "DEVID = 0x%08lx", dev_id);
}