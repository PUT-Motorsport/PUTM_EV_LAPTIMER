#include "sdcard.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_common.h"

#include <sys/stat.h>

#define SD_CARD_MOUNT_PATH "/sdcard"
#define MAX_CHAR_SIZE 64

static const char *TAG = "SDCARD";

esp_err_t sdcard_spi_init()
{
    spi_bus_config_t sdcard_spi_config = {
        .mosi_io_num = SD_SPI_MOSI,
        .miso_io_num = SD_SPI_MISO,
        .sclk_io_num = SD_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 10 * 1000,
    };
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &sdcard_spi_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI INIT FAIL");
        return ret;
    }
    ESP_LOGI(TAG, "SPI INIT OK");
    return ret;
}

esp_err_t sdcard_mount(sdmmc_card_t **out_card)
{
    if (out_card == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    host.max_freq_khz = 1000;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_SPI_CS;
    slot_config.host_id = host.slot;

    esp_err_t ret = esp_vfs_fat_sdspi_mount(SD_CARD_MOUNT_PATH, &host, &slot_config,
                                            &mount_config, out_card);

    if (ret == ESP_OK && out_card != NULL && *out_card == NULL)
    {
        ESP_LOGE(TAG, "CARD NULL");
        ret = ESP_FAIL;
    }

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "MOUNT FAILED");
        return ret;
    }
    ESP_LOGI(TAG, "MOUNT OK");
    sdmmc_card_print_info(stdout, *out_card);
    return ESP_OK;
}

esp_err_t sdcard_unmount(sdmmc_card_t **out_card)
{
    if (out_card == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "UNMOUNT OK");
    return esp_vfs_fat_sdcard_unmount(SD_CARD_MOUNT_PATH, *out_card);
}

static int32_t build_path(const char *filename, char *out_path, size_t out_path_len)
{
    if (filename == NULL || out_path == NULL)
        return -1;
    int32_t n = snprintf(out_path, out_path_len, "%s/%s", SD_CARD_MOUNT_PATH,
                         filename);
    if (n < 0 || (size_t)n >= out_path_len)
    {
        return -1;
    }
    return 0;
}

esp_err_t sdcard_write(const char *filename, const char *text)
{
    if (filename == NULL || text == NULL)
        return ESP_ERR_INVALID_ARG;

    char path[MAX_CHAR_SIZE];
    if (build_path(filename, path, sizeof(path)) != 0)
    {
        ESP_LOGE(TAG, "FILE LONG: %s", filename);
        return ESP_ERR_INVALID_ARG;
    }

    FILE *f = fopen(path, "w");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "FOPEN FAIL: %s", path);
        return ESP_FAIL;
    }

    size_t text_len = strlen(text);
    size_t written = fwrite(text, 1, text_len, f);

    if (fclose(f) != 0)
    {
        ESP_LOGE(TAG, "FCLOSE FAIL");
        return ESP_FAIL;
    }

    // ESP_LOGI(TAG, "WRITE: %zu BYTES TO %s", written, path);
    return (written == text_len) ? ESP_OK : ESP_FAIL;
}

esp_err_t sdcard_append(const char *filename, const char *text)
{

    if (filename == NULL || text == NULL)
        return ESP_ERR_INVALID_ARG;

    char path[MAX_CHAR_SIZE];
    if (build_path(filename, path, sizeof(path)) != 0)
    {
        ESP_LOGE(TAG, "FILE_LONG: %s", filename);
        return ESP_ERR_INVALID_ARG;
    }

    FILE *f = fopen(path, "a");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "FOPEN_FAIL: %s", path);
        return ESP_FAIL;
    }

    size_t text_len = strlen(text);
    size_t written = fwrite(text, 1, text_len, f);

    if (fclose(f) != 0)
    {
        ESP_LOGE(TAG, "FCLOSE_FAIL");
        return ESP_FAIL;
    }

    // ESP_LOGI(TAG, "APPEND %zu BYTES TO %s", written, path);
    return (written == text_len) ? ESP_OK : ESP_FAIL;
}

esp_err_t sdcard_read(const char *filename, char *buffer, size_t bufsize,
                      size_t *bytes_read)
{

    if (filename == NULL || buffer == NULL || bytes_read == NULL)
        return ESP_ERR_INVALID_ARG;

    *bytes_read = 0;
    char path[MAX_CHAR_SIZE];
    if (build_path(filename, path, sizeof(path)) != 0)
    {
        ESP_LOGE(TAG, "FILE_LONG: %s", filename);
        return ESP_ERR_INVALID_ARG;
    }

    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "FOPEN_FAIL: %s", path);
        return ESP_FAIL;
    }

    *bytes_read = fread(buffer, 1, bufsize - 1, f);
    if (ferror(f))
    {
        ESP_LOGE(TAG, "FREAD_FAIL");
        fclose(f);
        return ESP_FAIL;
    }

    buffer[*bytes_read] = '\0';

    if (fclose(f) != 0)
    {
        ESP_LOGE(TAG, "FCLOSE_FAIL");
        return ESP_FAIL;
    }
    // ESP_LOGI(TAG, "READ %zu BYES FROM %s", *bytes_read, path);

    return ESP_OK;
}
