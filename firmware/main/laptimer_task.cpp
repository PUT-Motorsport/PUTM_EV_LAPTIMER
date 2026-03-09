#include "laptimer_task.hpp"

#include "gpio.h"
#include "main.hpp"
#include "sdcard.h"
#include "timer.h"

/// @brief Minimal time to save in ms
#define LAPTIME_MIN 500

static const char *TAG = "LAPTIMER_TASK";

/// @brief Time penalty added to current laptime by LAP_DOO_PIN and LAP_OC_PIN in ms
const uint32_t DOO_TIME_PENALTY = 200;
const uint32_t OC_TIME_PENALTY = 1000;

/**
 * @brief Used to store current laptime that is readed from timer in main loop and saved in ISR
 */
Laptime laptime_current;

/**
 * @brief Used to store saved laptime after current laptime is saved in ISR,
laptime is then saved on lists and sent to other tasks
 */
Laptime laptime_saved;

/**
 * @brief State of all buttons
 */
Button_press doo_press;
Button_press oc_press;
Button_press driver_select_press;
Button_press wifi_press;

/**
 * @brief Saves received laptime on local best/last/driver lists sorted
 * @param laptime Laptime to save
 * @param list that stores local laptimes
 * @return
 */
static esp_err_t laptime_save_top(Laptime laptime, std::array<Laptime, LAPTIME_LIST_SIZE_LOCAL> &list_top)
{
    for (auto i = 0; i < list_top.size(); i++)
    {
        if (laptime.time < list_top.at(i).time || list_top.at(i).time == 0)
        {
            for (auto j = list_top.size() - 1; j > i; j--)
            {
                list_top.at(j) = list_top.at(j - 1);
            }
            list_top.at(i) = laptime;
            break;
        }
    }
    return ESP_OK;
}

static esp_err_t laptime_save_last(Laptime laptime, std::array<Laptime, LAPTIME_LIST_SIZE_LOCAL> &list_last)
{
    for (auto i = list_last.size() - 1; i > 0; i--)
    {
        list_last.at(i) = list_last.at(i - 1);
    }
    list_last.at(0) = laptime;
    return ESP_OK;
}

static esp_err_t laptime_save_driver(Laptime laptime, std::array<Laptime, DRIVER_MAX_COUNT> &list_driver)
{
    if (laptime.time < list_driver.at(laptime.driver_id).time || list_driver.at(laptime.driver_id).time == 0)
        list_driver.at(laptime.driver_id).time = laptime.time;
    list_driver.at(laptime.driver_id).driver_id = laptime.driver_id;
    list_driver.at(laptime.driver_id).oc_count += laptime.oc_count;
    list_driver.at(laptime.driver_id).doo_count += laptime.doo_count;
    list_driver.at(laptime.driver_id).penalty_time += laptime.penalty_time;
    list_driver.at(laptime.driver_id).count++;

    return ESP_OK;
}

/**
 * @brief Sends received laptime to UART (temporary sends it to esp-idf log)
 * @param laptime Laptime to save
 * @param list that stores local laptimes
 * @return
 */
static void laptime_save_uart(Laptime laptime)
{
    char laptime_saved_str[LAPTIME_STR_LENGTH] = LAPTIME_STR_DEFAULT;
    laptime.convert_string_full(laptime_saved_str,
                                sizeof(laptime_saved_str));
    printf("%s\n", laptime_saved_str);
}

/**
 * @brief Changes button state depending on button level and press time
 * @param btn_level Current logic level of button
 * @param button_press Struct containing button press state and press time
 * @return Changed button state
 */
Button_state button_hold(bool btn_level, Button_press button_press)
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

/**
 * @brief Adding or removing penalty to laptime based on button state
 * @param laptime Laptime that receives penalty
 */
static void penalty_check(Laptime *laptime)
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

