#include "main.h"

#include "laptimer_task.h"
#include "sdcard_task.h"
#include "lcd_task.h"
#include "wifi_task.h"

#include "gpio.h"
#include "timer.h"

SemaphoreHandle_t config_mutex = xSemaphoreCreateMutex();

SemaphoreHandle_t wifi_reset_semaphore = xSemaphoreCreateBinary();

QueueHandle_t laptime_saved_queue_sd = xQueueCreate(1, sizeof(Laptime));

QueueHandle_t laptime_current_queue_lcd = xQueueCreate(1, sizeof(Laptime));
QueueHandle_t laptime_current_queue_wifi = xQueueCreate(1, sizeof(Laptime));

SemaphoreHandle_t laptime_lists_mutex = xSemaphoreCreateMutex();

QueueHandle_t laptime_status_queue_lcd = xQueueCreate(1, sizeof(bool[3]));
QueueHandle_t laptime_status_queue_wifi = xQueueCreate(1, sizeof(bool[3]));

Config config_main;

Laptime laptime_list_top[LAPTIME_LIST_SIZE_LOCAL] = {0};
Laptime laptime_list_last[LAPTIME_LIST_SIZE_LOCAL] = {0};
Laptime laptime_list_driver[DRIVER_MAX_COUNT] = {0};

volatile bool stop_flag = true;
bool sd_active_flag = false;

/**
 * @brief Main task initializes core peripherals and creates program tasks
 */
extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(gpio_init());
    ESP_ERROR_CHECK(timer_init());
    ESP_ERROR_CHECK(isr_init());

    xTaskCreatePinnedToCore(sdcard_task, "SD_TASK", 4096, NULL, 0, NULL, 0);
    xTaskCreatePinnedToCore(laptimer_task, "LAPTIMER_TASK", 8192, NULL, 2,
                            NULL, 1);
    xTaskCreatePinnedToCore(lcd_task, "LCD_TASK", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(wifi_task, "WIFI_TASK", 4096, NULL, 1, NULL, 0);

    vTaskDelete(NULL);
}
