#include "timer.h"

#include "driver/gptimer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "soc/clk_tree_defs.h"
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

static const char *TAG = "TIMER";

gptimer_handle_t laptime_timer;

RV3028C7 rtc;

esp_err_t timer_init()
{
    gptimer_config_t timer_config = {.clk_src = GPTIMER_CLK_SRC_DEFAULT,
                                     .direction = GPTIMER_COUNT_UP,
                                     .resolution_hz = 1 * 10000};
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &laptime_timer));
    ESP_ERROR_CHECK(gptimer_enable(laptime_timer));
    ESP_ERROR_CHECK(gptimer_start(laptime_timer));

    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    ESP_LOGI("TIMER", "INIT OK");
    return ESP_OK;
}

esp_err_t rtc_init()
{
    Wire.begin(CONFIG_RTC_I2C_SDA, CONFIG_RTC_I2C_SCL, 40000);
    rtc.begin(Wire);
    return ESP_OK;
}

uint64_t timer_get_time(gptimer_handle_t timer_handle)
{
    uint64_t timer_value = 0;
    uint32_t timer_res = 0;
    uint64_t timer_time_10ms = 0;
    ESP_ERROR_CHECK(gptimer_get_raw_count(timer_handle, &timer_value));
    ESP_ERROR_CHECK(gptimer_get_resolution(timer_handle, &timer_res));
    timer_time_10ms = (timer_value * 100) / timer_res;
    return timer_time_10ms;
}

esp_err_t timer_reset(gptimer_handle_t timer_handle)
{
    return gptimer_set_raw_count(timer_handle, 0);
}

esp_err_t system_set_time(char time_buf[TIMEOFDAY_STR_LENGTH], char date_buf[DATE_STR_LENGTH])
{
    if (time_buf == NULL || date_buf == NULL)
        return ESP_FAIL;

    struct tm t = {0};
    struct timeval tv = {0};

    if (strptime(date_buf, "%d/%m/%Y", &t) == NULL)
        return ESP_FAIL;
    if (strptime(time_buf, "%H:%M:%S", &t) == NULL)
        return ESP_FAIL;

    t.tm_isdst = -1;
    tv.tv_sec = mktime(&t);

    if (settimeofday(&tv, NULL) != 0)
        return ESP_FAIL;
    return ESP_OK;
}

esp_err_t system_get_time(char time_buf[TIMEOFDAY_STR_LENGTH], char date_buf[DATE_STR_LENGTH])
{
    if (time_buf == NULL || date_buf == NULL)
        return ESP_FAIL;
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(time_buf, TIMEOFDAY_STR_LENGTH, "%H:%M:%S", &timeinfo);
    strftime(date_buf, DATE_STR_LENGTH, "%d/%m/%Y", &timeinfo);
    return ESP_OK;
}
