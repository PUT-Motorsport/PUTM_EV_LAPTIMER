#include "lcd_task.h"

#include "lcd.h"
#include "lvgl_ui.h"
#include "esp_lvgl_port.h"

/**
 * @brief LCD task initializes screen and graphics library,
 * then prints values received from laptimer_task on screen
 */
void lcd_task(void *args)
{
    static Driver_list driver_list_local;
    lv_disp_t *disp = lcd_init();

    lvgl_port_lock(0);
    ui_init();
    lvgl_port_unlock();

    bool status_list[3] = {0};
    static char ip_str[52] = "0.0.0.0";
    static char gate_str[8];
    static char stop_str[5];
    static char sd_str[7];
    static char wifi_str[9];
    wifi_mode_t wifi_mode = WIFI_MODE_NULL;

    for (;;)
    {
        if (xSemaphoreTake(config_mutex, 0) == pdTRUE)
        {
            memcpy(driver_list_local.list, config_main.driver_list.list, sizeof(driver_list_local));
            driver_list_local.driver_count = config_main.driver_list.driver_count;
            xSemaphoreGive(config_mutex);
        }

        // Update Status
        bool update_status = false;
        if (xQueueReceive(laptime_status_queue_lcd, status_list, 0) == pdTRUE)
        {
            update_status = true;
        }
        if (xQueueReceive(ip_queue, ip_str, 0) == pdTRUE)
        {
            update_status = true;
        }
        if (xQueueReceive(wifi_mode_queue, &wifi_mode, 0) == pdTRUE)
        {
            update_status = true;
        }

        if (update_status)
        {
            lvgl_port_lock(0);
            ui_update_status(sd_active_flag, (int)wifi_mode, ip_str, status_list[0], status_list[1]);
            lvgl_port_unlock();
        }

        // Update Current Laptime
        static Laptime laptime_current;
        if (xQueueReceive(laptime_current_queue_lcd, &laptime_current, 0) == pdTRUE)
        {
            char laptime_current_str[LAPTIME_STR_LENGTH] = {0};
            char laptime_penalty_str[PENALTY_TIME_STR_LENGTH] = {0};

            laptime_current.convert_string_full(laptime_current_str, sizeof(laptime_current_str));
            laptime_current.convert_string_penalty(laptime_penalty_str, sizeof(laptime_penalty_str));

            lvgl_port_lock(0);
            ui_update_current_lap(laptime_current_str,
                                  laptime_penalty_str,
                                  laptime_current.oc_count,
                                  laptime_current.doo_count,
                                  driver_list_local.list[laptime_current.driver_id]);
            lvgl_port_unlock();
        }

        // Update Laptime Lists
        if (xSemaphoreTake(laptime_lists_mutex, 0) == pdTRUE)
        {
            char laptime_top_str[LAPTIME_STR_LENGTH] = {0};
            char laptime_last_str[LAPTIME_STR_LENGTH] = {0};

            lvgl_port_lock(0);
            for (int i = 0; i < LAPTIME_LIST_SIZE_LCD; i++)
            {
                laptime_list_top[i].convert_string_full(laptime_top_str, sizeof(laptime_top_str));
                laptime_list_last[i].convert_string_full(laptime_last_str, sizeof(laptime_last_str));

                ui_update_top_lap(i, laptime_top_str);
                ui_update_last_lap(i, laptime_last_str);
            }
            lvgl_port_unlock();
            xSemaphoreGive(laptime_lists_mutex);
        }

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}