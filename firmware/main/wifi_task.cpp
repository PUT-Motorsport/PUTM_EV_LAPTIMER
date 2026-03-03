#include "wifi_task.h"

#include "wifi_driver.h"
#include "timer.h"
#include "webpage.h"

#include "esp_http_server.h"
#include "cJSON.h"

static const char *TAG = "WIFI_TASK";

/**
 * @brief Current laptime data used to display on website
 */
static Laptime laptime_current;

static Driver_list driver_list_local;

static bool ip_refresh_flag = false;

/**
 * @brief Mutex used by data_get_handler to ensure that it can read local data safely
 */
static SemaphoreHandle_t data_mutex = NULL;

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/**
 * @brief Handler updates data on website after receiving mutex
 * @param req HTTP request structure
 * @return Error check
 */
static esp_err_t data_get_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

    char laptime_current_str[LAPTIME_STR_LENGTH] = LAPTIME_STR_DEFAULT;
    char penalty_time_str[PENALTY_TIME_STR_LENGTH] = PENALTY_STR_DEFAULT;
    char penalty_count_str[PENALTY_COUNT_STR_LENGTH] = COUNT_STR_DEFAULT;

    if (xSemaphoreTake(data_mutex, portMAX_DELAY))
    {
        laptime_current.convert_string_full(laptime_current_str, sizeof(laptime_current_str));
        cJSON_AddStringToObject(root, "current", laptime_current_str);

        int id = laptime_current.driver_id;
        if (id < 0 || id >= DRIVER_MAX_COUNT)
            id = 0;
        cJSON_AddStringToObject(root, "current_driver_tag", driver_list_local.list[id]);
        cJSON_AddNumberToObject(root, "current_driver_id", id);

        laptime_current.convert_string_penalty(penalty_time_str, sizeof(penalty_time_str));
        cJSON_AddStringToObject(root, "penalty_time", penalty_time_str);

        snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_current.oc_count);
        cJSON_AddStringToObject(root, "penalty_oc", penalty_count_str);

        snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_current.doo_count);
        cJSON_AddStringToObject(root, "penalty_doo", penalty_count_str);

        cJSON *status = cJSON_CreateObject();
        cJSON_AddBoolToObject(status, "mode", config_main.two_gate_mode);
        cJSON_AddBoolToObject(status, "stop", stop_flag);
        cJSON_AddBoolToObject(status, "sd", sd_active_flag);
        cJSON_AddItemToObject(root, "status", status);

        // Take global mutex for lists
        if (xSemaphoreTake(laptime_lists_mutex, portMAX_DELAY))
        {
            cJSON *drivers_arr = cJSON_CreateArray();
            cJSON *driver_best_arr = cJSON_CreateArray();
            cJSON *driver_lap_count_arr = cJSON_CreateArray();
            cJSON *driver_pen_time_arr = cJSON_CreateArray();
            cJSON *driver_pen_oc_arr = cJSON_CreateArray();
            cJSON *driver_pen_doo_arr = cJSON_CreateArray();

            for (int i = 0; i <= driver_list_local.driver_count; i++)
            {
                cJSON_AddItemToArray(drivers_arr, cJSON_CreateString(driver_list_local.list[i]));

                laptime_list_driver[i].convert_string_time(laptime_current_str, sizeof(laptime_current_str));
                cJSON_AddItemToArray(driver_best_arr, cJSON_CreateString(laptime_current_str));

                cJSON_AddItemToArray(driver_lap_count_arr, cJSON_CreateNumber(laptime_list_driver[i].count - 1));

                laptime_list_driver[i].convert_string_penalty(penalty_time_str, sizeof(penalty_time_str));
                cJSON_AddItemToArray(driver_pen_time_arr, cJSON_CreateString(penalty_time_str));

                snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_list_driver[i].oc_count);
                cJSON_AddItemToArray(driver_pen_oc_arr, cJSON_CreateString(penalty_count_str));

                snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_list_driver[i].doo_count);
                cJSON_AddItemToArray(driver_pen_doo_arr, cJSON_CreateString(penalty_count_str));
            }
            cJSON_AddItemToObject(root, "all_drivers", drivers_arr);
            cJSON_AddItemToObject(root, "driver_best", driver_best_arr);
            cJSON_AddItemToObject(root, "driver_lap_count", driver_lap_count_arr);
            cJSON_AddItemToObject(root, "driver_pen_time", driver_pen_time_arr);
            cJSON_AddItemToObject(root, "driver_pen_oc", driver_pen_oc_arr);
            cJSON_AddItemToObject(root, "driver_pen_doo", driver_pen_doo_arr);

            cJSON *last_arr = cJSON_CreateArray();
            cJSON *top_arr = cJSON_CreateArray();

            cJSON *last_driver_tag_arr = cJSON_CreateArray();
            cJSON *last_driver_id_arr = cJSON_CreateArray();
            cJSON *top_driver_tag_arr = cJSON_CreateArray();
            cJSON *top_driver_id_arr = cJSON_CreateArray();

            cJSON *last_pen_time_arr = cJSON_CreateArray();
            cJSON *last_pen_oc_arr = cJSON_CreateArray();
            cJSON *last_pen_doo_arr = cJSON_CreateArray();

            for (int i = 0; i < LAPTIME_LIST_SIZE_WIFI; i++)
            {
                // Last
                laptime_list_last[i].convert_string_full(laptime_current_str, sizeof(laptime_current_str));
                cJSON_AddItemToArray(last_arr, cJSON_CreateString(laptime_current_str));

                int drv_id = laptime_list_last[i].driver_id;
                if (drv_id < 0 || drv_id >= DRIVER_MAX_COUNT)
                    drv_id = 0;
                cJSON_AddItemToArray(last_driver_tag_arr, cJSON_CreateString(driver_list_local.list[drv_id]));
                cJSON_AddItemToArray(last_driver_id_arr, cJSON_CreateNumber(drv_id));

                laptime_list_last[i].convert_string_penalty(penalty_time_str, sizeof(penalty_time_str));
                cJSON_AddItemToArray(last_pen_time_arr, cJSON_CreateString(penalty_time_str));

                snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_list_last[i].oc_count);
                cJSON_AddItemToArray(last_pen_oc_arr, cJSON_CreateString(penalty_count_str));

                snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_list_last[i].doo_count);
                cJSON_AddItemToArray(last_pen_doo_arr, cJSON_CreateString(penalty_count_str));

                // Top
                laptime_list_top[i].convert_string_full(laptime_current_str, sizeof(laptime_current_str));
                cJSON_AddItemToArray(top_arr, cJSON_CreateString(laptime_current_str));

                drv_id = laptime_list_top[i].driver_id;
                if (drv_id < 0 || drv_id >= DRIVER_MAX_COUNT)
                    drv_id = 0;
                cJSON_AddItemToArray(top_driver_tag_arr, cJSON_CreateString(driver_list_local.list[drv_id]));
                cJSON_AddItemToArray(top_driver_id_arr, cJSON_CreateNumber(drv_id));
            }

            cJSON_AddItemToObject(root, "last", last_arr);
            cJSON_AddItemToObject(root, "top", top_arr);

            cJSON_AddItemToObject(root, "last_driver_tag", last_driver_tag_arr);
            cJSON_AddItemToObject(root, "last_driver_id", last_driver_id_arr);
            cJSON_AddItemToObject(root, "top_driver_tag", top_driver_tag_arr);
            cJSON_AddItemToObject(root, "top_driver_id", top_driver_id_arr);

            cJSON_AddItemToObject(root, "last_pen_time", last_pen_time_arr);
            cJSON_AddItemToObject(root, "last_pen_oc", last_pen_oc_arr);
            cJSON_AddItemToObject(root, "last_pen_doo", last_pen_doo_arr);

            xSemaphoreGive(laptime_lists_mutex);
        }

        xSemaphoreGive(data_mutex);
    }

    const char *sys_info = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, sys_info, strlen(sys_info));
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief Handler downloads lists in CSV format
 * @param req HTTP request structure
 * @return Error check
 */
