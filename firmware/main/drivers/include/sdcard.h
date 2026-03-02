#pragma once

#include "main.h"

#include "sd_protocol_types.h"

/// @brief SD card pinout
#define SD_SPI_CLK CONFIG_SD_SPI_CLK
#define SD_SPI_MISO CONFIG_SD_SPI_MISO
#define SD_SPI_MOSI CONFIG_SD_SPI_MOSI
#define SD_SPI_CS CONFIG_SD_SPI_CS

#define SD_CD CONFIG_SD_CD

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t sdcard_spi_init();
    esp_err_t sdcard_mount(sdmmc_card_t **out_card);
    esp_err_t sdcard_unmount(sdmmc_card_t **out_card);

    esp_err_t sdcard_write(const char *filename, const char *text);
    esp_err_t sdcard_append(const char *filename, const char *text);
    esp_err_t sdcard_read(const char *filename, char *buffer, size_t bufsize,
                          size_t *bytes_read);

#ifdef __cplusplus
}
#endif