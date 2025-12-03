#include "laptimer_task.h"

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

/**
 * @brief Global variable determines behavior of gate inputs
 */
volatile Lapmode lap_mode = ONE_GATE_MODE;

/**
 * @brief Global variable indicates stopped laptime, set true by LAP_RESET_PIN and set false by LAP_GATE1_PIN
 */
volatile bool stop_flag = true;

volatile uint64_t doo_press_time = 0;
volatile uint64_t oc_press_time = 0;
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
esp_err_t laptime_save_local(Laptime *laptime, Laptime_list *list)
{
    if (list == NULL || list->list_last == NULL || list->list_last == NULL || laptime == NULL)
        return ESP_FAIL;
    Laptime *list_last = list->list_last;
    Laptime *list_top = list->list_top;
    for (int i = LAPTIME_LIST_SIZE_LOCAL - 1; i > 0; i--)
    {
        list_last[i] = list_last[i - 1];
    }
    list_last[0] = *laptime;

    for (int i = 0; i < LAPTIME_LIST_SIZE_LOCAL; i++)
    {
        if (laptime->time < list_top[i].time || list_top[i].time == 0)
        {
            for (int j = LAPTIME_LIST_SIZE_LOCAL - 1; j > i; j--)
            {
                list_top[j] = list_top[j - 1];
            }
            list_top[i] = *laptime;
            break;
        }
    }
    return ESP_OK;
}

/**
 * @brief Converts laptime from int measured in 10ms to string that is ready to display
 * @param laptime Laptime to convert
 * @param laptime_str String for converted laptime
 * @param size Size of string
 */
// void laptime_convert_string(Laptime laptime, char laptime_str[LAPTIME_STRING_LENGTH], size_t size)
// {
//     if (laptime_str == NULL)
//         return;

//     if (laptime.time == 0)
//     {
//         snprintf(laptime_str, size, "--, --:--:--");
//         return;
//     }

//     unsigned int mm = (laptime.time / 6000) % 60;
//     unsigned int ss = (laptime.time / 100) % 60;
//     unsigned int ms = laptime.time % 100;
//     if (size == LAPTIME_STRING_LENGTH)
//         snprintf(laptime_str, size, "%02u, %02u:%02u:%02u",
//                  laptime.count, mm, ss, ms);
// }

/**
 * @brief To be changed (it should send laptime over UART, not use printf)
 * @param laptime_str
 * @param size
 */
static void laptime_save_uart(char *laptime_str, size_t size)
{
    if (laptime_str == NULL)
        return;
    printf("%s", laptime_str);
    printf("\n");
}

/**
 * @brief Checks active flags and sends them to lcd_task and wifi_task
 */
void send_status()
{
    static bool status[3] = {false};
    switch (lap_mode)
    {
    case ONE_GATE_MODE:
        status[0] = false;
        break;
    case TWO_GATE_MODE:
        status[0] = true;
        break;
    default:
        break;
    }

    if (stop_flag)
        status[1] = true;
    else
        status[1] = false;

    if (sd_active_flag)
        status[2] = true;
    else
        status[2] = false;
    xQueueSend(lcd_laptime_status_queue, status, 0);
    xQueueSend(wifi_laptime_status_queue, status, 0);
}

/**
 * @brief Converts local laptime lists to strings, saves them to global arrays and allows lcd_task and wifi_task to read them
 * @param list Structure that stores local laptimes
 */
esp_err_t send_laptime_lists(Laptime_list *list)
{
    if (list == NULL || list->list_last == NULL || list->list_last == NULL)
        return ESP_FAIL;

    for (int i = 0; i < LAPTIME_LIST_SIZE_WIFI; i++)
    {
        if (i < LAPTIME_LIST_SIZE_LCD)
        {
            list->list_top[i].convert_string(lcd_list_buffer[0][i], LAPTIME_STRING_LENGTH);
            list->list_last[i].convert_string(lcd_list_buffer[1][i], LAPTIME_STRING_LENGTH);
        }

        list->list_top[i].convert_string(wifi_list_buffer[0][i], LAPTIME_STRING_LENGTH);
        list->list_last[i].convert_string(wifi_list_buffer[1][i], LAPTIME_STRING_LENGTH);
    }

    xSemaphoreGive(lcd_laptime_lists_semaphore);
    xSemaphoreGive(wifi_laptime_lists_semaphore);
    return ESP_OK;
}

