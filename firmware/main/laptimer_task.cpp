#include "laptimer_task.h"

#include "sdcard_task.h"

#include "gpio.h"
#include "main.h"
#include "sdcard.h"
#include "timer.h"

#include <stdio.h>
#include <cwchar>

static const char *TAG = "LAPTIMER_TASK";

/**
 * @brief Global variable used to store current laptime that is readed from timer in main loop and saved in ISR
 */
Laptime laptime_current;

/**
 * @brief Global variable used to store saved laptime after current laptime is saved in ISR,
 * laptime is then saved on lists and sent to other tasks
 */
Laptime laptime_saved;

volatile TickType_t doo_press_time = 0;
volatile TickType_t oc_press_time = 0;
volatile TickType_t ds_press_time = 0;
volatile btn_long_state doo_long_flag = BTN_STANDBY;
volatile btn_long_state oc_long_flag = BTN_STANDBY;
volatile btn_long_state ds_state = BTN_STANDBY;

/**
 * @brief Checks active state of LAP_MODE_PIN
 * @return Active mode
 */
Lapmode lap_mode_check()
{
    if (gpio_get_level(LAP_MODE_PIN) == 0)
        return ONE_GATE_MODE;
    else
        return TWO_GATE_MODE;
}

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

static void laptime_save_uart(char *laptime_str, size_t size)
{
    if (laptime_str == NULL)
        return;
    printf("%s", laptime_str);
    printf("\n");
}

btn_long_state btn_hold(bool btn_state, volatile btn_long_state btn_press_state, TickType_t press_time)
{
    switch (btn_press_state)
    {
    case BTN_STANDBY:
        return BTN_STANDBY;

    case BTN_HOLD_WAIT:
        if (btn_state == 1)
            return BTN_RELEASED_ACTION;
        else if (press_time > pdMS_TO_TICKS(1000))
            return BTN_HOLD_ACTION;
        else
            return BTN_HOLD_WAIT;

    case BTN_HOLD_ACTION:
        return BTN_AFTER_HOLD;
    case BTN_RELEASED_ACTION:
        return BTN_STANDBY;

    case BTN_AFTER_HOLD:
        if (btn_state == 1)
            return BTN_STANDBY;
        return BTN_AFTER_HOLD;

    default:
        return BTN_HOLD_WAIT;
    }
}

