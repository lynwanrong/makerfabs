#pragma once

#include "bsp_config.h"


typedef struct board_t* board_handle_t;

struct board_t{
};

extern board_handle_t board;


void board_init(void);