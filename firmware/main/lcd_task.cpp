#include "lcd_task.h"

#include "lcd.h"
#include "lvgl_ui.h"

#include "esp_lvgl_port.h"

const char *TAG = "LCD_TASK";

/**
 * @brief LCD task initializes screen and graphics library,
 * then prints values received from laptimer_task on screen
 */
void lcd_task(void *args)
{
    static Driver_list driver_list_local;
    ESP_ERROR_CHECK(lcd_init());

    lvgl_port_lock(0);
    ui_init();
    lvgl_port_unlock();

    static char ip_str[WIFI_IP_LENGTH] = "WAIT FOR IP";

    static bool sd_active_old = sd_active_flag;
    static wifi_mode_t wifi_mode_old = wifi_mode_flag;
    static bool two_gate_old = config_main.two_gate_mode;
    static bool stop_old = stop_flag;

    bool update_status = true;

    for (;;)
    {
        // Update driver list from config
        if (driver_list_local.driver_count != config_main.driver_list.driver_count)
        {
            if (xSemaphoreTake(config_mutex, 0) == pdTRUE)
            {
                memcpy(&driver_list_local, &config_main.driver_list, sizeof(driver_list_local));
                xSemaphoreGive(config_mutex);
            }
        }

        // Update Status
        if (sd_active_flag != sd_active_old)
        {
            sd_active_old = sd_active_flag;
            update_status = true;
        }
        if (wifi_mode_flag != wifi_mode_old)
        {
            wifi_mode_old = wifi_mode_flag;
            update_status = true;
        }
        if (config_main.two_gate_mode != two_gate_old)
        {
            two_gate_old = config_main.two_gate_mode;
            update_status = true;
        }
        if (stop_flag != stop_old)
        {
            stop_old = stop_flag;
            update_status = true;
        }
        if (xQueueReceive(ip_queue, ip_str, 0) == pdTRUE)
        {
            update_status = true;
        }

        if (update_status)
        {
            lvgl_port_lock(0);
            ui_update_status(sd_active_old, wifi_mode_old, ip_str, two_gate_old, stop_old);
            lvgl_port_unlock();
            update_status = false;
        }

        // Update Current Laptime
        static Laptime laptime_current;
        if (xQueueReceive(laptime_current_queue_lcd, &laptime_current, 0) == pdTRUE)
        {
            char laptime_count_str[COUNT_STR_LENGTH] = {0};
            char laptime_current_str[LAPTIME_STR_LENGTH] = {0};
            char laptime_penalty_str[PENALTY_TIME_STR_LENGTH] = {0};

            laptime_current.convert_string_count(laptime_count_str, sizeof(laptime_count_str));
            laptime_current.convert_string_time(laptime_current_str, sizeof(laptime_current_str));
            laptime_current.convert_string_penalty(laptime_penalty_str, sizeof(laptime_penalty_str));

            lvgl_port_lock(0);
            ui_update_current_lap(laptime_current_str,
                                  laptime_penalty_str,
                                  laptime_current.oc_count,
                                  laptime_current.doo_count,
                                  driver_list_local.list[laptime_current.driver_id], laptime_current.driver_id);
            lvgl_port_unlock();
        }

        // Update Laptime Lists
        if (lists_refresh_lcd_flag == true)
        {
            if (xSemaphoreTake(laptime_lists_mutex, 0) == pdTRUE)
            {
                char lap_count_top_str[COUNT_STR_LENGTH] = {0};
                char laptime_top_str[LAPTIME_STR_LENGTH] = {0};

                char lap_count_last_str[COUNT_STR_LENGTH] = {0};
                char laptime_last_str[LAPTIME_STR_LENGTH] = {0};

                lvgl_port_lock(0);
                for (int i = 0; i < LAPTIME_LIST_SIZE_LCD; i++)
                {
                    laptime_list_top[i].convert_string_count(lap_count_top_str, sizeof(lap_count_top_str));
                    laptime_list_top[i].convert_string_time(laptime_top_str, sizeof(laptime_top_str));

                    laptime_list_top[i].convert_string_count(lap_count_last_str, sizeof(lap_count_last_str));
                    laptime_list_last[i].convert_string_time(laptime_last_str, sizeof(laptime_last_str));

                    ui_update_top_lap(i + 1, lap_count_top_str, laptime_top_str, driver_list_local.list[laptime_list_top[i].driver_id], laptime_list_top[i].driver_id);
                    ui_update_last_lap(i + 1, lap_count_last_str, laptime_last_str, driver_list_local.list[laptime_list_last[i].driver_id], laptime_list_top[i].driver_id);
                }
                lvgl_port_unlock();
                xSemaphoreGive(laptime_lists_mutex);
                lists_refresh_lcd_flag = false;
            }
        }

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}