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

// Initialize main timer used to measure lap time
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

// Initialize I2C RV3028 rtc
esp_err_t rtc_init()
{
    Wire.begin(CONFIG_RTC_I2C_SDA, CONFIG_RTC_I2C_SCL, 40000);
    rtc.begin(Wire);
    return ESP_OK;
}

// Set time and date for I2C RV3028 rtc
esp_err_t rtc_set_time(char time_now[TIMEOFDAY_STR_LENGTH], char date_now[DATE_STR_LENGTH])
{
    char iso8601[20] = "\0";
    snprintf(iso8601, sizeof(iso8601), "%sT%s", date_now, time_now);
    rtc.setDateTimeFromISO8601(iso8601);
    return ESP_OK;
}

// Get time and date from I2C RV3028 rtc
esp_err_t rtc_get_time(char time_now[TIMEOFDAY_STR_LENGTH], char date_now[DATE_STR_LENGTH])
{
    char *iso8601 = rtc.getCurrentDateTime();
    char *date_buf = strtok(iso8601, "T");
    char *time_buf = strtok(NULL, "T");
    if (date_buf == NULL || time_buf == NULL)
        return ESP_FAIL;
    snprintf(time_now, TIMEOFDAY_STR_LENGTH, time_buf);
    snprintf(date_now, DATE_STR_LENGTH, date_buf);
    return ESP_OK;
}

// Get time from main lap timer measured in 1/100s
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

// Reset main lap timer
esp_err_t timer_reset(gptimer_handle_t timer_handle)
{
    return gptimer_set_raw_count(timer_handle, 0);
}

// Set system time and date
esp_err_t system_set_time(char time_now[TIMEOFDAY_STR_LENGTH], char date_now[DATE_STR_LENGTH])
{
    if (time_now == NULL || date_now == NULL)
        return ESP_FAIL;

    struct tm t = {0};
    struct timeval tv = {0};

    if (strptime(date_now, "%Y-%m-%d", &t) == NULL)
        return ESP_FAIL;
    if (strptime(time_now, "%H:%M:%S", &t) == NULL)
        return ESP_FAIL;

    t.tm_isdst = -1;
    tv.tv_sec = mktime(&t);

    if (settimeofday(&tv, NULL) != 0)
        return ESP_FAIL;
    return ESP_OK;
}

// Get system time and date
esp_err_t system_get_time(char time_now[TIMEOFDAY_STR_LENGTH], char date_now[DATE_STR_LENGTH])
{
    if (time_now == NULL || date_now == NULL)
        return ESP_FAIL;
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(time_now, TIMEOFDAY_STR_LENGTH, "%H:%M:%S", &timeinfo);
    strftime(date_now, DATE_STR_LENGTH, "%Y-%m-%d", &timeinfo);
    return ESP_OK;
}
