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
volatile bool doo_long_flag = false;
volatile bool oc_long_flag = false;

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
esp_err_t laptime_save_local(Laptime laptime, Laptime_list *list)
{
    if (list == NULL)
        return ESP_FAIL;
    for (int i = LAPTIME_LIST_SIZE_LOCAL - 1; i > 0; i--)
    {
        list->list_last[i] = list->list_last[i - 1];
    }
    list->list_last[0] = laptime;

    for (int i = 0; i < LAPTIME_LIST_SIZE_LOCAL; i++)
    {
        if (laptime.time < list->list_top[i].time || list->list_top[i].time == 0)
        {
            for (int j = LAPTIME_LIST_SIZE_LOCAL - 1; j > i; j--)
            {
                list->list_top[j] = list->list_top[j - 1];
            }
            list->list_top[i] = laptime;
            break;
        }
    }
    return ESP_OK;
}

static void laptime_save_uart(char *laptime_str, size_t size)
{
    if (laptime_str == NULL)
        return;
    printf("%s", laptime_str);
    printf("\n");
}

/**
 * @brief Converts local laptime lists to strings, saves them to global arrays and allows lcd_task and wifi_task to read them
 * @param list Structure that stores local laptimes
 */
esp_err_t send_laptime_lists(Laptime_list *list)
{
    if (list == NULL)
        return ESP_FAIL;

    for (int i = 0; i < LAPTIME_LIST_SIZE_WIFI; i++)
    {
        list->list_top[i].convert_string(list_top_str[i], LAPTIME_STR_LENGTH);
        list->list_last[i].convert_string(list_last_str[i], LAPTIME_STR_LENGTH);
        list->list_top[i].penalty_string(list_penalty_time_str[i], PENALTY_TIME_STR_LENGTH);
        snprintf(list_penalty_oc_str[i], PENALTY_COUNT_STR_LENGTH, "%u", list->list_top[i].oc_count);
        snprintf(list_penalty_doo_str[i], PENALTY_COUNT_STR_LENGTH, "%u", list->list_top[i].doo_count);
    }

    xSemaphoreGive(lcd_laptime_lists_semaphore);
    xSemaphoreGive(wifi_laptime_lists_semaphore);
    return ESP_OK;
}

bool penalty_check()
{
    uint32_t penalty_time_temp = laptime_current.penalty_time;
    TickType_t tick = xTaskGetTickCount();
    static bool doo_after_press_flag = false;
    static bool oc_after_press_flag = false;

    if (doo_long_flag == true)
    {
        doo_long_flag = laptime_current.penalty_check(gpio_get_level(LAP_DOO_PIN), &doo_after_press_flag, tick - doo_press_time, DOO_TIME_PENALTY);
    }
    if (oc_long_flag == true)
    {
        oc_long_flag = laptime_current.penalty_check(gpio_get_level(LAP_OC_PIN), &oc_after_press_flag, tick - oc_press_time, OC_TIME_PENALTY);
    }

    if (penalty_time_temp != laptime_current.penalty_time || laptime_current.penalty_time == 0)
        return true;
    return false;
}

void send_penalty()
{

    laptime_current.penalty_string(penalty_time_str, sizeof(penalty_time_str));
    snprintf(penalty_oc_str, sizeof(penalty_oc_str), "%3u", laptime_current.oc_count);
    snprintf(penalty_doo_str, sizeof(penalty_doo_str), "%3u", laptime_current.doo_count);

    xSemaphoreGive(lcd_laptime_penalty_semaphore);
    xSemaphoreGive(wifi_laptime_penalty_semaphore);
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
    if ((tick - doo_press_time) > pdMS_TO_TICKS(100) && doo_long_flag == false)
    {
        doo_long_flag = true;
        doo_press_time = tick;
    }
}

void oc_pin_isr()
{
    TickType_t tick = xTaskGetTickCountFromISR();
    if ((tick - oc_press_time) > pdMS_TO_TICKS(100) && oc_long_flag == false)
    {
        oc_long_flag = true;
        oc_press_time = tick;
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

    char laptimer_current_str[LAPTIME_STR_LENGTH] = {"--, --:--.--"};
    char laptime_saved_str[LAPTIME_STR_LENGTH] = {"--, --:--.--"};

    Laptime_list laptime_list;

    bool stop_flag_old = stop_flag;
    bool sd_active_flag_old = sd_active_flag;
    bool status_update_flag = true;

    xQueueSend(lcd_laptime_current_queue, laptimer_current_str, 0);
    xQueueSend(wifi_laptime_current_queue, laptimer_current_str, 0);
    penalty_check();
    send_laptime_lists(&laptime_list);

    for (;;)
    {
        if (stop_flag == false)
        {
            laptime_current.time = timer_get_time(laptime_timer);
            status_update_flag = penalty_check();
            laptime_current.convert_string(laptimer_current_str, LAPTIME_STR_LENGTH);
            xQueueSend(lcd_laptime_current_queue, laptimer_current_str, 0);
            xQueueSend(wifi_laptime_current_queue, laptimer_current_str, 0);
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

        if (laptime_saved.time > 0)
        {
            ESP_LOGI(TAG, "DOO: %u, OC: %u, Penalty sum: %llu\n", laptime_saved.doo_count, laptime_saved.oc_count, laptime_saved.penalty_time);

            laptime_saved.convert_string(laptime_saved_str,
                                         sizeof(laptime_saved_str));

            laptime_save_uart(laptime_saved_str,
                              sizeof(laptime_saved_str));
            laptime_save_local(laptime_saved, &laptime_list);
            laptime_saved.reset();
            xQueueSend(sd_queue, laptime_saved_str, 0);
            send_laptime_lists(&laptime_list);
        }
        if (status_update_flag == true)
        {
            xSemaphoreGive(lcd_laptime_status_semaphore);
            xSemaphoreGive(wifi_laptime_status_semaphore);
            send_penalty();
            status_update_flag = false;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
