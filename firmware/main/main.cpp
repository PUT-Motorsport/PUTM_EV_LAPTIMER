#include "esp_err.h"

#include "freertos/idf_additions.h"

#include "laptimer_task.h"
#include "sdcard_task.h"
#include "lcd_task.h"

#include "gpio.h"
#include "spi.h"
#include "timer.h"

/*
SD_SPI_CLK - GPIO14
SD_SPI_MISO - GPIO35
SD_SPI_MOSI - GPIO13
SD_SPI_CS - GPIO27

LCD_SPI_CLK - GPIO18
LCD_SPI_MOSI - GPIO23
LCD_SPI_CS - GPIO5
LCD_DC - GPIO17
LCD_RESET - GPIO16
LCD_BL - GPIO21


LAP_GATE1_BTN - GPIO32
LAP_GATE2_BTN - GPIO33
LAP_RESET_BTN - GPIO25
LAP_MODE_BTN - GPIO26
*/

QueueHandle_t sd_queue = xQueueCreate(LAPTIME_LIST_SIZE_LOCAL, sizeof(char[LAPTIME_STRING_LENGTH]));
QueueHandle_t sd_reinit_semaphore = xSemaphoreCreateBinary();
QueueHandle_t lcd_laptime_current_queue = xQueueCreate(2, sizeof(wchar_t[LAPTIME_STRING_LENGTH]));
QueueHandle_t lcd_laptime_lists_queue = xQueueCreate(1, sizeof(wchar_t[2][LAPTIME_LIST_SIZE_LCD][LAPTIME_STRING_LENGTH]));
QueueHandle_t lcd_laptime_status_queue = xQueueCreate(1, sizeof(bool[3]));

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(spi_init());
    ESP_ERROR_CHECK(gpio_init());
    ESP_ERROR_CHECK(timer_init());
    ESP_ERROR_CHECK(isr_init());

    xTaskCreatePinnedToCore(sdcard_task, "SD_TASK", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(laptimer_task, "LAPTIMER_TASK", 4096, NULL, 1,
                            NULL, 1);
    xTaskCreatePinnedToCore(lcd_task, "LCD_TASK", 8192, NULL, 1, NULL, 0);

    vTaskDelete(NULL);
}
