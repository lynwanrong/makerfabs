#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "log.h"


#define DWT_READ_OTP_ALL 0x00


void reset_DWIC(void);

/******************************************
*               SPI速率配置函数
*******************************************/
void port_set_dw_ic_spi_slowrate(void);
void port_set_dw_ic_spi_fastrate(void);

void Sleep(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif