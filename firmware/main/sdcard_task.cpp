#include "sdcard_task.h"

#include "portmacro.h"
#include "sd_protocol_types.h"
#include "sdmmc_cmd.h"

#include "sdcard.h"
#include "gpio.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "SDCARD_TASK";

char session_str[14] = {"#00"};
int session_num = 0;

static char driver_list_local[DRIVER_MAX_COUNT][DRIVER_TAG_LENGTH] = DRIVER_LIST_DEFAULT;

bool sd_detect_flag = false;

esp_err_t sdcard_get_driver_list(sdmmc_card_t **card_pointer)
{
    unsigned int br;
    char sd_buffer[SD_BUFFER_SIZE] = "\0";

    char *driver_tag_temp = {0};
    char driver_list_temp[DRIVER_MAX_COUNT - 1][DRIVER_TAG_LENGTH] = {"---"};
    uint8_t driver_count_temp = 0;

    if (sdcard_read("DRIVERS.csv", sd_buffer, sizeof(sd_buffer), &br) ==
        ESP_OK)
    {
        sd_buffer[br] = '\0';
        driver_tag_temp = strtok(sd_buffer, ",");
        for (; (driver_count_temp < DRIVER_MAX_COUNT - 1 && driver_tag_temp != NULL); driver_count_temp++)
        {
            driver_tag_temp[DRIVER_TAG_LENGTH - 1] = '\0';
            ESP_LOGI(TAG, "DRIVER: %s", driver_tag_temp);
            strcpy(driver_list_temp[driver_count_temp], driver_tag_temp);
            driver_tag_temp = strtok(NULL, ",");
        }
        ESP_LOGI(TAG, "COUNT: %d", driver_count_temp);
        if (xSemaphoreTake(driver_list_mutex, portMAX_DELAY) == pdTRUE)
        {
            for (int i = 0; i < driver_count_temp; i++)
            {
                strncpy(driver_list[i + 1], driver_list_temp[i], sizeof(driver_list[i + 1]));
            }
            driver_count = driver_count_temp;
            xSemaphoreGive(driver_list_mutex);
        }
    }
    return ESP_OK;
}

/**
 * @brief Mounts sd card in file system, checks communication and creates new .csv file if doesn't exist
 * @param card_pointer Pointer to sd card handle
 * @return Error check
 */
esp_err_t sdcard_init(sdmmc_card_t **card_pointer)
{
    esp_err_t ret = ESP_OK;
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
    sd_active_flag = true;
    unsigned int br;
    char sd_buffer[SD_BUFFER_SIZE] = "\0";

    /// Try to read laptimer file, if it's missing create one with session number 0
    if (sdcard_read("laptimer.csv", sd_buffer, sizeof(sd_buffer), &br) !=
        ESP_OK)
    {
        ESP_LOGI(TAG, "CREATING NEW FILE");
        ret = sdcard_write("laptimer.csv", "SESSION, LAP, TIME, PENALTY, OC, DOO, DRIVER\n");
        if (ret)
        {
            return ret;
        }
    }
    // If file exists, check last session number and increment it
    else if (session_num == 0)
    {
        sd_buffer[br] = '\0';
        char *last_ch = strrchr(sd_buffer, '#');
        if (last_ch != NULL)
        {
            session_num = atoi(last_ch + 1);
            session_num++;
        }
    }
    sdcard_get_driver_list(card_pointer);
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
esp_err_t sdcard_save_laptime(Laptime laptime_saved)
{
    esp_err_t ret = ESP_OK;

    char laptime_saved_str[LAPTIME_STR_LENGTH] = {0};
    char laptime_penalty_str[PENALTY_TIME_STR_LENGTH] = {0};
    char laptime_oc_str[PENALTY_COUNT_STR_LENGTH] = {0};
    char laptime_doo_str[PENALTY_COUNT_STR_LENGTH] = {0};
    char laptime_driver_str[DRIVER_TAG_LENGTH] = {0};

    laptime_saved.convert_string(laptime_saved_str, sizeof(laptime_saved_str));
    laptime_saved.penalty_string(laptime_penalty_str, sizeof(laptime_penalty_str));
    snprintf(laptime_oc_str, sizeof(laptime_oc_str), "%3u,", laptime_saved.oc_count);
    snprintf(laptime_doo_str, sizeof(laptime_doo_str), "%3u,", laptime_saved.doo_count);
    snprintf(laptime_driver_str, sizeof(laptime_driver_str), "%s", driver_list_local[laptime_saved.driver_id]);

    if (sd_active_flag == false)
        return ESP_FAIL;

    ret = sdcard_append("laptimer.csv", session_str);
    if (ret)
    {
        return ret;
    }

    ret = sdcard_append("laptimer.csv", laptime_saved_str);
    if (ret)
    {
        return ret;
    }
    ret = sdcard_append("laptimer.csv", ",");
    if (ret)
    {
        return ret;
    }
    ret = sdcard_append("laptimer.csv", laptime_penalty_str);
    if (ret)
    {
        return ret;
    }
    ret = sdcard_append("laptimer.csv", ",");
    if (ret)
    {
        return ret;
    }
    ret = sdcard_append("laptimer.csv", laptime_oc_str);
    if (ret)
    {
        return ret;
    }
    ret = sdcard_append("laptimer.csv", laptime_doo_str);
    if (ret)
    {
        return ret;
    }
    ret = sdcard_append("laptimer.csv", laptime_driver_str);
    if (ret)
    {
        return ret;
    }
    ret = sdcard_append("laptimer.csv", "\n");
    if (ret)
    {
        return ret;
    }
    if (ret == ESP_OK)
        ESP_LOGI(TAG, "LAPTIME SAVE OK");
    else
        ESP_LOGI(TAG, "LAPTIME SAVE FAIL");
    return ret;
}

esp_err_t sdcard_check_integrity(char laptime_check_str[LAPTIME_STR_LENGTH])
{
    if (laptime_check_str == NULL)
        return ESP_FAIL;

    esp_err_t ret = ESP_OK;
    unsigned int br;
    char sd_buffer[SD_BUFFER_SIZE] = "\0";
    ret = sdcard_read("laptimer.csv", sd_buffer, sizeof(sd_buffer), &br);
    if (ret)
    {
        return ret;
    }
    if (strstr(sd_buffer, laptime_check_str) != NULL)
        ret = ESP_OK;
    else
        ret = ESP_FAIL;
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
    if (sdcard_spi_init() == ESP_FAIL)
        vTaskDelete(NULL);
    if ((sd_detect_flag = !gpio_get_level((gpio_num_t)SD_CD)) == true)
        sd_active_flag = !sdcard_init(&card_handle);
    static Laptime laptime_saved;
    for (;;)
    {
        if (xSemaphoreTake(driver_list_mutex, 0) == pdTRUE)
        {
            memcpy(driver_list_local, driver_list, sizeof(driver_list_local));
            xSemaphoreGive(driver_list_mutex);
        }

        sd_detect_flag = !gpio_get_level((gpio_num_t)SD_CD);

        if (sd_detect_flag == false && sd_active_flag == true)
        {
            sdcard_deinit(&card_handle);
            sd_active_flag = false;
        }

        if (sd_detect_flag == true && sd_active_flag == true)
        {
            if (xQueueReceive(laptime_saved_queue_sd, &laptime_saved, 0) == pdTRUE)
            {
                sd_active_flag = !sdcard_save_laptime(laptime_saved);
                // sdcard_check_integrity(laptime_saved_str);
            }
        }
        if (sd_detect_flag == true && sd_active_flag == false)
        {
            sd_active_flag = !sdcard_init(&card_handle);
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}