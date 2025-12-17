#pragma once

#include "bsp_config.h"


typedef struct board_t* board_handle_t;

struct board_t{
    bsp_pcf85063a_handle_t bsp_pcf85063a_handle;
};

extern board_handle_t board;


void board_init(void);