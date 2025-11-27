#pragma once

#include "freertos/idf_additions.h"

#define LAPTIME_LIST_SIZE_LOCAL 50
#define LAPTIME_LIST_SIZE_LCD 5
#define LAPTIME_MIN 500
#define LAPTIME_STRING_LENGTH 13
#define LAPTIME_STRING_LENGTH_UART 14

extern QueueHandle_t sd_queue;
extern QueueHandle_t sd_reinit_semaphore;
extern QueueHandle_t lcd_laptime_current_queue;
extern QueueHandle_t lcd_laptime_lists_semaphore;
extern QueueHandle_t lcd_laptime_status_queue;

extern char lcd_list_buffer[2][LAPTIME_LIST_SIZE_LOCAL][LAPTIME_STRING_LENGTH];