static esp_err_t laptimes_csv_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"laptimes.csv\"");

    // Header
    httpd_resp_sendstr_chunk(req, "Number,Laptime [mm:ss:ms], Driver, Penalty Time [mm:ss],OC,DOO,Date,Hour\n");

    if (xSemaphoreTake(laptime_lists_mutex, portMAX_DELAY))
    {
        char line[256];
        char time_str[LAPTIME_STR_LENGTH];
        char pen_str[PENALTY_TIME_STR_LENGTH];

        for (int i = 0; i < LAPTIME_LIST_SIZE_WIFI; i++)
        {
            laptime_list_last[i].convert_string_full(time_str, sizeof(time_str));
            laptime_list_last[i].convert_string_penalty(pen_str, sizeof(pen_str));

            int drv_id = laptime_list_last[i].driver_id;
            if (drv_id < 0 || drv_id >= DRIVER_MAX_COUNT)
                drv_id = 0;

            snprintf(line, sizeof(line), "%s,%s,%s,%u,%u,%s,%s\n",
                     time_str,
                     driver_list_local.list[drv_id],
                     pen_str,
                     laptime_list_last[i].oc_count,
                     laptime_list_last[i].doo_count,
                     laptime_list_last[i].date,
                     laptime_list_last[i].timeofday);
            httpd_resp_sendstr_chunk(req, line);
        }
        xSemaphoreGive(laptime_lists_mutex);
    }

    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