void penalty_check()
{
    if (doo_long_flag == true)
    {
        if (gpio_get_level(LAP_DOO_PIN) == 0)
        {
            if ((pdTICKS_TO_MS(xTaskGetTickCount()) - doo_press_time) > 1000)
            {
                if (laptime_current.penalty_time >= DOO_TIME_PENALTY && laptime_current.doo_count > 0)
                {
                    laptime_current.doo_count--;
                    laptime_current.penalty_time -= DOO_TIME_PENALTY;
                }
                doo_long_flag = false;
            }
        }
        else
        {
            laptime_current.doo_count++;
            laptime_current.penalty_time += DOO_TIME_PENALTY;
            doo_long_flag = false;
        }
    }

    if (oc_long_flag == true)
    {
        if (gpio_get_level(LAP_OC_PIN) == 0)
        {
            if ((pdTICKS_TO_MS(xTaskGetTickCount()) - oc_press_time) > 1000)
            {
                if (laptime_current.penalty_time >= OC_TIME_PENALTY && laptime_current.oc_count > 0)
                {
                    laptime_current.oc_count--;
                    laptime_current.penalty_time -= OC_TIME_PENALTY;
                }
                oc_long_flag = false;
            }
        }
        else
        {
            laptime_current.oc_count++;
            laptime_current.penalty_time += OC_TIME_PENALTY;
            oc_long_flag = false;
        }
    }
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
    stop_flag = true;
    laptime_current.reset();
}

void doo_pin_isr()
{
    uint64_t tick = pdTICKS_TO_MS(xTaskGetTickCountFromISR());
    if ((tick - doo_press_time) > 80 && doo_long_flag == false)
    {
        doo_long_flag = true;
        doo_press_time = tick;
    }
}

void oc_pin_isr()
{
    uint64_t tick = pdTICKS_TO_MS(xTaskGetTickCountFromISR());
    if ((tick - oc_press_time) > 80 && oc_long_flag == false)
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

    char laptimer_current_str[LAPTIME_STRING_LENGTH] = {"--, --:--:--"};
    char laptime_saved_str[LAPTIME_STRING_LENGTH] = {"--, --:--:--"};

    Laptime laptime_list_top[LAPTIME_LIST_SIZE_LOCAL] = {0};
    Laptime laptime_list_last[LAPTIME_LIST_SIZE_LOCAL] = {0};
    Laptime_list laptime_list = {laptime_list_top, laptime_list_last};

    bool stop_flag_old = stop_flag;
    bool sdcard_flag_old = sd_active_flag;

    xQueueSend(lcd_laptime_current_queue, laptimer_current_str, 0);
    xQueueSend(wifi_laptime_current_queue, laptimer_current_str, 0);
    send_laptime_lists(&laptime_list);

    for (;;)
    {
        if (stop_flag == false)
        {
            laptime_current.time = timer_get_time(laptime_timer);
            penalty_check();
            laptime_current.convert_string(laptimer_current_str, LAPTIME_STRING_LENGTH);
            xQueueSend(lcd_laptime_current_queue, laptimer_current_str, 0);
            xQueueSend(wifi_laptime_current_queue, laptimer_current_str, 0);
        }

        if (stop_flag != stop_flag_old)
        {
            stop_flag_old = stop_flag;
            lap_mode = lap_mode_check();
            if (sd_active_flag == false && stop_flag == true)
            {
                xSemaphoreGive(sd_reinit_semaphore);
            }

            send_status();
        }

        if (sdcard_flag_old != sd_active_flag)
        {
            sdcard_flag_old = sd_active_flag;
            send_status();
        }

        if (laptime_saved.time > 0)
        {
            ESP_LOGI(TAG, "DOO: %u, OC: %u, Penalty sum: %llu\n", laptime_saved.doo_count, laptime_saved.oc_count, laptime_saved.penalty_time);

            laptime_saved.convert_string(laptime_saved_str,
                                         sizeof(laptime_saved_str));

            laptime_save_uart(laptime_saved_str,
                              sizeof(laptime_saved_str));
            laptime_save_local(&laptime_saved, &laptime_list);
            laptime_saved.reset();
            xQueueSend(sd_queue, laptime_saved_str, 0);
            send_laptime_lists(&laptime_list);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
