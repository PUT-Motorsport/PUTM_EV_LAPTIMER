#pragma once

#include "main.h"

#include "driver/gptimer_types.h"
#include "RV3028C7.h"

esp_err_t rtc_init();

esp_err_t rtc_set_time(char time_now[TIMEOFDAY_STR_LENGTH], char date_now[DATE_STR_LENGTH]);

esp_err_t rtc_get_time(char time_now[TIMEOFDAY_STR_LENGTH], char date_now[DATE_STR_LENGTH]);

#ifdef __cplusplus
extern "C"
{
#endif

    extern gptimer_handle_t laptime_timer;

    esp_err_t timer_init();

    uint64_t timer_get_time(gptimer_handle_t timer_handle);

    esp_err_t timer_reset(gptimer_handle_t timer_handle);

    esp_err_t system_set_time(char time_now[TIMEOFDAY_STR_LENGTH], char date_now[DATE_STR_LENGTH]);

    esp_err_t system_get_time(char time_now[TIMEOFDAY_STR_LENGTH], char date_now[DATE_STR_LENGTH]);

#ifdef __cplusplus
}
#endif