/**
 * @brief Handler downloads driver statistics in CSV format
 * @param req HTTP request structure
 * @return Error check
 */
static esp_err_t drivers_csv_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"drivers.csv\"");

    // Header
    httpd_resp_sendstr_chunk(req, "Driver,Best laptime,Lap count,Penalty time sum,OC sum,DOO sum\n");

    if (xSemaphoreTake(laptime_lists_mutex, portMAX_DELAY))
    {
        char line[256];
        char time_str[LAPTIME_STR_LENGTH];
        char pen_str[PENALTY_TIME_STR_LENGTH];

        for (int i = 1; i <= driver_list_local.driver_count; i++)
        {
            laptime_list_driver[i].convert_string_time(time_str, sizeof(time_str));
            laptime_list_driver[i].convert_string_penalty(pen_str, sizeof(pen_str));

            snprintf(line, sizeof(line), "%s,%s,%u,%s,%u,%u\n",
                     driver_list_local.list[i],
                     time_str,
                     laptime_list_driver[i].count,
                     pen_str,
                     laptime_list_driver[i].oc_count,
                     laptime_list_driver[i].doo_count);
            httpd_resp_sendstr_chunk(req, line);
        }
        xSemaphoreGive(laptime_lists_mutex);
    }

    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

/**
 * @brief Handler returns current config in JSON format
 * @param req HTTP request structure
 * @return Error check
 */
static esp_err_t config_get_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (xSemaphoreTake(config_mutex, portMAX_DELAY))
    {
        cJSON_AddBoolToObject(root, "two_gate_mode", config_main.two_gate_mode);
        // Return current wifi mode as number (0 for AP, 1 for STA)
        cJSON_AddNumberToObject(root, "wifi_mode", config_main.wifi_mode);
        cJSON_AddStringToObject(root, "wifi_ssid", config_main.wifi_ssid);
        cJSON_AddStringToObject(root, "wifi_password", config_main.wifi_password);
        cJSON_AddStringToObject(root, "time_set", config_main.time_set);
        cJSON_AddStringToObject(root, "date_set", config_main.date_set);

        cJSON *drivers = cJSON_CreateArray();
        // Driver list starts from 1, index 0 is placeholder
        for (int i = 1; i <= config_main.driver_list.driver_count; i++)
        {
            cJSON_AddItemToArray(drivers, cJSON_CreateString(config_main.driver_list.list[i]));
        }
        cJSON_AddItemToObject(root, "driver_list", drivers);

        xSemaphoreGive(config_mutex);
    }
    const char *resp = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    free((void *)resp);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief Handler receives new config via POST
 * @param req HTTP request structure
 * @return Error check
 */
