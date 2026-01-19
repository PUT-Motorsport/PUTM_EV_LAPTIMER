#include "laptimer_task.h"

#include "gpio.h"
#include "main.h"
#include "sdcard.h"
#include "timer.h"

#include <stdio.h>
#include <cwchar>

static const char *TAG = "LAPTIMER_TASK";

volatile bool stop_flag = true;

/**
 * @brief Global variable used to store current laptime that is readed from timer in main loop and saved in ISR
 */
Laptime laptime_current;

/**
 * @brief Global variable used to store saved laptime after current laptime is saved in ISR,
laptime is then saved on lists and sent to other tasks
 */
Laptime laptime_saved;

Button_press doo_press;
Button_press oc_press;
Button_press driver_select_press;
Button_press wifi_press;

/**
 * @brief Saves received laptime on local best/last lists sorted
 * @param laptime Laptime to save
 * @param list Structure that stores local laptimes
 * @return
 */

esp_err_t laptime_save_top(Laptime laptime, Laptime list_top[LAPTIME_LIST_SIZE_LOCAL])
{
    if (list_top == NULL)
        return ESP_FAIL;

    for (int i = 0; i < LAPTIME_LIST_SIZE_LOCAL; i++)
    {
        if (laptime.time < list_top[i].time || list_top[i].time == 0)
        {
            for (int j = LAPTIME_LIST_SIZE_LOCAL - 1; j > i; j--)
            {
                list_top[j] = list_top[j - 1];
            }
            list_top[i] = laptime;
            break;
        }
    }
    return ESP_OK;
}

esp_err_t laptime_save_last(Laptime laptime, Laptime list_last[LAPTIME_LIST_SIZE_LOCAL])
{
    if (list_last == NULL)
        return ESP_FAIL;

    for (int i = LAPTIME_LIST_SIZE_LOCAL - 1; i > 0; i--)
    {
        list_last[i] = list_last[i - 1];
    }
    list_last[0] = laptime;
    return ESP_OK;
}

esp_err_t laptime_save_driver(Laptime laptime, Laptime list_driver[DRIVER_MAX_COUNT])
{
    if (list_driver == NULL)
        return ESP_FAIL;

    if (laptime.time < list_driver[laptime.driver_id].time || list_driver[laptime.driver_id].time == 0)
        list_driver[laptime.driver_id].time = laptime.time;
    list_driver[laptime.driver_id].driver_id = laptime.driver_id;
    list_driver[laptime.driver_id].oc_count += laptime.oc_count;
    list_driver[laptime.driver_id].doo_count += laptime.doo_count;
    list_driver[laptime.driver_id].penalty_time += laptime.penalty_time;
    list_driver[laptime.driver_id].count++;

    return ESP_OK;
}

static void laptime_save_uart(Laptime laptime)
{
    char laptime_saved_str[LAPTIME_STR_LENGTH] = LAPTIME_STR_DEFAULT;
    laptime.convert_string_full(laptime_saved_str,
                                sizeof(laptime_saved_str));
    printf("%s", laptime_saved_str);
    printf("\n");
}

button_state button_hold(bool btn_level, Button_press button_press)
{
    TickType_t tick = xTaskGetTickCount();
    switch (button_press.state)
    {
    case BTN_STANDBY:
        return BTN_STANDBY;

    case BTN_HOLD_WAIT:
        if (btn_level == 1)
            return BTN_RELEASED_ACTION;
        else if (tick - button_press.time > pdMS_TO_TICKS(1000))
            return BTN_HOLD_ACTION;
        else
            return BTN_HOLD_WAIT;

    case BTN_HOLD_ACTION:
        return BTN_AFTER_HOLD;
    case BTN_RELEASED_ACTION:
        return BTN_STANDBY;

    case BTN_AFTER_HOLD:
        if (btn_level == 1)
            return BTN_STANDBY;
        return BTN_AFTER_HOLD;

    default:
        return BTN_HOLD_WAIT;
    }
}

