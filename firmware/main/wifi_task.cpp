#include "wifi_task.h"

#include "wifi_ap.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include <string.h>
#include "main.h"

static const char *TAG = "WIFI_TASK";

/**
 * @brief Current laptime data used to display on website
 */
static Laptime current_laptime_data;

static Driver_list driver_list_local;

/**
 * @brief Status flags used to display on website
 */
static bool status_flags[3] = {false, true, false};

/**
 * @brief Mutex used by data_get_handler to ensure that it can read local data safely
 */
static SemaphoreHandle_t data_mutex = NULL;

/* HTML Content */
static const char *index_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: monospace; background: #111; color: #eee; margin: 0; padding: 10px; font-size: 16px; }
h1 { text-align: center; margin-top: 10px; }
.container { display: flex; flex-wrap: wrap; justify-content: center; gap: 20px; max-width: 1400px; margin: 0 auto; }
.main-panel { flex: 1; min-width: 300px; max-width: 900px; text-align: center; }
.side-panel { flex: 0 0 500px; background: #222; padding: 15px; border-radius: 8px; height: fit-content; text-align: left; border: 1px solid #444; }
.val { font-size: 3em; font-weight: bold; }
.label { color: #aaa; font-size: 1.2em; margin-bottom: 5px; }
.status { margin: 20px; font-size: 1.2em; }
.status span { padding: 8px 15px; margin: 5px; border: 1px solid #444; display: inline-block; border-radius: 4px; }
.on { background: #0a0; color: #fff; }
.off { background: #333; color: #888; }
.tables-wrapper { display: flex; flex-direction: row; flex-wrap: wrap; gap: 20px; margin-top: 20px; justify-content: center; }
.table-box { flex: 1; min-width: 300px; background: #1a1a1a; padding: 10px; border-radius: 5px; border: 1px solid #333; }
.table-title { font-size: 1.5em; color: #fff; margin-bottom: 10px; text-align: center; border-bottom: 1px solid #444; padding-bottom: 5px; }
table { width: 100%; font-size: 1.1em; border-collapse: collapse; }
th, td { padding: 8px; border-bottom: 1px solid #333; text-align: center; }
th { color: #aaa; }
.penalty { color: #ffeb3b; }
.penalty-val { font-size: 2em; font-weight: bold; color: #ffeb3b; }
.pen-col { color: #ffeb3b; }
.driver-color-box { display: inline-block; width: 20px; height: 20px; border-radius: 4px; vertical-align: middle; border: 2px solid #fff; margin-right: 10px; }
.current-container { display: flex; justify-content: center; align-items: baseline; gap: 20px; flex-wrap: wrap; }
.download-btn { padding: 15px 30px; font-size: 1.2em; background: #333; color: #fff; border: 1px solid #444; cursor: pointer; margin: 5px 0; width: 100%; border-radius: 4px; display: block; text-decoration: none; text-align: center;}
.legend-item { font-size: 1.1em; display: flex; align-items: center; padding: 8px 0; border-bottom: 1px solid #333; }
.legend-item:last-child { border-bottom: none; }
h3 { margin-top: 0; color: #ccc; border-bottom: 1px solid #555; padding-bottom: 10px; text-align: center; }
/* Info Table Styles */
.info-table { margin: 20px auto; width: auto; min-width: 60%; background: #222; border-radius: 5px; overflow: hidden; }
.info-table th { background: #333; padding: 10px 20px; }
.info-table td { padding: 10px 20px; font-size: 1.3em; font-weight: bold; }
@media (max-width: 800px) {
    .container { flex-direction: column; }
    .side-panel { width: auto; flex: none; order: 2; }
    .main-panel { order: 1; }
    .tables-wrapper { flex-direction: column; }
}
</style>
</head>
<body>
<h1>PUTM Laptimer</h1>
<div class="container">

  <div class="main-panel">
      <div>
            <div class="status">
        <span id="mode">1 GATE</span>
        <span id="stop">STOP</span>
        <span id="sd">SD</span>
      </div>
        <div class="label">CURRENT LAP</div>
        <div class="current-container">
            <div class="val" id="curr">--,--:--:--</div>
            <div class="penalty-val" id="pen_time">+00:00</div>
        </div>
      </div>

      <!-- Driver & Penalty Info Table -->
      <table class="info-table">
        <thead>
            <tr>
                <th>DRIVER</th>
                <th class="penalty">OC</th>
                <th class="penalty">DOO</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td>
                    <span id="curr_drv_tag">XXX</span>
                    <span id="curr_drv_col" class="driver-color-box" style="background: black; margin-left: 10px;"></span>
                </td>
                <td class="penalty" id="pen_oc">0</td>
                <td class="penalty" id="pen_doo">0</td>
            </tr>
        </tbody>
      </table>

      <div class="tables-wrapper">
          <!-- LAST LAPS TABLE -->
          <div class="table-box">
            <div class="table-title">LAST LAPS</div>
            <table>
                <thead>
                    <tr>
                        <th>Time</th>
                        <th>Driver</th>
                        <th>Pen.</th>
                        <th>OC</th>
                        <th>DOO</th>
                    </tr>
                </thead>
                <tbody id="list_last"></tbody>
            </table>
          </div>

          <!-- TOP LAPS TABLE -->
          <div class="table-box">
            <div class="table-title">TOP LAPS</div>
            <table>
                <thead>
                    <tr>
                        <th>Time</th>
                        <th>Driver</th>
                    </tr>
                </thead>
                <tbody id="list_top"></tbody>
            </table>
          </div>
      </div>

  </div>

  <div class="side-panel">
     <h3>DRIVER LIST</h3>
     <div id="driver_legend"></div>
     <div style="margin-top: 20px;">
        <form action="/api/csv" method="get">
          <button type="submit" class="download-btn">DOWNLOAD LAPS CSV</button>
        </form>
        <form action="/api/drivers_csv" method="get">
          <button type="submit" class="download-btn">DOWNLOAD DRIVERS CSV</button>
        </form>
     </div>
  </div>

</div>

<script>
const colors = ['black', 'blue', 'red', 'yellow', 'green', 'magenta', 'brown', 'cyan', 'purple', 'olive'];

function update() {
  fetch('/api/data?t=' + new Date().getTime()).then(r => r.json()).then(d => {
    document.getElementById('curr').innerText = d.current;

    document.getElementById('curr_drv_tag').innerText = d.current_driver_tag || "---";
    let currColorIdx = d.current_driver_id || 0;
    if(currColorIdx < 0 || currColorIdx >= colors.length) currColorIdx = 0;
    document.getElementById('curr_drv_col').style.background = colors[currColorIdx];

    document.getElementById('pen_time').innerText = d.penalty_time || "+00:00";
    document.getElementById('pen_oc').innerText = (d.penalty_oc || "0");
    document.getElementById('pen_doo').innerText = (d.penalty_doo || "0");

    document.getElementById('mode').innerText = d.status.mode ? "2 GATE" : "1 GATE";
    document.getElementById('stop').className = d.status.stop ? "on" : "off";
    document.getElementById('sd').className = d.status.sd ? "on" : "off";

    if (d.all_drivers) {
        let legendHtml = "<table><thead><tr><th>Driver</th><th>Best</th><th>Laps</th><th>Pen.</th><th>OC</th><th>DOO</th></tr></thead><tbody>";
        d.all_drivers.forEach((drv, i) => {
             // Skip placeholder only if it matches standard placeholder
             if (drv === "---") return;
             let color = colors[i] || 'black';

             let best = (d.driver_best && d.driver_best[i]) ? d.driver_best[i] : '-';
             let laps = (d.driver_lap_count && d.driver_lap_count[i] !== undefined) ? d.driver_lap_count[i] : '0';
             let p_time = (d.driver_pen_time && d.driver_pen_time[i]) ? d.driver_pen_time[i] : '0';
             let p_oc = (d.driver_pen_oc && d.driver_pen_oc[i]) ? d.driver_pen_oc[i] : '0';
             let p_doo = (d.driver_pen_doo && d.driver_pen_doo[i]) ? d.driver_pen_doo[i] : '0';

             legendHtml += `<tr>
                <td style=\"text-align: left;\">
                    <span class=\"driver-color-box\" style=\"background:${color}; margin-right:5px;\"></span>${drv}
                </td>
                <td>${best}</td>
                <td>${laps}</td>
                <td class=\"pen-col\">${p_time}</td>
                <td class=\"pen-col\">${p_oc}</td>
                <td class=\"pen-col\">${p_doo}</td>
             </tr>`;
        });
        legendHtml += "</tbody></table>";
        document.getElementById('driver_legend').innerHTML = legendHtml;
    }

    // Update LAST list
    let lastHtml = "";
    if (d.last) {
        for(let i=0; i<d.last.length; i++) {
            let t = d.last[i] || '-';
            let drv = (d.last_driver_tag && d.last_driver_tag[i]) ? d.last_driver_tag[i] : '';
            let drv_id = (d.last_driver_id && d.last_driver_id[i] !== undefined) ? d.last_driver_id[i] : 0;
            let p_time = (d.last_pen_time && d.last_pen_time[i]) ? d.last_pen_time[i] : '';
            let p_oc = (d.last_pen_oc && d.last_pen_oc[i]) ? d.last_pen_oc[i] : '';
            let p_doo = (d.last_pen_doo && d.last_pen_doo[i]) ? d.last_pen_doo[i] : '';

            if (t === '-' || t.includes('--:--.--')) continue;

            let color = colors[drv_id] || 'black';
            lastHtml += `<tr>
                <td>${t}</td>
                <td>${drv}<span class=\"driver-color-box\" style=\"background:${color}; margin-left:5px; margin-right:0;\"></span></td>
                <td class=\"pen-col\">${p_time}</td>
                <td class=\"pen-col\">${p_oc}</td>
                <td class=\"pen-col\">${p_doo}</td>
            </tr>`;
        }
    }
    document.getElementById('list_last').innerHTML = lastHtml;

    // Update TOP list
    let topHtml = "";
    if (d.top) {
        for(let i=0; i<d.top.length; i++) {
            let t = d.top[i] || '-';
            let drv = (d.top_driver_tag && d.top_driver_tag[i]) ? d.top_driver_tag[i] : '';
            let drv_id = (d.top_driver_id && d.top_driver_id[i] !== undefined) ? d.top_driver_id[i] : 0;

            if (t === '-' || t.includes('--:--.--')) continue;

            let color = colors[drv_id] || 'black';
            topHtml += `<tr>
                <td>${t}</td>
                <td>${drv}<span class=\"driver-color-box\" style=\"background:${color}; margin-left:5px; margin-right:0;\"></span></td>
            </tr>`;
        }
    }
    document.getElementById('list_top').innerHTML = topHtml;
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

    char temp_str[LAPTIME_STR_LENGTH] = {0};
    char temp_pen_str[PENALTY_TIME_STR_LENGTH] = {0};
    char temp_count_str[PENALTY_COUNT_STR_LENGTH] = {0};

    // Take local mutex for atomic access to local data
    if (xSemaphoreTake(data_mutex, portMAX_DELAY))
    {
        current_laptime_data.convert_string(temp_str, sizeof(temp_str));
        cJSON_AddStringToObject(root, "current", temp_str);

        int id = current_laptime_data.driver_id;
        if (id < 0 || id >= DRIVER_MAX_COUNT)
            id = 0;
        cJSON_AddStringToObject(root, "current_driver_tag", driver_list_local.list[id]);
        cJSON_AddNumberToObject(root, "current_driver_id", id);

        current_laptime_data.penalty_string(temp_pen_str, sizeof(temp_pen_str));
        cJSON_AddStringToObject(root, "penalty_time", temp_pen_str);

        snprintf(temp_count_str, sizeof(temp_count_str), "%u", current_laptime_data.oc_count);
        cJSON_AddStringToObject(root, "penalty_oc", temp_count_str);

        snprintf(temp_count_str, sizeof(temp_count_str), "%u", current_laptime_data.doo_count);
        cJSON_AddStringToObject(root, "penalty_doo", temp_count_str);

        cJSON *status = cJSON_CreateObject();
        cJSON_AddBoolToObject(status, "mode", status_flags[0]);
        cJSON_AddBoolToObject(status, "stop", status_flags[1]);
        cJSON_AddBoolToObject(status, "sd", status_flags[2]);
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

                laptime_list_driver[i].convert_string_time(temp_str, sizeof(temp_str));
                cJSON_AddItemToArray(driver_best_arr, cJSON_CreateString(temp_str));

                cJSON_AddItemToArray(driver_lap_count_arr, cJSON_CreateNumber(laptime_list_driver[i].count - 1));

                laptime_list_driver[i].penalty_string(temp_pen_str, sizeof(temp_pen_str));
                cJSON_AddItemToArray(driver_pen_time_arr, cJSON_CreateString(temp_pen_str));

                snprintf(temp_count_str, sizeof(temp_count_str), "%u", laptime_list_driver[i].oc_count);
                cJSON_AddItemToArray(driver_pen_oc_arr, cJSON_CreateString(temp_count_str));

                snprintf(temp_count_str, sizeof(temp_count_str), "%u", laptime_list_driver[i].doo_count);
                cJSON_AddItemToArray(driver_pen_doo_arr, cJSON_CreateString(temp_count_str));
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
                laptime_list_last[i].convert_string(temp_str, sizeof(temp_str));
                cJSON_AddItemToArray(last_arr, cJSON_CreateString(temp_str));

                int drv_id = laptime_list_last[i].driver_id;
                if (drv_id < 0 || drv_id >= DRIVER_MAX_COUNT)
                    drv_id = 0;
                cJSON_AddItemToArray(last_driver_tag_arr, cJSON_CreateString(driver_list_local.list[drv_id]));
                cJSON_AddItemToArray(last_driver_id_arr, cJSON_CreateNumber(drv_id));

                laptime_list_last[i].penalty_string(temp_pen_str, sizeof(temp_pen_str));
                cJSON_AddItemToArray(last_pen_time_arr, cJSON_CreateString(temp_pen_str));

                snprintf(temp_count_str, sizeof(temp_count_str), "%u", laptime_list_last[i].oc_count);
                cJSON_AddItemToArray(last_pen_oc_arr, cJSON_CreateString(temp_count_str));

                snprintf(temp_count_str, sizeof(temp_count_str), "%u", laptime_list_last[i].doo_count);
                cJSON_AddItemToArray(last_pen_doo_arr, cJSON_CreateString(temp_count_str));

                // Top
                laptime_list_top[i].convert_string(temp_str, sizeof(temp_str));
                cJSON_AddItemToArray(top_arr, cJSON_CreateString(temp_str));

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
static esp_err_t csv_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"laptimes.csv\"");

    // Header
    httpd_resp_sendstr_chunk(req, "Number,Driver,Laptime [mm:ss:ms],Penalty Time [mm:ss],OC,DOO\n");

    if (xSemaphoreTake(laptime_lists_mutex, portMAX_DELAY))
    {
        char line[128];
        char time_str[LAPTIME_STR_LENGTH];
        char pen_str[PENALTY_TIME_STR_LENGTH];

        for (int i = 0; i < LAPTIME_LIST_SIZE_WIFI; i++)
        {
            laptime_list_last[i].convert_string(time_str, sizeof(time_str));
            laptime_list_last[i].penalty_string(pen_str, sizeof(pen_str));

            int drv_id = laptime_list_last[i].driver_id;
            if (drv_id < 0 || drv_id >= DRIVER_MAX_COUNT)
                drv_id = 0;

            snprintf(line, sizeof(line), "%s,%s,%s,%u,%u\n",
                     time_str,
                     driver_list_local.list[drv_id],
                     pen_str,
                     laptime_list_last[i].oc_count,
                     laptime_list_last[i].doo_count);
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
        char line[128];
        char time_str[LAPTIME_STR_LENGTH];
        char pen_str[PENALTY_TIME_STR_LENGTH];

        for (int i = 1; i <= driver_list_local.driver_count; i++)
        {
            laptime_list_driver[i].convert_string_time(time_str, sizeof(time_str));
            laptime_list_driver[i].penalty_string(pen_str, sizeof(pen_str));

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
            .handler = csv_get_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &csv);

        httpd_uri_t drivers_csv = {
            .uri = "/api/drivers_csv",
            .method = HTTP_GET,
            .handler = drivers_csv_get_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &drivers_csv);

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

    data_mutex = xSemaphoreCreateMutex();

    int driver_update_counter = 0;

    for (;;)
    {
        Laptime temp_lap;

        driver_update_counter++;
        if (driver_update_counter >= 100)
        {
            driver_update_counter = 0;
            if (xSemaphoreTake(driver_list_mutex, 0) == pdTRUE)
            {
                if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE)
                {
                    memcpy(driver_list_local.list, driver_list_main.list, sizeof(driver_list_local.list));
                    driver_list_local.driver_count = driver_list_main.driver_count;
                    xSemaphoreGive(data_mutex);
                }
                xSemaphoreGive(driver_list_mutex);
            }
        }

        if (xQueueReceive(laptime_current_queue_wifi, &temp_lap, 0) == pdTRUE)
        {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY))
            {
                current_laptime_data = temp_lap;
                xSemaphoreGive(data_mutex);
            }
        }

        bool temp_status[3];
        if (xQueueReceive(laptime_status_queue_wifi, &temp_status, 0) == pdTRUE)
        {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY))
            {
                memcpy(status_flags, temp_status, sizeof(status_flags));
                xSemaphoreGive(data_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
