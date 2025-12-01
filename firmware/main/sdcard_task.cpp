#include "sdcard_task.h"

#include "portmacro.h"
#include "sd_protocol_types.h"
#include "sdmmc_cmd.h"

#include "sdcard.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char session_str[14] = {0};

static const char *TAG = "SDCARD_TASK";

/**
 * @brief Mounts sd card in file system, checks communication and creates new .csv file if doesn't exist
 * @param card_pointer Pointer to sd card handle
 * @return Error check
 */
esp_err_t sdcard_init(sdmmc_card_t **card_pointer)
{
    esp_err_t ret = ESP_OK;
    /// SD hardware initialization
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

    /// Try to communicate with sd card
    ret = sdmmc_get_status(card_handle);
    if (ret)
    {
        return ret;
    }

    unsigned int br;
    char sd_buffer[SD_BUFFER_SIZE] = "\0";
    int session_num = 0;

    /// Try to read laptimer file, if it's missing create one with session number 0
    if (sdcard_read("laptimer.csv", sd_buffer, sizeof(sd_buffer), &br) !=
        ESP_OK)
    {
        ESP_LOGI(TAG, "CREATING NEW FILE");
        ret = sdcard_write("laptimer.csv", "SESSION, LAP, TIME\n");
    }
    // If file exists, check last session number and increment it
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

/**
 * @brief Unmounts sd card
 * @param card_pointer Pointer to sd card handle
 * @return Error check
 */
esp_err_t sdcard_deinit(sdmmc_card_t **card_pointer)
{
    return sdcard_unmount(card_pointer);
}

/**
 * @brief Saves received laptime on sd card, if card doesn't respond return error
 * @param laptime_saved_str
 * @return Error check
 */
esp_err_t sdcard_save_laptime(char laptime_saved_str[LAPTIME_STRING_LENGTH])
{
    esp_err_t ret = ESP_OK;

    if (laptime_saved_str == NULL || sd_active_flag == false)
        return ESP_ERR_INVALID_ARG;

    ret = sdcard_append("laptimer.csv", session_str);
    if (ret)
        return ret;

    ret = sdcard_append("laptimer.csv", laptime_saved_str);
    if (ret == ESP_OK)
        ESP_LOGI(TAG, "LAPTIME SAVE OK");
    else
        ESP_LOGI(TAG, "LAPTIME SAVE FAIL");
    return ret;
}

/**
 * @brief SD card task initializes sd card and file system, then in loop:
 * - Tries to save laptime received from queue
 * - Deinitalizes sd card if save failed
 * - Tries to reinitialize card when receive semaphore from laptimer_task
 */
void sdcard_task(void *args)
{
    sdmmc_card_t *card_handle = NULL;
    sdcard_init(&card_handle);
    char buf[LAPTIME_STRING_LENGTH];
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
    vTaskDelete(NULL);
}