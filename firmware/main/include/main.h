#pragma once

#include "freertos/idf_additions.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include <esp_wifi_types_generic.h>

/// @brief Size of local stored best/last laptime list
#define LAPTIME_LIST_SIZE_LOCAL 50

/// @brief Length of string needed to store converted laptime
#define LAPTIME_STR_LENGTH 21

#define PENALTY_TIME_STR_LENGTH 11

#define PENALTY_COUNT_STR_LENGTH 25

/// @brief Number of status flags displayed on LCD and webpage (gate mode, stop flag, sd flag)
#define STATUS_LIST_LENGTH 3

#define DRIVER_MAX_COUNT 10

#define DRIVER_TAG_LENGTH 4

#define TIMEOFDAY_STR_LENGTH 9
#define DATE_STR_LENGTH 11

#define WIFI_SSID_STR_LENGTH 32
#define WIFI_PASSWORD_STR_LENGTH 64

#define DRIVER_LIST_DEFAULT {"---", "AAA", "BBB", "CCC"}

#define LAPTIME_STR_DEFAULT "--, --:--.--"
#define PENALTY_STR_DEFAULT "+00:00"
#define COUNT_STR_DEFAULT "0"

#define WIFI_IP_DEFAULT
#define WIFI_SSID_DEFAULT "PUTM_LAPTIMER"
#define WIFI_PASSWORD_DEFAULT "\0"
#define WIFI_CHANNEL_DEFAULT 1
#define WIFI_MAX_CONN_DEFAULT 3
#define TIMEOFDAY_STR_DEFAULT "21:37:00"
#define DATE_STR_DEFAULT "2026-01-01"

#ifdef __cplusplus

class Laptime
{
public:
    volatile uint16_t count = 1;
    volatile uint64_t time = 0;

    uint32_t penalty_time = 0;
    uint16_t doo_count = 0;
    uint16_t oc_count = 0;

    int8_t driver_id = 0;

    char timeofday[TIMEOFDAY_STR_LENGTH] = TIMEOFDAY_STR_DEFAULT;
    char date[DATE_STR_LENGTH] = DATE_STR_DEFAULT;

    void reset();
    void new_lap();

    /**
     * @brief Converts laptime from int measured in 10ms to string that is ready to display
     * @param laptime_str String for converted laptime
     * @param size Size of string
     */
    void convert_string_full(char laptime_str[LAPTIME_STR_LENGTH], size_t size);

    void convert_string_time(char laptime_str[LAPTIME_STR_LENGTH], size_t size);

    void convert_string_penalty(char laptime_str[PENALTY_TIME_STR_LENGTH], size_t size);
};

struct Driver_list
{
    char list[DRIVER_MAX_COUNT][DRIVER_TAG_LENGTH] = DRIVER_LIST_DEFAULT;
    uint8_t driver_count = 3;
};

struct Config
{
    bool two_gate_mode = false;
    Driver_list driver_list;
    wifi_mode_t wifi_mode = WIFI_MODE_STA;
    char wifi_ssid[WIFI_SSID_STR_LENGTH] = WIFI_SSID_DEFAULT;
    char wifi_password[WIFI_PASSWORD_STR_LENGTH] = WIFI_PASSWORD_DEFAULT;
    char time_set[TIMEOFDAY_STR_LENGTH] = TIMEOFDAY_STR_DEFAULT;
    char date_set[DATE_STR_LENGTH] = DATE_STR_DEFAULT;
    // uint8_t wifi_channel = WIFI_CHANNEL_DEFAULT;
    // uint8_t wifi_max_connection = WIFI_MAX_CONN_DEFAULT;
};

enum Wifi_reset
{
    WIFI_RESET_CONFIG,
    WIFI_RESET_DEFAULTS,
};

#endif

/**
 * @defgroup freertos
 * @brief FreeRTOS intertask communication
 */

extern SemaphoreHandle_t config_mutex;

extern SemaphoreHandle_t wifi_reset_queue;

/**
 * @ingroup freertos
 * @brief Queue passes laptimes from main logic laptimer_task to sdcard_task
 */
extern QueueHandle_t laptime_saved_queue_sd;

/**
 * @ingroup freertos
 * @brief Queue passes current laptime from main logic laptimer_task to lcd_task and wifi_task
 */
extern QueueHandle_t laptime_current_queue_lcd;
extern QueueHandle_t laptime_current_queue_wifi;

/**
 * @ingroup freertos
 * @brief Semaphore allows lcd_task and wifi_task to read stored laptime lists
 */
extern SemaphoreHandle_t laptime_lists_mutex;

/**
 * @ingroup freertos
 * @brief Queue passes active status flags from main logic laptimer_task to lcd_task and wifi_task
 * bool[0] - two_gates_mode
 * bool[1] - stop_flag
 * bool[2] - sdcard_flag
 */
extern QueueHandle_t laptime_status_queue_lcd;
extern QueueHandle_t laptime_status_queue_wifi;

extern QueueHandle_t ip_queue;
extern QueueHandle_t wifi_mode_queue;

#ifdef __cplusplus

extern Config config_main;

extern Laptime laptime_list_top[LAPTIME_LIST_SIZE_LOCAL];
extern Laptime laptime_list_last[LAPTIME_LIST_SIZE_LOCAL];
extern Laptime laptime_list_driver[DRIVER_MAX_COUNT];
#endif

extern bool sd_active_flag;
