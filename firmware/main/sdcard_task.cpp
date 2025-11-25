#include "sdcard_task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/idf_additions.h"
#include "main.h"
#include "portmacro.h"
#include "sd_protocol_types.h"
#include "sdmmc_cmd.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "sdcard.h"

bool first_lap_flag = true;
char session_str[14] = "#0,";

static const char *TAG = "SDCARD_TASK";

esp_err_t sdcard_init(sdmmc_card_t **card_pointer)
{
    esp_err_t ret = ESP_OK;
    // sd hardware initialization
    ret = sdcard_mount(card_pointer);
    if (ret)
    {
        ESP_LOGE(TAG, "%s", esp_err_to_name(ret));
        return ret;
    }
    sdmmc_card_t *card_handle = *card_pointer;
    if (card_handle == NULL)
    {
        return ESP_FAIL;
    }

    // try communication with sd
    ret = sdmmc_get_status(card_handle);
    if (ret)
    {
        return ret;
    }

    // try reading laptimer file, if it's missing create one with session number 0
    unsigned int br;
    char sd_buffer[256] = "\0";
    int session_num = 0;
    if (sdcard_read("laptimer.csv", sd_buffer, sizeof(sd_buffer), &br) !=
        ESP_OK)
    {
        ESP_LOGI(TAG, "CREATING NEW FILE");
        ret = sdcard_write("laptimer.csv", "SESSION, LAP, TIME\n");
    }
    else
    {
        sd_buffer[br] = '\0';
        char *last_ch = strrchr(sd_buffer, '#');
        if (last_ch != NULL)
        {
            session_num = atoi(last_ch + 1);
            session_num++;
        }
    }
    sprintf(session_str, "#%d,", session_num);
    ESP_LOGI(TAG, "INIT OK");
    return ret;
}

esp_err_t sdcard_deinit(sdmmc_card_t **card_pointer)
{
    return sdcard_unmount(card_pointer);
}

esp_err_t sdcard_save_laptime(char laptime_saved_str[13])
{
    esp_err_t ret = ESP_OK;

    if (laptime_saved_str == NULL || sd_active_flag == false)
        return ESP_ERR_INVALID_ARG;

    // if it's first lap, create new session label
    ret = sdcard_append("laptimer.csv", session_str);
    if (ret)
        return ret;

    ret = sdcard_append("laptimer.csv", laptime_saved_str);
    return ret;
}

void sdcard_task(void *args)
{
    sdmmc_card_t *card_handle = NULL;
    sdcard_init(&card_handle);
    char buf[13];
    for (;;)
    {
        if (sd_active_flag == true && sd_fail_flag == false)
        {
            if (xQueueReceive(sd_queue, buf, portMAX_DELAY) == pdTRUE)
            {
                sdcard_save_laptime(buf);
            }
        }
        else if (sd_active_flag == false && sd_fail_flag == false)
        {
            sdcard_deinit(&card_handle);
            sd_fail_flag = true;
        }
        else if (sd_active_flag == false && sd_fail_flag == true)
        {
            xSemaphoreTake(sd_reinit_semaphore, portMAX_DELAY);
            if (sdcard_init(&card_handle) == ESP_OK)
                sd_fail_flag = false;
        }
    }
    ESP_LOGI(TAG, "TASK ABORT");
    vTaskDelete(NULL);
}