void penalty_check(Laptime *laptime)
{

    switch (doo_press.state = button_hold(gpio_get_level(LAP_DOO_PIN), doo_press))
    {
    case BTN_HOLD_ACTION:
        if (laptime->penalty_time >= DOO_TIME_PENALTY && laptime->doo_count > 0)
        {
            laptime->doo_count--;
            laptime->penalty_time -= DOO_TIME_PENALTY;
        }
        break;
    case BTN_RELEASED_ACTION:
        laptime->doo_count++;
        laptime->penalty_time += DOO_TIME_PENALTY;
        break;
    default:
        break;
    }

    switch (oc_press.state = button_hold(gpio_get_level(LAP_OC_PIN), oc_press))
    {
    case BTN_HOLD_ACTION:
        if (laptime->penalty_time >= OC_TIME_PENALTY && laptime->oc_count > 0)
        {
            laptime->oc_count--;
            laptime->penalty_time -= OC_TIME_PENALTY;
        }
        break;
    case BTN_RELEASED_ACTION:
        laptime->oc_count++;
        laptime->penalty_time += OC_TIME_PENALTY;
        break;
    default:
        break;
    }
    return;
}

bool driver_select(Driver_list *driver_list)
{
    switch (driver_select_press.state = button_hold(gpio_get_level(DRIVER_SELECT_PIN), driver_select_press))
    {
    case BTN_HOLD_ACTION:
        laptime_current.driver_id--;
        if (laptime_current.driver_id < 1)
            laptime_current.driver_id = driver_list->driver_count;
        return true;
    case BTN_RELEASED_ACTION:
        laptime_current.driver_id++;
        if (laptime_current.driver_id > driver_list->driver_count)
            laptime_current.driver_id = 1;
        return true;
    default:
        return false;
    }
    return false;
}

void wifi_reset_check()
{
    Wifi_reset wifi_reset_flag = WIFI_RESET_DEFAULTS;
    switch (wifi_press.state = button_hold(gpio_get_level(WIFI_PIN), wifi_press))
    {
    case BTN_HOLD_ACTION:
        wifi_reset_flag = WIFI_RESET_DEFAULTS;
        break;
    case BTN_RELEASED_ACTION:
        wifi_reset_flag = WIFI_RESET_CONFIG;
        break;
    default:
        return;
    }
    xQueueSend(wifi_reset_queue, &wifi_reset_flag, 0);
}

void IRAM_ATTR button_isr(Button_press *button_press)
{
    TickType_t tick = xTaskGetTickCountFromISR();
    if ((tick - button_press->time) > pdMS_TO_TICKS(100) && button_press->state == BTN_STANDBY)
    {
        button_press->state = BTN_HOLD_WAIT;
        button_press->time = tick;
    }
}

/**
 * @brief ISR for first gate depends on active mode
 * two_gate_mode == false - first gate saves laptime and starts new lap on every negative edge
 * two_gate_mode == true - first gate only starts new lap on negative edge if stop_flag is true
 */
void IRAM_ATTR gate1_pin_isr()
{
    laptime_current.time = timer_get_time(laptime_timer);
    if (config_main.two_gate_mode == false)
    {
        if (stop_flag == false && laptime_current.time > LAPTIME_MIN)
        {
            laptime_saved = laptime_current;
            laptime_saved.time += laptime_current.penalty_time;
            timer_reset(laptime_timer);
            laptime_current.new_lap();
        }
        else if (stop_flag == true)
        {
            stop_flag = false;
            timer_reset(laptime_timer);
        }
        return;
    }
    else
    {
        if (stop_flag == true)
        {
            stop_flag = false;
            timer_reset(laptime_timer);
        }
    }
    return;
}

/**
 * @brief ISR for second gate depends on active mode
 * two_gate_mode == false - second gate is not active
 * two_gate_mode == true - second gates saves laptime on negative edge if stop_flag is false
 */
void IRAM_ATTR gate2_pin_isr()
{
    laptime_current.time = timer_get_time(laptime_timer);
    if (config_main.two_gate_mode == false)
        return;
    else
    {
        if (stop_flag == false && laptime_current.time > LAPTIME_MIN)
        {
            laptime_saved = laptime_current;
            laptime_saved.time += laptime_current.penalty_time;
            stop_flag = true;
            laptime_current.new_lap();
        }
    }
    return;
}

/**
 * @brief ISR for reset button, resets current laptime and activates stop_flag
 */
void IRAM_ATTR reset_pin_isr()
{
    if (stop_flag == false)
    {
        stop_flag = true;
        laptime_current.reset();
    }
}

void IRAM_ATTR doo_pin_isr()
{
    button_isr(&doo_press);
}

