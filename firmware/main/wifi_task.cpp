#include "wifi_task.hpp"

#include "wifi_driver.h"
#include "timer.h"
#include "webpage.hpp"

#include "esp_http_server.h"
#include <ArduinoJson.h>

struct HttpdChunkWriter {
    httpd_req_t *req;
    char buffer[512];
    size_t len;

    HttpdChunkWriter(httpd_req_t *r) : req(r), len(0) {}

    size_t write(uint8_t c) {
        buffer[len++] = c;
        if (len == sizeof(buffer)) {
            httpd_resp_send_chunk(req, buffer, len);
            len = 0;
        }
        return 1;
    }

    size_t write(const uint8_t *buf, size_t size) {
        size_t written = 0;
        while (size > 0) {
            size_t space = sizeof(buffer) - len;
            size_t chunk = size < space ? size : space;
            memcpy(buffer + len, buf, chunk);
            len += chunk;
            buf += chunk;
            size -= chunk;
            written += chunk;
            if (len == sizeof(buffer)) {
                httpd_resp_send_chunk(req, buffer, len);
                len = 0;
            }
        }
        return written;
    }

    void flush() {
        if (len > 0) {
            httpd_resp_send_chunk(req, buffer, len);
            len = 0;
        }
        httpd_resp_send_chunk(req, NULL, 0); // End chunked response
    }
};

