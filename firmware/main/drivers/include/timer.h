#pragma once

#include "driver/gptimer_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

extern gptimer_handle_t laptime_timer;

esp_err_t timer_init();

uint64_t timer_get_time(gptimer_handle_t timer_handle);

esp_err_t timer_reset(gptimer_handle_t timer_handle);

#ifdef __cplusplus
}
#endif