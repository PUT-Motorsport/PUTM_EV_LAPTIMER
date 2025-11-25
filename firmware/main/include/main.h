#pragma once

#include "freertos/idf_additions.h"

#define LAPTIME_LIST_SIZE 50
#define LAPTIME_MIN 500
#define LAPTIME_LENGTH 13

extern QueueHandle_t sd_queue;
extern QueueHandle_t sd_reinit_semaphore;
extern QueueHandle_t lcd_laptime_current_queue;
extern QueueHandle_t lcd_laptime_lists_queue;
extern QueueHandle_t lcd_laptime_status_queue;
