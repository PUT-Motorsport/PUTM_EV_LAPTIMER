#include "laptimer_task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "gpio.h"
#include "main.h"
#include "sdcard.h"
#include "timer.h"
#include <stdio.h>
#include <cwchar>

static const char *TAG = "LAPTIMER_TASK";

Laptime laptime_current = {1, 0};

Laptime laptime_saved = {1, 0};

volatile Lapmode lap_mode = ONE_GATE_MODE;

volatile bool stop_flag = true;

Lapmode lap_mode_check()
{
    if (gpio_get_level(LAP_MODE_BTN) == 0)
        return ONE_GATE_MODE;
    else
        return TWO_GATE_MODE;
}

void laptime_reset(Laptime *laptime)
{
    laptime->time = 0;
    laptime->count++;
}

esp_err_t laptime_save_local(Laptime *laptime, Laptime_list *list)
{
    if (list == NULL || list->list_last == NULL || list->list_last == NULL || laptime == NULL)
        return ESP_FAIL;
    Laptime *list_last = list->list_last;
    Laptime *list_top = list->list_top;
    for (int i = LAPTIME_LIST_SIZE - 1; i > 0; i--)
    {
        list_last[i] = list_last[i - 1];
    }
    list_last[0] = *laptime;

    for (int i = 0; i < LAPTIME_LIST_SIZE; i++)
    {
        if (laptime->time < list_top[i].time || list_top[i].time == 0)
        {
            for (int j = LAPTIME_LIST_SIZE - 1; j > i; j--)
            {
                list_top[j] = list_top[j - 1];
            }
            list_top[i] = *laptime;
            break;
        }
    }
    laptime->count = 0;
    laptime->time = 0;
    return ESP_OK;
}

void laptime_convert_string(Laptime laptime, char *laptime_str, size_t size)
{
    if (laptime_str == NULL)
        return;

    if (laptime.time == 0)
    {
        snprintf(laptime_str, size, "--. --:--:--");
        return;
    }

    unsigned int mm = (laptime.time / 6000) % 60;
    unsigned int ss = (laptime.time / 100) % 60;
    unsigned int ms = laptime.time % 100;
    if (size == 13U)
        snprintf(laptime_str, size, "%02u. %02u:%02u:%02u",
                 laptime.count, mm, ss, ms);
    else if (size == 14U)
        snprintf(laptime_str, size, "%02u,%02u:%02u:%02u\n",
                 laptime.count, mm, ss, ms);
}

void laptime_convert_wstring(Laptime laptime, wchar_t *laptime_wstr, int size)
{
    if (laptime_wstr == NULL)
        return;

    if (laptime.time == 0)
    {
        swprintf(laptime_wstr, size, L"--. --:--:--");
        return;
    }

    unsigned int mm = (laptime.time / 6000) % 60;
    unsigned int ss = (laptime.time / 100) % 60;
    unsigned int ms = laptime.time % 100;
    if (size == LAPTIME_LENGTH)
        swprintf(laptime_wstr, size, L"%02u. %02u:%02u:%02u",
                 laptime.count, mm, ss, ms);
    else if (size == 14)
        swprintf(laptime_wstr, size, L"%02u,%02u:%02u:%02u\n",
                 laptime.count, mm, ss, ms);
}

static void laptime_save_uart(char *laptime_str, size_t size)
{
    if (laptime_str == NULL)
        return;
    //   HAL_UART_Transmit_DMA(&huart3, (uint8_t *)lapTimeString, size);
    printf("%s", laptime_str);
}

static void laptime_save_sdcard(char *laptime_str, QueueHandle_t sd_queue)
{
    xQueueSend(sd_queue, laptime_str, 0);
}

void send_laptime(Laptime laptime)
{
    wchar_t laptime_wstr[LAPTIME_LENGTH];
    laptime_convert_wstring(laptime, laptime_wstr, LAPTIME_LENGTH);
    xQueueSend(lcd_laptime_current_queue, laptime_wstr, 0);
}