void penalty_check(Laptime *laptime)
{
    uint32_t penalty_time_temp = laptime->penalty_time;
    TickType_t tick = xTaskGetTickCount();
    doo_long_flag = btn_hold(gpio_get_level(LAP_DOO_PIN), doo_long_flag, tick - doo_press_time);
    oc_long_flag = btn_hold(gpio_get_level(LAP_OC_PIN), oc_long_flag, tick - oc_press_time);

    switch (doo_long_flag)
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
    switch (oc_long_flag)
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

void driver_select()
{
    TickType_t tick = xTaskGetTickCount();
    ds_state = btn_hold(gpio_get_level(DRIVER_SELECT_PIN), ds_state, tick - ds_press_time);
    switch (ds_state)
    {
    case BTN_HOLD_ACTION:
        laptime_current.driver_id--;
        if (laptime_current.driver_id < 1)
            laptime_current.driver_id = driver_count;
        break;
    case BTN_RELEASED_ACTION:
        laptime_current.driver_id++;
        if (laptime_current.driver_id > driver_count)
            laptime_current.driver_id = 1;
        break;
    default:
        return;
    }
    ESP_LOGI(TAG, "DRIVER ID: %d, DRIVER TAG: %s", laptime_current.driver_id, driver_list[laptime_current.driver_id]);

    return;
}

/**
 * @brief ISR for first gate depends on active mode
 * lap_mode == ONE_GATE_MODE - first gate saves laptime and starts new lap on every negative edge
 * lap_mode == TWO_GATE_MODE - first gate only starts new lap on negative edge if stop_flag is true
 */
void gate1_pin_isr()
{
    switch (lap_mode)
    {
    case ONE_GATE_MODE:
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
        break;
    case TWO_GATE_MODE:
        if (stop_flag == true)
        {
            stop_flag = false;
            timer_reset(laptime_timer);
        }
        break;
    default:
        break;
    }
}

/**
 * @brief ISR for second gate depends on active mode
 * lap_mode == ONE_GATE_MODE - second gate is not active
 * lap_mode == TWO_GATE_MODE - second gates saves laptime on negative edge if stop_flag is false
 */
void gate2_pin_isr()
{
    switch (lap_mode)
    {
    case ONE_GATE_MODE:
        break;
    case TWO_GATE_MODE:
        if (stop_flag == false && laptime_current.time > LAPTIME_MIN)
        {
            laptime_saved = laptime_current;
            laptime_saved.time += laptime_current.penalty_time;
            stop_flag = true;
            laptime_current.new_lap();
        }
        break;
    default:
        break;
    }
}

/**
 * @brief ISR for reset button, resets current laptime and activates stop_flag
 */
void reset_pin_isr()
{
    if (stop_flag == false)
    {
        stop_flag = true;
        laptime_current.reset();
    }
}

void doo_pin_isr()
{
    TickType_t tick = xTaskGetTickCountFromISR();
    if ((tick - doo_press_time) > pdMS_TO_TICKS(100) && doo_long_flag == BTN_STANDBY)
    {
        doo_long_flag = BTN_HOLD_WAIT;
        doo_press_time = tick;
    }
}

void oc_pin_isr()
{
    TickType_t tick = xTaskGetTickCountFromISR();
    if ((tick - oc_press_time) > pdMS_TO_TICKS(100) && oc_long_flag == BTN_STANDBY)
    {
        oc_long_flag = BTN_HOLD_WAIT;
        oc_press_time = tick;
    }
}

void driver_select_pin_isr()
{
    TickType_t tick = xTaskGetTickCountFromISR();
    if ((tick - ds_press_time) > pdMS_TO_TICKS(100) && ds_state == BTN_STANDBY)
    {
        ds_state = BTN_HOLD_WAIT;
        ds_press_time = tick;
    }
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
    ESP_LOGI("ISR", "INIT OK");
    return ESP_OK;
}

/**
 * @brief Laptimer main logic task in main loop:
 * 1. Updates current laptime if stop_flag is false
 * 2. On every stop_flag change checks if laptime mode changed
 * 3. Tries to reinit sdcard when LAP_RESET_PIN is clicked
 * 4. When laptime is saved, stores it locally and sends through UART, sd card, shows it on LCD and WIFI page
 */
void laptimer_task(void *args)
{
    char laptime_current_str[LAPTIME_STR_LENGTH] = {"--, --:--.--"};
    char laptime_saved_str[LAPTIME_STR_LENGTH] = {"--, --:--.--"};

    bool stop_flag_old = stop_flag;
    bool sd_active_flag_old = sd_active_flag;
    bool status_update_flag = true;

    xQueueSend(laptime_current_queue_lcd, &laptime_current, 0);
    xQueueSend(laptime_current_queue_wifi, &laptime_current, 0);

    penalty_check(&laptime_current);

    for (;;)
    {
        driver_select();
        if (stop_flag == false)
        {
            laptime_current.time = timer_get_time(laptime_timer);
            penalty_check(&laptime_current);
            laptime_current.convert_string(laptime_current_str, LAPTIME_STR_LENGTH);
            xQueueSend(laptime_current_queue_lcd, &laptime_current, 0);
            xQueueSend(laptime_current_queue_wifi, &laptime_current, 0);
        }

        if (stop_flag != stop_flag_old)
        {
            stop_flag_old = stop_flag;
            lap_mode = lap_mode_check();
            status_update_flag = true;
        }

        if (sd_active_flag_old != sd_active_flag)
        {
            sd_active_flag_old = sd_active_flag;
            status_update_flag = true;
        }

        if (laptime_saved.time > 0 && xSemaphoreTake(laptime_lists_mutex, 0) == pdTRUE)
        {
            ESP_LOGI(TAG, "DOO: %u, OC: %u, Penalty sum: %llu\n", laptime_saved.doo_count, laptime_saved.oc_count, laptime_saved.penalty_time);

            laptime_saved.convert_string(laptime_saved_str,
                                         sizeof(laptime_saved_str));

            laptime_save_uart(laptime_saved_str,
                              sizeof(laptime_saved_str));
            laptime_save_top(laptime_saved, laptime_list_top);
            laptime_save_last(laptime_saved, laptime_list_last);
            laptime_save_driver(laptime_saved, laptime_list_driver);
            xQueueSend(laptime_saved_queue_sd, &laptime_saved, 0);
            xSemaphoreGive(laptime_lists_mutex);
            laptime_saved.reset();
        }
        if (status_update_flag == true)
        {
            bool status_list[3] = {lap_mode, stop_flag, sd_active_flag};
            xQueueSend(laptime_status_queue_lcd, status_list, 0);
            xQueueSend(laptime_status_queue_wifi, status_list, 0);
            status_update_flag = false;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
