#include "lcd_task.hpp"

#include "lcd.h"
#include "lvgl_ui.h"

#include "esp_lvgl_port.h"

/// @brief Size of lcd displayed best/last laptime list
#define LAPTIME_LIST_SIZE_LCD 5

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

    char ip_str[WIFI_IP_LENGTH] = "WAIT FOR IP";

    bool sd_active_old = !sd_active_flag;
    wifi_mode_t wifi_mode_old = WIFI_MODE_NULL;
    bool two_gate_old = !config_main.two_gate_mode;
    bool stop_old = !stop_flag;

    static Laptime laptime_current;

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
            lvgl_port_lock(0);
            ui_update_sd(sd_active_old);
            lvgl_port_unlock();
        }
        if (wifi_mode_flag != wifi_mode_old || xQueueReceive(ip_queue, ip_str, 0) == pdTRUE)
        {
            wifi_mode_old = wifi_mode_flag;
            lvgl_port_lock(0);
            ui_update_wifi(wifi_mode_old, ip_str);
            lvgl_port_unlock();
        }
        if (config_main.two_gate_mode != two_gate_old)
        {
            two_gate_old = config_main.two_gate_mode;
            lvgl_port_lock(0);
            ui_update_gates_mode(two_gate_old);
            lvgl_port_unlock();
        }
        if (stop_flag != stop_old)
        {
            stop_old = stop_flag;
            lvgl_port_lock(0);
            ui_update_stop_status(stop_old);
            lvgl_port_unlock();
        }

        // Update Current Laptime
        if (xQueueReceive(laptime_current_queue_lcd, &laptime_current, 0) == pdTRUE)
        {
            char laptime_current_str[LAPTIME_STR_LENGTH] = {0};
            char laptime_penalty_str[PENALTY_TIME_STR_LENGTH] = {0};

            laptime_current.convert_string_time(laptime_current_str, sizeof(laptime_current_str));
            laptime_current.convert_string_penalty(laptime_penalty_str, sizeof(laptime_penalty_str));

            lvgl_port_lock(0);
            ui_update_current_lap(laptime_current_str,
                                  laptime_penalty_str,
                                  laptime_current.count,
                                  laptime_current.oc_count,
                                  laptime_current.doo_count,
                                  driver_list_local.list.at(laptime_current.driver_id), laptime_current.driver_id);
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
                    laptime_list_top.at(i).convert_string_count(lap_count_top_str, sizeof(lap_count_top_str));
                    laptime_list_top.at(i).convert_string_time(laptime_top_str, sizeof(laptime_top_str));

                    laptime_list_last.at(i).convert_string_count(lap_count_last_str, sizeof(lap_count_last_str));
                    laptime_list_last.at(i).convert_string_time(laptime_last_str, sizeof(laptime_last_str));

                    ui_update_top_lap(i + 1, lap_count_top_str, laptime_top_str, driver_list_local.list[laptime_list_top.at(i).driver_id], laptime_list_top.at(i).driver_id);
                    ui_update_last_lap(i + 1, lap_count_last_str, laptime_last_str, driver_list_local.list[laptime_list_last.at(i).driver_id], laptime_list_last.at(i).driver_id);
                }
                lvgl_port_unlock();
                xSemaphoreGive(laptime_lists_mutex);
                lists_refresh_lcd_flag = false;
            }
        }

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}