#include "wifi_task.h"

#include "wifi_ap.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "WIFI_TASK";

/**
 * @brief Flag activated by semaphore received from laptimer_task on every laptime lists update
 */
bool new_lists_flag = true;

/**
 * @brief Current laptime string used to display on website, readed periodically by data_get_handler
 */
static char current_laptime[LAPTIME_STRING_LENGTH] = "--,--:--:--";

/**
 * @brief Status flags used to display on website, readed periodically by data_get_handler
 */
static bool status_flags[3] = {false, false, false};

/**
 * @brief Mutex usde by data_get_handler to ensure that it can read data safely
 */
static SemaphoreHandle_t data_mutex = NULL;

/* HTML Content */
static const char *index_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: monospace; background: #111; color: #eee; text-align: center; }
.val { font-size: 2em; font-weight: bold; }
.label { color: #aaa; }
.status { margin: 10px; }
.status span { padding: 5px; margin: 5px; border: 1px solid #444; }
.on { background: #0a0; color: #fff; }
.off { background: #333; color: #888; }
table { margin: 0 auto; width: 90%; max-width: 600px; }
td { padding: 5px; border-bottom: 1px solid #333; }
</style>
</head>
<body>
<h1>PUTM Laptimer</h1>
<div>
  <div class="label">CURRENT LAP</div>
  <div class="val" id="curr">--,--:--:--</div>
</div>
<div class="status">
  <span id="mode">1 GATE</span>
  <span id="stop">STOP</span>
  <span id="sd">SD</span>
</div>
<table>
<thead><tr><th>LAST 5</th><th>TOP 5</th></tr></thead>
<tbody id="lists"></tbody>
</table>
<script>
function update() {
  fetch('/api/data').then(r => r.json()).then(d => {
    document.getElementById('curr').innerText = d.current;
    document.getElementById('mode').innerText = d.status.mode ? "2 GATE" : "1 GATE";
    document.getElementById('stop').className = d.status.stop ? "on" : "off";
    document.getElementById('sd').className = d.status.sd ? "on" : "off";
    
    let html = "";
    if (d.last && d.top) {
        for(let i=0; i<15; i++) {
            html += `<tr><td>${d.last[i] || '-'}</td><td>${d.top[i] || '-'}</td></tr>`;
        }
    }
    document.getElementById('lists').innerHTML = html;
  });
}
setInterval(update, 200);
</script>
</body>
</html>
)rawliteral";

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

    if (xSemaphoreTake(data_mutex, portMAX_DELAY))
    {
        cJSON_AddStringToObject(root, "current", current_laptime);

        cJSON *status = cJSON_CreateObject();
        cJSON_AddBoolToObject(status, "mode", status_flags[0]);
        cJSON_AddBoolToObject(status, "stop", status_flags[1]);
        cJSON_AddBoolToObject(status, "sd", status_flags[2]);
        cJSON_AddItemToObject(root, "status", status);

        cJSON *last_arr = cJSON_CreateArray();
        cJSON *top_arr = cJSON_CreateArray();

        if (new_lists_flag == true)
        {
            for (int i = 0; i < LAPTIME_LIST_SIZE_WIFI; i++)
            {
                cJSON_AddItemToArray(last_arr, cJSON_CreateString(wifi_list_buffer[1][i]));
                cJSON_AddItemToArray(top_arr, cJSON_CreateString(wifi_list_buffer[0][i]));
            }
            new_lists_flag = false;
            xSemaphoreGive(wifi_laptime_lists_semaphore);
        }
        cJSON_AddItemToObject(root, "last", last_arr);
        cJSON_AddItemToObject(root, "top", top_arr);

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
 * @brief Initialization of web server
 * @return Handle to http instance
 */
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
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
    wifi_init_softap();
    start_webserver();

    char temp_laptime[LAPTIME_STRING_LENGTH];
    bool temp_status[3];

    data_mutex = xSemaphoreCreateMutex();

    for (;;)
    {
        if (xQueueReceive(wifi_laptime_current_queue, temp_laptime, 0) == pdTRUE)
        {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY))
            {
                strncpy(current_laptime, temp_laptime, LAPTIME_STRING_LENGTH);
                xSemaphoreGive(data_mutex);
            }
        }

        if (xQueueReceive(wifi_laptime_status_queue, temp_status, 0) == pdTRUE)
        {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY))
            {
                memcpy(status_flags, temp_status, sizeof(status_flags));
                xSemaphoreGive(data_mutex);
            }
        }

        if (xSemaphoreTake(wifi_laptime_lists_semaphore, 0) == pdTRUE && new_lists_flag == false)
        {
            new_lists_flag = true;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
    vTaskDelete(NULL);
}
