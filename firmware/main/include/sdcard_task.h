#pragma once

#include "main.h"

#define SD_BUFFER_SIZE 256

extern bool sd_active_flag;
extern bool sd_fail_flag;

void sdcard_task(void *args);