/**
 * @brief Changing driver assigned to laptime
 * @param laptime Laptime that will have driver changed
 * @param driver_list structure with list of drivers and driver count
 */
static bool driver_select(Laptime *laptime, Driver_list *driver_list)
{
    switch (driver_select_press.state = button_hold(gpio_get_level(DRIVER_SELECT_PIN), driver_select_press))
    {
    case BTN_HOLD_ACTION:
        laptime->driver_id--;
        if (laptime->driver_id < 1)
            laptime->driver_id = driver_list->driver_count;
        return true;
    case BTN_RELEASED_ACTION:
        laptime->driver_id++;
        if (laptime->driver_id > driver_list->driver_count)
            laptime->driver_id = 1;
        return true;
    default:
        return false;
    }
    return false;
}

/**
 * @brief Returns wifi reset state based on button press state
 * @return Wifi reset state
 */
static Wifi_reset wifi_reset_check()
{
    switch (wifi_press.state = button_hold(gpio_get_level(WIFI_PIN), wifi_press))
    {
    case BTN_HOLD_ACTION:
        return WIFI_RESET_DEFAULTS;
    case BTN_RELEASED_ACTION:
        return WIFI_RESET_CONFIG;
    default:
        return WIFI_NO_RESET;
    }
}

/**
 * @brief Generic ISR for button that can be short pressed or long pressed
 * @param button_press Structure with button state and press time
 */
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
            timer_start(laptime_timer);
        }
        return;
    }
    else
    {
        if (stop_flag == true)
        {
            stop_flag = false;
            timer_start(laptime_timer);
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
            timer_stop(laptime_timer);
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
        timer_stop(laptime_timer);
        laptime_current.reset();
    }
}

void IRAM_ATTR wifi_pin_isr()
{
    button_isr(&wifi_press);
}

#ifdef CONFIG_TWO_GATE_WIRELESS_MASTER

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

#endif

esp_err_t isr_init()
{
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_GATE1_PIN,
                                         (gpio_isr_t)gate1_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_GATE2_PIN,
                                         (gpio_isr_t)gate2_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_RESET_PIN,
                                         (gpio_isr_t)reset_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(WIFI_PIN,
                                         (gpio_isr_t)wifi_pin_isr, NULL));

#ifdef CONFIG_TWO_GATE_WIRELESS_MASTER

    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_DOO_PIN,
                                         (gpio_isr_t)doo_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(LAP_OC_PIN,
                                         (gpio_isr_t)oc_pin_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(DRIVER_SELECT_PIN,
                                         (gpio_isr_t)driver_select_pin_isr, NULL));

#endif

    ESP_LOGI("ISR", "INIT OK");
    return ESP_OK;
}

/**
 * @brief Laptimer main logic task in main loop:
 * 1. Updates local drivers list if needed
 * 2. Checks buttons ISRs
 * 3. Updates current laptime from timer if stop_flag is false
 * 4. Sends current laptime to other tasks
 * 5. When laptime is saved, stores it locally and sends through UART, sd card, shows it on LCD and WIFI page
 */
void laptimer_task(void *args)
{
    Driver_list driver_list_local;

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

        driver_select(&laptime_current, &driver_list_local);
        penalty_check(&laptime_current);

        if (wifi_reset_flag == WIFI_NO_RESET)
            wifi_reset_flag = wifi_reset_check();

        // Read laptime
        if (stop_flag == false)
        {
            laptime_current.time = timer_get_time(laptime_timer);
        }

        // Send laptime to lcd and wifi webpage
        xQueueSend(laptime_current_queue_lcd, &laptime_current, 0);
        xQueueSend(laptime_current_queue_wifi, &laptime_current, 0);

        // Save laptime to lists and sd card, send with uart, reset laptime
        if (laptime_saved.time > 0)
        {
            if (xSemaphoreTake(laptime_lists_mutex, 0) == pdTRUE)
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
                lists_refresh_lcd_flag = true;
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}