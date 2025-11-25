#pragma once

#include "driver/spi_common.h"
#include "esp_err.h"

#define SD_SPI_CLK 14
#define SD_SPI_MISO 35
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

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t spi_init(void);

#ifdef __cplusplus
}
#endif