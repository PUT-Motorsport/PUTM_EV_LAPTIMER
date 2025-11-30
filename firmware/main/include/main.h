#pragma once

#include "freertos/idf_additions.h"

#define LAP_GATE1_BTN GPIO_NUM_32
#define LAP_GATE2_BTN GPIO_NUM_33
#define LAP_RESET_BTN GPIO_NUM_35
#define LAP_MODE_PIN GPIO_NUM_25

#define SD_SPI_CLK 14
#define SD_SPI_MISO 26
#define SD_SPI_MOSI 13
#define SD_SPI_CS 27

#define LCD_SPI_CLK 18
#define LCD_SPI_MOSI 23
#define LCD_SPI_CS 5
#define LCD_DC 17
#define LCD_RESET 16
#define LCD_BL 21

#define SPI_SD_HOST SPI2_HOST
#define SPI_LCD_HOST SPI3_HOST

#define LAPTIME_LIST_SIZE_LOCAL 50
#define LAPTIME_LIST_SIZE_LCD 5
#define LAPTIME_LIST_SIZE_WIFI 15
#define LAPTIME_MIN 500
#define LAPTIME_STRING_LENGTH 13
#define LAPTIME_STRING_LENGTH_UART 14

extern QueueHandle_t sd_queue;
extern QueueHandle_t sd_reinit_semaphore;
extern QueueHandle_t lcd_laptime_current_queue;
extern QueueHandle_t lcd_laptime_lists_semaphore;
extern QueueHandle_t lcd_laptime_status_queue;

extern QueueHandle_t wifi_laptime_current_queue;
extern QueueHandle_t wifi_laptime_status_queue;
extern QueueHandle_t wifi_laptime_lists_semaphore;

extern char lcd_list_buffer[2][LAPTIME_LIST_SIZE_LCD][LAPTIME_STRING_LENGTH];
extern char wifi_list_buffer[2][LAPTIME_LIST_SIZE_WIFI][LAPTIME_STRING_LENGTH];
