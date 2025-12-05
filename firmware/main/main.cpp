#include "main.h"

#include "laptimer_task.h"
#include "sdcard_task.h"
#include "lcd_task.h"
#include "wifi_task.h"

#include "gpio.h"
#include "timer.h"

QueueHandle_t sd_queue = xQueueCreate(LAPTIME_LIST_SIZE_LOCAL, sizeof(char[LAPTIME_STRING_LENGTH]));
QueueHandle_t sd_reinit_semaphore = xSemaphoreCreateBinary();

QueueHandle_t lcd_laptime_current_queue = xQueueCreate(1, sizeof(char[LAPTIME_STRING_LENGTH]));
QueueHandle_t lcd_laptime_penalty_queue = xQueueCreate(1, sizeof(Penalty_data));
QueueHandle_t lcd_laptime_lists_semaphore = xSemaphoreCreateBinary();
QueueHandle_t lcd_laptime_status_queue = xQueueCreate(1, sizeof(bool[STATUS_LIST_LENGTH]));

QueueHandle_t wifi_laptime_current_queue = xQueueCreate(1, sizeof(char[LAPTIME_STRING_LENGTH]));
QueueHandle_t wifi_laptime_lists_semaphore = xSemaphoreCreateBinary();
QueueHandle_t wifi_laptime_status_queue = xQueueCreate(1, sizeof(bool[STATUS_LIST_LENGTH]));

char lcd_list_buffer[2][LAPTIME_LIST_SIZE_LCD][LAPTIME_STRING_LENGTH] = {0};
char wifi_list_buffer[2][LAPTIME_LIST_SIZE_WIFI][LAPTIME_STRING_LENGTH] = {0};

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