/// @brief Size of wifi displayed best/last laptime list
#define LAPTIME_LIST_SIZE_WIFI 50

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
    JsonDocument root;

    char laptime_current_str[LAPTIME_STR_LENGTH] = LAPTIME_STR_DEFAULT;
    char penalty_time_str[PENALTY_TIME_STR_LENGTH] = PENALTY_STR_DEFAULT;
    char penalty_count_str[PENALTY_COUNT_STR_LENGTH] = COUNT_STR_DEFAULT;

    if (xSemaphoreTake(data_mutex, portMAX_DELAY))
    {
        laptime_current.convert_string_full(laptime_current_str, sizeof(laptime_current_str));
        root["current"] = laptime_current_str;

        int id = laptime_current.driver_id;
        if (id < 0 || id >= DRIVER_MAX_COUNT)
            id = 0;
        root["current_driver_tag"] = driver_list_local.list[id];
        root["current_driver_id"] = id;

        laptime_current.convert_string_penalty(penalty_time_str, sizeof(penalty_time_str));
        root["penalty_time"] = penalty_time_str;

        snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_current.oc_count);
        root["penalty_oc"] = penalty_count_str;

        snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_current.doo_count);
        root["penalty_doo"] = penalty_count_str;

        JsonObject status = root["status"].to<JsonObject>();
        status["mode"] = config_main.two_gate_mode;
        status["stop"] = stop_flag;
        status["sd"] = sd_active_flag;

        // Take global mutex for lists
        if (xSemaphoreTake(laptime_lists_mutex, portMAX_DELAY))
        {
            JsonArray drivers_arr = root["all_drivers"].to<JsonArray>();
            JsonArray driver_best_arr = root["driver_best"].to<JsonArray>();
            JsonArray driver_lap_count_arr = root["driver_lap_count"].to<JsonArray>();
            JsonArray driver_pen_time_arr = root["driver_pen_time"].to<JsonArray>();
            JsonArray driver_pen_oc_arr = root["driver_pen_oc"].to<JsonArray>();
            JsonArray driver_pen_doo_arr = root["driver_pen_doo"].to<JsonArray>();

            for (int i = 0; i <= driver_list_local.driver_count; i++)
            {
                drivers_arr.add(driver_list_local.list[i]);

                laptime_list_driver[i].convert_string_time(laptime_current_str, sizeof(laptime_current_str));
                driver_best_arr.add(laptime_current_str);

                driver_lap_count_arr.add(laptime_list_driver[i].count - 1);

                laptime_list_driver[i].convert_string_penalty(penalty_time_str, sizeof(penalty_time_str));
                driver_pen_time_arr.add(penalty_time_str);

                snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_list_driver[i].oc_count);
                driver_pen_oc_arr.add(penalty_count_str);

                snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_list_driver[i].doo_count);
                driver_pen_doo_arr.add(penalty_count_str);
            }

            JsonArray last_arr = root["last"].to<JsonArray>();
            JsonArray top_arr = root["top"].to<JsonArray>();

            JsonArray last_driver_tag_arr = root["last_driver_tag"].to<JsonArray>();
            JsonArray last_driver_id_arr = root["last_driver_id"].to<JsonArray>();
            JsonArray top_driver_tag_arr = root["top_driver_tag"].to<JsonArray>();
            JsonArray top_driver_id_arr = root["top_driver_id"].to<JsonArray>();

            JsonArray last_pen_time_arr = root["last_pen_time"].to<JsonArray>();
            JsonArray last_pen_oc_arr = root["last_pen_oc"].to<JsonArray>();
            JsonArray last_pen_doo_arr = root["last_pen_doo"].to<JsonArray>();

            for (int i = 0; i < LAPTIME_LIST_SIZE_WIFI; i++)
            {
                // Last
                laptime_list_last[i].convert_string_full(laptime_current_str, sizeof(laptime_current_str));
                last_arr.add(laptime_current_str);

                int drv_id = laptime_list_last[i].driver_id;
                if (drv_id < 0 || drv_id >= DRIVER_MAX_COUNT)
                    drv_id = 0;
                last_driver_tag_arr.add(driver_list_local.list[drv_id]);
                last_driver_id_arr.add(drv_id);

                laptime_list_last[i].convert_string_penalty(penalty_time_str, sizeof(penalty_time_str));
                last_pen_time_arr.add(penalty_time_str);

                snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_list_last[i].oc_count);
                last_pen_oc_arr.add(penalty_count_str);

                snprintf(penalty_count_str, sizeof(penalty_count_str), "%u", laptime_list_last[i].doo_count);
                last_pen_doo_arr.add(penalty_count_str);

                // Top
                laptime_list_top[i].convert_string_full(laptime_current_str, sizeof(laptime_current_str));
                top_arr.add(laptime_current_str);

                drv_id = laptime_list_top[i].driver_id;
                if (drv_id < 0 || drv_id >= DRIVER_MAX_COUNT)
                    drv_id = 0;
                top_driver_tag_arr.add(driver_list_local.list[drv_id]);
                top_driver_id_arr.add(drv_id);
            }

            xSemaphoreGive(laptime_lists_mutex);
        }

        xSemaphoreGive(data_mutex);
    }

    httpd_resp_set_type(req, "application/json");
    HttpdChunkWriter writer(req);
    serializeJson(root, writer);
    writer.flush();
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
    JsonDocument root;
    if (xSemaphoreTake(config_mutex, portMAX_DELAY))
    {
        root["two_gate_mode"] = config_main.two_gate_mode;
        // Return current wifi mode as number (0 for AP, 1 for STA)
        root["wifi_mode"] = config_main.wifi_mode;
        root["wifi_ssid"] = config_main.wifi_ssid;
        root["wifi_password"] = config_main.wifi_password;
        root["time_set"] = config_main.time_set;
        root["date_set"] = config_main.date_set;

        JsonArray drivers = root["driver_list"].to<JsonArray>();
        // Driver list starts from 1, index 0 is placeholder
        for (int i = 1; i <= config_main.driver_list.driver_count; i++)
        {
            drivers.add(config_main.driver_list.list[i]);
        }
        xSemaphoreGive(config_mutex);
    }
    httpd_resp_set_type(req, "application/json");
    HttpdChunkWriter writer(req);
    serializeJson(root, writer);
    writer.flush();
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

    JsonDocument root;
    DeserializationError error = deserializeJson(root, buf);
    if (error)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (xSemaphoreTake(config_mutex, portMAX_DELAY))
    {
        if (root["two_gate_mode"].is<bool>())
            config_main.two_gate_mode = root["two_gate_mode"];

        if (root["wifi_mode"].is<int>())
        {
            config_main.wifi_mode = (root["wifi_mode"].as<int>() == 1) ? WIFI_MODE_STA : WIFI_MODE_AP;
        }
        else if (root["wifi_mode"].is<bool>())
        {
            config_main.wifi_mode = root["wifi_mode"].as<bool>() ? WIFI_MODE_STA : WIFI_MODE_AP;
        }

        if (root["wifi_ssid"].is<const char*>())
        {
            strncpy(config_main.wifi_ssid, root["wifi_ssid"].as<const char*>(), sizeof(config_main.wifi_ssid) - 1);
            config_main.wifi_ssid[sizeof(config_main.wifi_ssid) - 1] = '\0';
        }

        if (root["wifi_password"].is<const char*>())
        {
            strncpy(config_main.wifi_password, root["wifi_password"].as<const char*>(), sizeof(config_main.wifi_password) - 1);
            config_main.wifi_password[sizeof(config_main.wifi_password) - 1] = '\0';
        }

        if (root["time_set"].is<const char*>())
        {
            strncpy(config_main.time_set, root["time_set"].as<const char*>(), sizeof(config_main.time_set) - 1);
            config_main.time_set[sizeof(config_main.time_set) - 1] = '\0';
        }

        if (root["date_set"].is<const char*>())
        {
            strncpy(config_main.date_set, root["date_set"].as<const char*>(), sizeof(config_main.date_set) - 1);
            config_main.date_set[sizeof(config_main.date_set) - 1] = '\0';
        }

        system_set_time(config_main.time_set, config_main.date_set);

        if (root["driver_list"].is<JsonArray>())
        {
            JsonArray drivers = root["driver_list"].as<JsonArray>();
            int count = drivers.size();
            // Limit to max count (minus 1 for the placeholder at index 0)
            if (count > DRIVER_MAX_COUNT - 1)
                count = DRIVER_MAX_COUNT - 1;

            config_main.driver_list.driver_count = count;

            // Clear existing list beyond count if needed, or just overwrite
            // We overwrite from index 1
            for (int i = 0; i < count; i++)
            {
                if (drivers[i].is<const char*>())
                {
                    strncpy(config_main.driver_list.list[i + 1], drivers[i].as<const char*>(), DRIVER_TAG_LENGTH - 1);
                    config_main.driver_list.list[i + 1][DRIVER_TAG_LENGTH - 1] = '\0';
                }
            }
        }
        xSemaphoreGive(config_mutex);
    }
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

    ESP_ERROR_CHECK(wifi_init());
    start_webserver();

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

            if (wifi_restart(wifi_mode_flag, wifi_ssid_local, wifi_password_local) == ESP_OK)
            {
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
        }
#ifdef CONFIG_TWO_GATE_WIRELESS_MASTER

        // Get ip and send to lcd task to display on screen
        if (ip_refresh_flag == true)
        {
            if (wifi_get_ip(ip_str) == ESP_OK && xQueueSend(ip_queue, ip_str, portMAX_DELAY))
            {
                ip_refresh_flag = false;
            }
        }

#endif

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}