void IRAM_ATTR oc_pin_isr()
{
    button_isr(&oc_press);
}

void IRAM_ATTR driver_select_pin_isr()
{
    button_isr(&driver_select_press);
}

void IRAM_ATTR wifi_pin_isr()
{
    button_isr(&wifi_press);
}

esp_err_t isr_init()
{
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_GATE1_PIN,
                                         (gpio_isr_t)gate1_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_GATE2_PIN,
                                         (gpio_isr_t)gate2_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_RESET_PIN,
                                         (gpio_isr_t)reset_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_DOO_PIN,
                                         (gpio_isr_t)doo_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_OC_PIN,
                                         (gpio_isr_t)oc_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(DRIVER_SELECT_PIN,
                                         (gpio_isr_t)driver_select_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(WIFI_PIN,
                                         (gpio_isr_t)wifi_pin_isr, NULL));
    ESP_LOGI("ISR", "INIT OK");
    return ESP_OK;
}

/**
 * @brief Laptimer main logic task in main loop:
 * 1. Updates local drivers list if needed
 * 2. Checks buttons ISRs
 * 3. Updates current laptime from timer if stop_flag is false
 * 4. Sends current laptime to other tasks
 * 5. Sends status flags to other tasks if changed
 * 6. When laptime is saved, stores it locally and sends through UART, sd card, shows it on LCD and WIFI page
 */
void laptimer_task(void *args)
{
    bool stop_flag_old = stop_flag;
    bool sd_active_flag_old = sd_active_flag;
    bool status_update_flag = true;
    Driver_list driver_list_local;

    xQueueSend(laptime_current_queue_lcd, &laptime_current, 0);
    xQueueSend(laptime_current_queue_wifi, &laptime_current, 0);

    penalty_check(&laptime_current);

    for (;;)
    {
        // Update driver list from config
        if (driver_list_local.driver_count != config_main.driver_list.driver_count)
        {
            if (xSemaphoreTake(config_mutex, 0) == pdTRUE)
            {
                memcpy(driver_list_local.list, config_main.driver_list.list, sizeof(driver_list_local));
                driver_list_local.driver_count = config_main.driver_list.driver_count;
                xSemaphoreGive(config_mutex);
            }
        }

        driver_select(&driver_list_local);
        wifi_reset_check();

        // Read laptime and check penalties
        if (stop_flag == false)
        {
            laptime_current.time = timer_get_time(laptime_timer);
            penalty_check(&laptime_current);
        }

        // Send laptime to lcd and wifi webpage
        xQueueSend(laptime_current_queue_lcd, &laptime_current, 0);
        xQueueSend(laptime_current_queue_wifi, &laptime_current, 0);

        // Update flags
        if (stop_flag != stop_flag_old || sd_active_flag_old != sd_active_flag)
        {
            stop_flag_old = stop_flag;
            sd_active_flag_old = sd_active_flag;
            status_update_flag = true;
        }

        // Save laptime to lists and sd card, send with uart, reset laptime
        if (laptime_saved.time > 0 && xSemaphoreTake(laptime_lists_mutex, 0) == pdTRUE)
        {
            Laptime laptime_saved_local = laptime_saved;
            laptime_saved.reset();

            ESP_ERROR_CHECK(system_get_time(laptime_saved_local.timeofday, laptime_saved_local.date));
            // ESP_ERROR_CHECK(rtc_set_time(laptime_saved_local.timeofday, laptime_saved_local.date));
            ESP_LOGI(TAG, "TIMEOFDAY: %s, DATE: %s", laptime_saved_local.timeofday, laptime_saved_local.date);
            laptime_save_uart(laptime_saved_local);
            laptime_save_top(laptime_saved_local, laptime_list_top);
            laptime_save_last(laptime_saved_local, laptime_list_last);
            laptime_save_driver(laptime_saved_local, laptime_list_driver);
            xQueueSend(laptime_saved_queue_sd, &laptime_saved_local, 0);
            xSemaphoreGive(laptime_lists_mutex);
        }

        // Send status flags to other tasks
        if (status_update_flag == true)
        {
            bool status_list[3] = {config_main.two_gate_mode, stop_flag, sd_active_flag};
            xQueueSend(laptime_status_queue_lcd, status_list, 0);
            xQueueSend(laptime_status_queue_wifi, status_list, 0);
            status_update_flag = false;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}