#include "port.h"
#include "dw3000_spi.h"
#include "dw3000_hw.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

void port_set_dw_ic_spi_fastrate(void)
{
    dw3000_spi_speed_fast();
}


void reset_DWIC(void)
{
    dw3000_hw_reset();
}

void Sleep(uint32_t delay_ms)
{
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}


void port_task_yield(void)
{
    taskYIELD();
}

void port_esp_task_wdt_reset()
{
    esp_task_wdt_reset();
}