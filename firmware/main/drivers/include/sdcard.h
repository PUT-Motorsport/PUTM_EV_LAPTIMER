#pragma once

#include "esp_err.h"

#include "sd_protocol_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    extern volatile bool sd_active_flag;
    extern volatile bool sd_fail_flag;

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