static esp_err_t config_post_handler(httpd_req_t *req)
{
    char buf[1024];
    int ret, remaining = req->content_len;
    if (remaining >= sizeof(buf))
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0)
        return ESP_FAIL;
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (xSemaphoreTake(config_mutex, portMAX_DELAY))
    {
        cJSON *gates = cJSON_GetObjectItem(root, "two_gate_mode");
        if (gates)
            config_main.two_gate_mode = cJSON_IsTrue(gates);

        cJSON *wifi_mode_json = cJSON_GetObjectItem(root, "wifi_mode");
        if (wifi_mode_json)
        {
            if (cJSON_IsNumber(wifi_mode_json))
            {
                config_main.wifi_mode = (wifi_mode_json->valueint == 1) ? WIFI_MODE_STA : WIFI_MODE_AP;
            }
            else if (cJSON_IsBool(wifi_mode_json))
            {
                config_main.wifi_mode = cJSON_IsTrue(wifi_mode_json) ? WIFI_MODE_STA : WIFI_MODE_AP;
            }
        }

        cJSON *ssid = cJSON_GetObjectItem(root, "wifi_ssid");
        if (ssid && ssid->valuestring)
        {
            strncpy(config_main.wifi_ssid, ssid->valuestring, sizeof(config_main.wifi_ssid) - 1);
            config_main.wifi_ssid[sizeof(config_main.wifi_ssid) - 1] = '\0';
        }

        cJSON *pass = cJSON_GetObjectItem(root, "wifi_password");
        if (pass && pass->valuestring)
        {
            strncpy(config_main.wifi_password, pass->valuestring, sizeof(config_main.wifi_password) - 1);
            config_main.wifi_password[sizeof(config_main.wifi_password) - 1] = '\0';
        }

        cJSON *time_set_json = cJSON_GetObjectItem(root, "time_set");
        if (time_set_json && time_set_json->valuestring)
        {
            strncpy(config_main.time_set, time_set_json->valuestring, sizeof(config_main.time_set) - 1);
            config_main.time_set[sizeof(config_main.time_set) - 1] = '\0';
        }

        cJSON *date_set_json = cJSON_GetObjectItem(root, "date_set");
        if (date_set_json && date_set_json->valuestring)
        {
            strncpy(config_main.date_set, date_set_json->valuestring, sizeof(config_main.date_set) - 1);
            config_main.date_set[sizeof(config_main.date_set) - 1] = '\0';
        }

        system_set_time(config_main.time_set, config_main.date_set);

        cJSON *drivers = cJSON_GetObjectItem(root, "driver_list");
        if (drivers && cJSON_IsArray(drivers))
        {
            int count = cJSON_GetArraySize(drivers);
            // Limit to max count (minus 1 for the placeholder at index 0)
            if (count > DRIVER_MAX_COUNT - 1)
                count = DRIVER_MAX_COUNT - 1;

            config_main.driver_list.driver_count = count;

            // Clear existing list beyond count if needed, or just overwrite
            // We overwrite from index 1
            for (int i = 0; i < count; i++)
            {
                cJSON *item = cJSON_GetArrayItem(drivers, i);
                if (item && item->valuestring)
                {
                    strncpy(config_main.driver_list.list[i + 1], item->valuestring, DRIVER_TAG_LENGTH - 1);
                    config_main.driver_list.list[i + 1][DRIVER_TAG_LENGTH - 1] = '\0';
                }
            }
        }
        xSemaphoreGive(config_mutex);
    }
    cJSON_Delete(root);
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

