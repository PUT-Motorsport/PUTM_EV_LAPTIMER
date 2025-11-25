#pragma once

#include "sd_protocol_types.h"
#include "main.h"

void sdcard_task(void *args);

// laptimer sd looger initialization
int sdcard_init(sdmmc_card_t **card_pointer);

// save recorded laptime
int sdcard_save_laptime(char laptime_saved_str[13]);
