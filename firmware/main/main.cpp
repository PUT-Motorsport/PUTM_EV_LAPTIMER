#include "main.h"

#include "laptimer_task.h"
#include "sdcard_task.h"
#include "lcd_task.h"
#include "wifi_task.h"

#include "gpio.h"
#include "timer.h"

SemaphoreHandle_t config_mutex = xSemaphoreCreateMutex();

QueueHandle_t wifi_reset_queue = xQueueCreate(1, sizeof(Wifi_reset));

QueueHandle_t laptime_saved_queue_sd = xQueueCreate(LAPTIME_LIST_SIZE_LOCAL, sizeof(Laptime));

QueueHandle_t laptime_current_queue_lcd = xQueueCreate(1, sizeof(Laptime));
QueueHandle_t laptime_current_queue_wifi = xQueueCreate(1, sizeof(Laptime));

SemaphoreHandle_t laptime_lists_mutex = xSemaphoreCreateMutex();

QueueHandle_t laptime_status_queue_lcd = xQueueCreate(1, sizeof(bool[3]));
QueueHandle_t laptime_status_queue_wifi = xQueueCreate(1, sizeof(bool[3]));

QueueHandle_t ip_queue = xQueueCreate(1, sizeof(char[52]));
QueueHandle_t wifi_mode_queue = xQueueCreate(1, sizeof(wifi_mode_t));

Config config_main;

Laptime laptime_list_top[LAPTIME_LIST_SIZE_LOCAL];
Laptime laptime_list_last[LAPTIME_LIST_SIZE_LOCAL];
Laptime laptime_list_driver[DRIVER_MAX_COUNT];

bool sd_active_flag = false;

void Laptime::reset()
{

    this->time = 0;
    this->penalty_time = 0;
    this->doo_count = 0;
    this->oc_count = 0;
}

void Laptime::new_lap()
{
    this->count++;
    this->reset();
}
void Laptime::convert_string_full(char laptime_str[LAPTIME_STR_LENGTH], size_t size)
{
    if (laptime_str == NULL)
        return;

    if (this->time == 0)
    {
        snprintf(laptime_str, size, LAPTIME_STR_DEFAULT);
        return;
    }

    unsigned int mm = (this->time / 6000) % 60;
    unsigned int ss = (this->time / 100) % 60;
    unsigned int ms = this->time % 100;
    if (size == LAPTIME_STR_LENGTH)
        snprintf(laptime_str, size, "%02u, %02u:%02u:%02u",
                 this->count, mm, ss, ms);
}
void Laptime::convert_string_time(char laptime_str[LAPTIME_STR_LENGTH], size_t size)

{
    if (laptime_str == NULL)
        return;

    if (this->time == 0)
    {
        snprintf(laptime_str, size, "--:--.--");
        return;
    }

    unsigned int mm = (this->time / 6000) % 60;
    unsigned int ss = (this->time / 100) % 60;
    unsigned int ms = this->time % 100;
    if (size == LAPTIME_STR_LENGTH)
        snprintf(laptime_str, size, "%02u:%02u:%02u", mm, ss, ms);
}
void Laptime::convert_string_penalty(char laptime_str[PENALTY_TIME_STR_LENGTH], size_t size)
{
    if (laptime_str == NULL)
        return;

    if (this->time == 0)
    {
        snprintf(laptime_str, size, PENALTY_STR_DEFAULT);
        return;
    }

    unsigned int mm = (this->penalty_time / 6000) % 60;
    unsigned int ss = (this->penalty_time / 100) % 60;
    if (size >= 11)
    {
        snprintf(laptime_str, size, "+%02u:%02u", mm, ss);
    }
}

/**
 * @brief Main task initializes core peripherals and creates program tasks
 */
extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(gpio_init());
    ESP_ERROR_CHECK(timer_init());
    ESP_ERROR_CHECK(isr_init());
    // ESP_ERROR_CHECK(rtc_init());

    if (xSemaphoreTake(config_mutex, portMAX_DELAY))
    {
        ESP_ERROR_CHECK(system_set_time(config_main.time_set, config_main.date_set));
        xSemaphoreGive(config_mutex);
    }

    xTaskCreatePinnedToCore(sdcard_task, "SD_TASK", 4096, NULL, 0, NULL, 0);
    xTaskCreatePinnedToCore(laptimer_task, "LAPTIMER_TASK", 8192, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(lcd_task, "LCD_TASK", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(wifi_task, "WIFI_TASK", 4096, NULL, 1, NULL, 0);

    vTaskDelete(NULL);
}