/**
 * @brief Initialization of web server
 * @return Handle to http instance
 */
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: \'%d\'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &root);

        httpd_uri_t data = {
            .uri = "/api/data",
            .method = HTTP_GET,
            .handler = data_get_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &data);

        httpd_uri_t csv = {
            .uri = "/api/csv",
            .method = HTTP_GET,
            .handler = laptimes_csv_get_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &csv);

        httpd_uri_t drivers_csv = {
            .uri = "/api/drivers_csv",
            .method = HTTP_GET,
            .handler = drivers_csv_get_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &drivers_csv);

        httpd_uri_t config_get = {
            .uri = "/api/config",
            .method = HTTP_GET,
            .handler = config_get_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &config_get);

        httpd_uri_t config_post = {
            .uri = "/api/config",
            .method = HTTP_POST,
            .handler = config_post_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &config_post);

        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

/**
 * @brief WIFI task initializes access point and web server,
 *  then in loop updates values displayed on http website with values received from laptimer_task
 */
void wifi_task(void *args)
{
    data_mutex = xSemaphoreCreateMutex();

    char wifi_ssid_local[WIFI_SSID_STR_LENGTH] = WIFI_SSID_DEFAULT;
    char wifi_password_local[WIFI_PASSWORD_STR_LENGTH] = WIFI_PASSWORD_DEFAULT;
    char ip_str[WIFI_IP_LENGTH] = {0};

    for (;;)
    {

        // Update driver list from config
        if (driver_list_local.driver_count != config_main.driver_list.driver_count)
        {
            if (xSemaphoreTake(config_mutex, 0) == pdTRUE)
            {
                if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE)
                {
                    memcpy(&driver_list_local, &config_main.driver_list, sizeof(driver_list_local));
                    xSemaphoreGive(data_mutex);
                }
                xSemaphoreGive(config_mutex);
            }
        }

        // Wifi reinitialization
        if (wifi_reset_flag != WIFI_NO_RESET)
        {
            // Reinit with config values
            if (wifi_reset_flag == WIFI_RESET_CONFIG)
            {
                if (xSemaphoreTake(config_mutex, portMAX_DELAY) == pdTRUE)
                {
                    snprintf(wifi_ssid_local, sizeof(wifi_ssid_local), config_main.wifi_ssid);
                    snprintf(wifi_password_local, sizeof(wifi_password_local), config_main.wifi_password);
                    wifi_mode_flag = config_main.wifi_mode;
                    xSemaphoreGive(config_mutex);
                }
            }
            // Reinit with safe values
            else if (wifi_reset_flag == WIFI_RESET_DEFAULTS)
            {
                snprintf(wifi_ssid_local, sizeof(wifi_ssid_local), "%s", WIFI_SSID_DEFAULT);
                snprintf(wifi_password_local, sizeof(wifi_password_local), "%s", WIFI_PASSWORD_DEFAULT);
                wifi_mode_flag = WIFI_MODE_DEFAULT;
            }

            if (wifi_reinit(wifi_mode_flag, wifi_ssid_local, wifi_password_local) == ESP_OK)
            {
                start_webserver();
                snprintf(ip_str, sizeof(ip_str), "WAIT FOR IP");
                xQueueSend(ip_queue, ip_str, 0);
                ip_refresh_flag = true;
                wifi_reset_flag = WIFI_NO_RESET;
            }
            else
                wifi_mode_flag = WIFI_MODE_NULL;
        }
        else
        {
            // Read current laptime to display
            Laptime laptime_current_temp;
            if (xQueueReceive(laptime_current_queue_wifi, &laptime_current_temp, 0) == pdTRUE)
            {
                if (xSemaphoreTake(data_mutex, portMAX_DELAY))
                {
                    laptime_current = laptime_current_temp;
                    xSemaphoreGive(data_mutex);
                }
            }

            // Get ip and send to lcd task to display on screen
            if (ip_refresh_flag == true)
            {
                if (wifi_get_ip(ip_str) == ESP_OK)
                {
                    xQueueSend(ip_queue, ip_str, 0);
                    ip_refresh_flag = false;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