// void send_status()
// {
//     switch (lap_mode)
//     {
//     case ONE_GATE_MODE:
//         lcd_print_string(LCD_WIDTH - UI_LETTER_WIDTH * 6 - PADDING, PADDING, L"1 GATE ",
//                          UI_FONT, GRAY);
//         break;
//     case TWO_GATE_MODE:
//         lcd_print_string(LCD_WIDTH - UI_LETTER_WIDTH * 6 - PADDING, PADDING, L"2 GATE ",
//                          UI_FONT, GRAY);
//         break;
//     default:
//         break;
//     }

//     if (stop_flag)
//         lcd_print_string(LCD_WIDTH / 2, PADDING, L"STOP", UI_FONT, RED);
//     else
//         lcd_print_string(LCD_WIDTH / 2, PADDING, L"    ", UI_FONT, BLACK);

//     if (sd_active_flag)
//         lcd_print_string(LCD_WIDTH / 2 + UI_LETTER_WIDTH * 5, PADDING, L"SD",
//                          UI_FONT, GREEN);
//     else
//         lcd_print_string(LCD_WIDTH / 2 + UI_LETTER_WIDTH * 5, PADDING, L"  ",
//                          UI_FONT, BLACK);
// }

esp_err_t send_laptime_lists(Laptime_list *list)
{
    if (list == NULL || list->list_last == NULL || list->list_last == NULL)
        return ESP_FAIL;

    static wchar_t list_wch[2][5][LAPTIME_LENGTH];

    for (int i = 0; i < 5; i++)
    {
        laptime_convert_wstring(list->list_top[i], list_wch[0][i], LAPTIME_LENGTH);
        laptime_convert_wstring(list->list_last[i], list_wch[1][i], LAPTIME_LENGTH);
    }
    xQueueSend(lcd_laptime_lists_queue, list_wch, 0);
    return ESP_OK;
}

void gate1_btn_isr()
{
    switch (lap_mode)
    {
    case ONE_GATE_MODE:
        if (stop_flag == false && laptime_current.time > LAPTIME_MIN)
        {
            laptime_saved = laptime_current;
            timer_reset(laptime_timer);
            laptime_reset(&laptime_current);
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

void gate2_btn_isr()
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
            laptime_reset(&laptime_current);
        }
        break;
    default:
        break;
    }
}

void reset_btn_isr()
{
    laptime_current.time = 0;
    stop_flag = true;
}

esp_err_t isr_init()
{
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_GATE1_BTN,
                                         (gpio_isr_t)gate1_btn_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_GATE2_BTN,
                                         (gpio_isr_t)gate2_btn_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_RESET_BTN,
                                         (gpio_isr_t)reset_btn_isr, NULL));
    ESP_LOGI("ISR", "INIT OK");
    return ESP_OK;
}

void laptimer_task(void *args)
{
    char laptime_saved_str[14];

    Laptime laptime_list_top[LAPTIME_LIST_SIZE] = {0};
    Laptime laptime_list_last[LAPTIME_LIST_SIZE] = {0};
    Laptime_list laptime_list = {laptime_list_top, laptime_list_last};

    bool stop_flag_old = stop_flag;
    bool sdcard_flag_old = sd_active_flag;

    send_laptime(laptime_current);
    send_laptime_lists(&laptime_list);

    for (;;)
    {
        if (stop_flag == false)
        {
            laptime_current.time = timer_get_time(laptime_timer);
            send_laptime(laptime_current);
        }

        if (stop_flag != stop_flag_old)
        {
            stop_flag_old = stop_flag;
            lap_mode = lap_mode_check();
            if (sd_active_flag == false && stop_flag == true)
            {
                xSemaphoreGive(sd_reinit_semaphore);
            }

            // show_status();
            send_laptime(laptime_current);
        }

        if (sdcard_flag_old != sd_active_flag)
        {
            sdcard_flag_old = sd_active_flag;
            // show_status();
        }

        if (laptime_saved.time > 0)
        {
            laptime_convert_string(laptime_saved, laptime_saved_str,
                                   sizeof(laptime_saved_str));

            laptime_save_uart(laptime_saved_str,
                              sizeof(laptime_saved_str));
            laptime_save_local(&laptime_saved, &laptime_list);
            laptime_save_sdcard(laptime_saved_str, sd_queue);
            send_laptime_lists(&laptime_list);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
