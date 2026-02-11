#include "sdcard_task.h"

#include "portmacro.h"
#include "sd_protocol_types.h"
#include "sdmmc_cmd.h"

#include "sdcard.h"
#include "gpio.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>

const char *config_file_name = "config.txt";
const char *laptimes_file_name = "laptimer.csv";

static const char *TAG = "SDCARD_TASK";

char session_str[14] = {"#00"};
int session_num = 0;

bool sd_detect_flag = false;

esp_err_t sdcard_get_config(sdmmc_card_t **card_pointer)
{
    unsigned int br;
    char sd_buffer[SD_BUFFER_SIZE] = "\0";

    Config config_temp;

    if (sdcard_read(config_file_name, sd_buffer, sizeof(sd_buffer), &br) ==
        ESP_OK)
    {
        sd_buffer[br] = '\0';
        cJSON *config_json = cJSON_Parse(sd_buffer);
        if (config_json == NULL)
        {
            ESP_LOGE(TAG, "NO JSON");
            return ESP_FAIL;
        }

        cJSON *gates_json = cJSON_GetObjectItem(config_json, "two_gate_mode");
        cJSON *wifi_json = cJSON_GetObjectItem(config_json, "wifi_config");
        cJSON *driver_list_json = cJSON_GetObjectItem(config_json, "driver_list");

        if (gates_json)
        {
            config_temp.two_gate_mode = cJSON_IsTrue(gates_json);
            ESP_LOGI(TAG, "2 Gates mode: %u\n", (uint8_t)config_temp.two_gate_mode);
        }

        if (wifi_json)
        {
            cJSON *wifi_mode_json = cJSON_GetObjectItem(wifi_json, "mode");
            cJSON *wifi_ssid_json = cJSON_GetObjectItem(wifi_json, "ssid");
            cJSON *wifi_password_json = cJSON_GetObjectItem(wifi_json, "password");
            if ((bool)cJSON_GetNumberValue(wifi_mode_json) == false)
                config_temp.wifi_mode = WIFI_MODE_AP;
            else
                config_temp.wifi_mode = WIFI_MODE_STA;

            if (cJSON_IsString(wifi_ssid_json) && (wifi_ssid_json->valuestring != NULL))
            {
                snprintf(config_temp.wifi_ssid, sizeof(config_temp.wifi_ssid), "%s", wifi_ssid_json->valuestring);
            }
            if (cJSON_IsString(wifi_password_json) && (wifi_password_json->valuestring != NULL))
            {
                snprintf(config_temp.wifi_password, sizeof(config_temp.wifi_password), "%s", wifi_password_json->valuestring);
            }
            ESP_LOGI(TAG, "Wifi ssid: %s, Wifi password: %s\n", config_temp.wifi_ssid, config_temp.wifi_password);
        }

        if (driver_list_json)
        {
            config_temp.driver_list.driver_count = cJSON_GetArraySize(driver_list_json);
            ESP_LOGI(TAG, "Driver_count: %u\n", config_temp.driver_list.driver_count);

            for (int i = 0; i < config_temp.driver_list.driver_count; i++)
            {
                cJSON *driver_temp_json = cJSON_GetArrayItem(driver_list_json, i);
                if (cJSON_IsString(driver_temp_json) && (driver_temp_json->valuestring != NULL))
                {
                    snprintf(config_temp.driver_list.list[i + 1], sizeof(config_temp.driver_list.list[i + 1]), "%s", driver_temp_json->valuestring);
                    ESP_LOGI(TAG, "DRIVER: %s\n", config_temp.driver_list.list[i + 1]);
                }
            }
        }

        cJSON_Delete(config_json);

        if (xSemaphoreTake(config_mutex, portMAX_DELAY) == pdTRUE)
        {
            memcpy(&config_main, &config_temp, sizeof(config_main));
            xSemaphoreGive(config_mutex);
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
    if (sdcard_read(laptimes_file_name, sd_buffer, sizeof(sd_buffer), &br) !=
        ESP_OK)
    {
        ESP_LOGI(TAG, "CREATING NEW FILE");
        ret = sdcard_write(laptimes_file_name, "SESSION, LAP, TIME, PENALTY, OC, DOO, DRIVER, DATE, HOUR\n");
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
    sdcard_get_config(card_pointer);
    sprintf(session_str, "#%d", session_num);
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
esp_err_t sdcard_save_laptime(Laptime laptime_saved, Driver_list *driver_list)
{
    esp_err_t ret = ESP_OK;
    char laptime_record_str[LAPTIME_STR_LENGTH + PENALTY_TIME_STR_LENGTH + 2 * PENALTY_COUNT_STR_LENGTH + DRIVER_TAG_LENGTH + DATE_STR_LENGTH + TIMEOFDAY_STR_LENGTH] = {0};

    char laptime_saved_str[LAPTIME_STR_LENGTH] = {0};
    char laptime_penalty_str[PENALTY_TIME_STR_LENGTH] = {0};

    laptime_saved.convert_string_full(laptime_saved_str, sizeof(laptime_saved_str));
    laptime_saved.convert_string_penalty(laptime_penalty_str, sizeof(laptime_penalty_str));
    snprintf(laptime_record_str, sizeof(laptime_record_str), "%s,%s,%s,%3u,%3u,%s,%s,%s\n", session_str, laptime_saved_str, laptime_penalty_str,
             laptime_saved.oc_count, laptime_saved.doo_count,
             driver_list->list[laptime_saved.driver_id],
             laptime_saved.date, laptime_saved.timeofday);

    if (sd_active_flag == false)
        return ESP_FAIL;

    ret = sdcard_append(laptimes_file_name, laptime_record_str);
    if (ret)
        return ret;

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
    ret = sdcard_read(laptimes_file_name, sd_buffer, sizeof(sd_buffer), &br);
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

    Laptime laptime_saved;
    Driver_list driver_list_local;

    for (;;)
    {
        if (xSemaphoreTake(config_mutex, 0) == pdTRUE)
        {
            memcpy(driver_list_local.list, config_main.driver_list.list, sizeof(driver_list_local));
            driver_list_local.driver_count = config_main.driver_list.driver_count;
            xSemaphoreGive(config_mutex);
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
                sd_active_flag = !sdcard_save_laptime(laptime_saved, &driver_list_local);
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