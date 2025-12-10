#include "main.h"

#include "laptimer_task.h"
#include "sdcard_task.h"
#include "lcd_task.h"
#include "wifi_task.h"

#include "gpio.h"
#include "timer.h"

QueueHandle_t sd_queue = xQueueCreate(LAPTIME_LIST_SIZE_LOCAL, sizeof(char[LAPTIME_STR_LENGTH]));

QueueHandle_t lcd_laptime_current_queue = xQueueCreate(1, sizeof(char[LAPTIME_STR_LENGTH]));
QueueHandle_t lcd_laptime_penalty_semaphore = xSemaphoreCreateBinary();
QueueHandle_t lcd_laptime_lists_semaphore = xSemaphoreCreateBinary();
QueueHandle_t lcd_laptime_status_semaphore = xSemaphoreCreateBinary();

QueueHandle_t wifi_laptime_current_queue = xQueueCreate(1, sizeof(char[LAPTIME_STR_LENGTH]));
QueueHandle_t wifi_laptime_penalty_semaphore = xSemaphoreCreateBinary();
QueueHandle_t wifi_laptime_lists_semaphore = xSemaphoreCreateBinary();
QueueHandle_t wifi_laptime_status_semaphore = xSemaphoreCreateBinary();

char list_top_str[LAPTIME_LIST_SIZE_WIFI][LAPTIME_STR_LENGTH] = {0};
char list_last_str[LAPTIME_LIST_SIZE_WIFI][LAPTIME_STR_LENGTH] = {0};
char list_penalty_time_str[LAPTIME_LIST_SIZE_WIFI][PENALTY_TIME_STR_LENGTH] = {0};
char list_penalty_oc_str[LAPTIME_LIST_SIZE_WIFI][PENALTY_COUNT_STR_LENGTH] = {0};
char list_penalty_doo_str[LAPTIME_LIST_SIZE_WIFI][PENALTY_COUNT_STR_LENGTH] = {0};

char penalty_time_str[PENALTY_TIME_STR_LENGTH] = "+00:00";
char penalty_oc_str[PENALTY_COUNT_STR_LENGTH] = "0";
char penalty_doo_str[PENALTY_COUNT_STR_LENGTH] = "0";

char driver_list[DRIVER_COUNT][4] = {"AAA", "BBB", "CCC"};

Lapmode lap_mode = ONE_GATE_MODE;
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
