#include "wifi_task.h"

#include "wifi_ap.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include <string.h>
#include "main.h"

static const char *TAG = "WIFI_TASK";

extern uint16_t list_driver_lap_count[DRIVER_MAX_COUNT];

/**
 * @brief Flag activated by semaphore received from laptimer_task on every laptime lists update
 */
bool new_lists_flag = true;
bool new_penalty_flag = true;

/**
 * @brief Current laptime string used to display on website, readed periodically by data_get_handler
 */
static char current_laptime[LAPTIME_STR_LENGTH] = "--,--:--:--";

/**
 * @brief Current driver ID used to display on website
 */
static int16_t current_driver_id = 0;

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
const colors = ['black', 'blue', 'red', 'yellow', 'green', 'magenta', 'brown', 'cyan'];

function update() {
  fetch('/api/data?t=' + new Date().getTime()).then(r => r.json()).then(d => {
    document.getElementById('curr').innerText = d.current;
    
    document.getElementById('curr_drv_tag').innerText = d.current_driver_tag || "XXX";
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
             if (drv === "XXX") return;
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

    if (xSemaphoreTake(data_mutex, portMAX_DELAY))
    {
        cJSON_AddStringToObject(root, "current", current_laptime);

        cJSON_AddStringToObject(root, "current_driver_tag", driver_list[current_driver_id]);
        cJSON_AddNumberToObject(root, "current_driver_id", current_driver_id);

        cJSON *drivers_arr = cJSON_CreateArray();
        cJSON *driver_best_arr = cJSON_CreateArray();
        cJSON *driver_lap_count_arr = cJSON_CreateArray();
        cJSON *driver_pen_time_arr = cJSON_CreateArray();
        cJSON *driver_pen_oc_arr = cJSON_CreateArray();
        cJSON *driver_pen_doo_arr = cJSON_CreateArray();

        for (int i = 0; i <= DRIVER_COUNT; i++)
        {
            cJSON_AddItemToArray(drivers_arr, cJSON_CreateString(driver_list[i]));
            cJSON_AddItemToArray(driver_best_arr, cJSON_CreateString(list_driver_str[i]));
            cJSON_AddItemToArray(driver_lap_count_arr, cJSON_CreateNumber(list_driver_lap_count[i]));
            cJSON_AddItemToArray(driver_pen_time_arr, cJSON_CreateString(list_driver_penalty_time_str[i]));
            cJSON_AddItemToArray(driver_pen_oc_arr, cJSON_CreateString(list_driver_penalty_oc_str[i]));
            cJSON_AddItemToArray(driver_pen_doo_arr, cJSON_CreateString(list_driver_penalty_doo_str[i]));
        }
        cJSON_AddItemToObject(root, "all_drivers", drivers_arr);
        cJSON_AddItemToObject(root, "driver_best", driver_best_arr);
        cJSON_AddItemToObject(root, "driver_lap_count", driver_lap_count_arr);
        cJSON_AddItemToObject(root, "driver_pen_time", driver_pen_time_arr);
        cJSON_AddItemToObject(root, "driver_pen_oc", driver_pen_oc_arr);
        cJSON_AddItemToObject(root, "driver_pen_doo", driver_pen_doo_arr);

        cJSON *status = cJSON_CreateObject();
        cJSON_AddBoolToObject(status, "mode", status_flags[0]);
        cJSON_AddBoolToObject(status, "stop", status_flags[1]);
        cJSON_AddBoolToObject(status, "sd", status_flags[2]);
        cJSON_AddItemToObject(root, "status", status);

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
            cJSON_AddItemToArray(last_arr, cJSON_CreateString(list_last_str[i]));
            cJSON_AddItemToArray(top_arr, cJSON_CreateString(list_top_str[i]));

            cJSON_AddItemToArray(last_driver_tag_arr, cJSON_CreateString(driver_list[list_last_driver_id[i]]));
            cJSON_AddItemToArray(last_driver_id_arr, cJSON_CreateNumber(list_last_driver_id[i]));

            cJSON_AddItemToArray(top_driver_tag_arr, cJSON_CreateString(driver_list[list_top_driver_id[i]]));
            cJSON_AddItemToArray(top_driver_id_arr, cJSON_CreateNumber(list_top_driver_id[i]));

            cJSON_AddItemToArray(last_pen_time_arr, cJSON_CreateString(list_penalty_time_str[i]));
            cJSON_AddItemToArray(last_pen_oc_arr, cJSON_CreateString(list_penalty_oc_str[i]));
            cJSON_AddItemToArray(last_pen_doo_arr, cJSON_CreateString(list_penalty_doo_str[i]));
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

        cJSON_AddStringToObject(root, "penalty_time", current_penalty_time_str);
        cJSON_AddStringToObject(root, "penalty_oc", current_penalty_oc_str);
        cJSON_AddStringToObject(root, "penalty_doo", current_penalty_doo_str);

        // Reset flags if needed, although we are not gating on them anymore for response construction
        if (new_lists_flag)
        {
            new_lists_flag = false;
        }
        if (new_penalty_flag)
        {
            new_penalty_flag = false;
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

    if (xSemaphoreTake(data_mutex, portMAX_DELAY))
    {
        char line[128];
        for (int i = 0; i < LAPTIME_LIST_SIZE_WIFI; i++)
        {
            // Using last lap list for CSV as per logic in previous version (implied by using list_last_str)
            snprintf(line, sizeof(line), "%s,%s,%s,%s,%s\n",
                     list_last_str[i],
                     driver_list[list_last_driver_id[i]],
                     list_penalty_time_str[i],
                     list_penalty_oc_str[i],
                     list_penalty_doo_str[i]);
            httpd_resp_sendstr_chunk(req, line);
        }
        xSemaphoreGive(data_mutex);
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
    httpd_resp_sendstr_chunk(req, "Driver,Best lap number, Best laptime,Lap count ,Penalty time sum,OC sum,DOO sum\n");

    if (xSemaphoreTake(data_mutex, portMAX_DELAY))
    {
        char line[128];
        for (int i = 1; i <= DRIVER_COUNT; i++)
        {
            snprintf(line, sizeof(line), "%s,%s,%u,%s,%s,%s\n",
                     driver_list[i],
                     list_driver_str[i],
                     list_driver_lap_count[i],
                     list_driver_penalty_time_str[i],
                     list_driver_penalty_oc_str[i],
                     list_driver_penalty_doo_str[i]);
            httpd_resp_sendstr_chunk(req, line);
        }
        xSemaphoreGive(data_mutex);
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

    char temp_laptime[LAPTIME_STR_LENGTH];
    int32_t temp_driver_id_buf; // Buffer to safely receive potentially 4 bytes from queue

    data_mutex = xSemaphoreCreateMutex();

    for (;;)
    {
        if (xQueueReceive(wifi_laptime_current_queue, temp_laptime, 0) == pdTRUE)
        {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY))
            {
                strncpy(current_laptime, temp_laptime, LAPTIME_STR_LENGTH);
                xSemaphoreGive(data_mutex);
            }
        }

        // Receive driver ID - handle potential pointer/value size mismatch safe-ishly
        // The queue was created with size 4 (pointer size), but sender sends &int16_t.
        // We read 4 bytes into a buffer and cast.
        if (xQueueReceive(wifi_laptime_driver_queue, &temp_driver_id_buf, 0) == pdTRUE)
        {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY))
            {
                current_driver_id = (int16_t)temp_driver_id_buf;
                // Boundary check
                if (current_driver_id < 0 || current_driver_id > DRIVER_COUNT)
                {
                    current_driver_id = 0;
                }
                xSemaphoreGive(data_mutex);
            }
        }

        if (xSemaphoreTake(wifi_laptime_status_semaphore, 0) == pdTRUE)
        {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY))
            {
                status_flags[0] = (lap_mode);
                status_flags[1] = stop_flag;
                status_flags[2] = sd_active_flag;
                xSemaphoreGive(data_mutex);
            }
            xSemaphoreGive(wifi_laptime_status_semaphore);
        }

        if (xSemaphoreTake(wifi_laptime_lists_semaphore, 0) == pdTRUE && new_lists_flag == false)
        {
            new_lists_flag = true;
        }

        if (xSemaphoreTake(wifi_laptime_penalty_semaphore, 0) == pdTRUE && new_penalty_flag == false)
        {
            new_penalty_flag = true;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}