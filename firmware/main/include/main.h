#pragma once

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/idf_additions.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include <esp_wifi_types_generic.h>

/**
 * @defgroup defines
 * @brief Defines
 * @{
 */

/// @brief Size of local stored best/last laptime list
#define LAPTIME_LIST_SIZE_LOCAL 50

#define LAPTIME_STR_LENGTH 21

#define COUNT_STR_LENGTH 3

#define PENALTY_TIME_STR_LENGTH 11

#define PENALTY_COUNT_STR_LENGTH 25

#define DRIVER_MAX_COUNT 10

#define DRIVER_TAG_LENGTH 4

#define TIMEOFDAY_STR_LENGTH 9
#define DATE_STR_LENGTH 11

#define WIFI_SSID_STR_LENGTH 32
#define WIFI_PASSWORD_STR_LENGTH 64
#define WIFI_IP_LENGTH 52

#define DRIVER_LIST_DEFAULT        \
    {                              \
        "---", "AAA", "BBB", "CCC" \
    }

#define LAPTIME_STR_DEFAULT "--, --:--.--"
#define PENALTY_STR_DEFAULT "+00:00"
#define COUNT_STR_DEFAULT "0"

#define WIFI_MODE_DEFAULT WIFI_MODE_AP
#define WIFI_IP_DEFAULT "192.168.4.1"
#define WIFI_SSID_DEFAULT "PUTM_LAPTIMER"
#define WIFI_PASSWORD_DEFAULT "\0"
#define WIFI_CHANNEL_DEFAULT 1
#define WIFI_MAX_CONN_DEFAULT 3
#define TIMEOFDAY_STR_DEFAULT "21:37:00"
#define DATE_STR_DEFAULT "2026-01-01"

#ifdef __cplusplus

/**
 * @}
 * @defgroup types
 * @brief Types
 * @{
 */

/**
 * @brief Laptime class contains all properties of every laptime measurement.
 */

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
     * @brief Converts laptime count from int to string
     * @param laptime_str String for converted laptime
     * @param size Size of string
     */
    void convert_string_count(char count_str[COUNT_STR_LENGTH], size_t size);

    /**
     * @brief Converts laptime from int measured in 10ms to string that contains count and time
     * @param laptime_str String for converted laptime
     * @param size Size of string
     */
    void convert_string_full(char laptime_str[LAPTIME_STR_LENGTH], size_t size);

    /**
     * @brief Converts laptime from int measured in 10ms to string that contains only time
     * @param laptime_str String for converted laptime
     * @param size Size of string
     */
    void convert_string_time(char laptime_str[LAPTIME_STR_LENGTH], size_t size);

    /**
     * @brief Converts penalty time from int measured in 10ms to string that contains penalty time
     * @param laptime_str String for converted laptime
     * @param size Size of string
     */
    void convert_string_penalty(char laptime_str[PENALTY_TIME_STR_LENGTH], size_t size);
};

/**
 * @brief Contains list of three letter driver tags and number of drivers
 */
struct Driver_list
{
    char list[DRIVER_MAX_COUNT][DRIVER_TAG_LENGTH] = DRIVER_LIST_DEFAULT;
    uint8_t driver_count = 3;
};

/**
 * @brief Contains laptimer configuration settings for wifi, driver list, time etc.
 * Configuration is retrieved from sdcard or wifi webpage.
 */
struct Config
{
    bool two_gate_mode = false;
    Driver_list driver_list;
    wifi_mode_t wifi_mode = WIFI_MODE_DEFAULT;
    char wifi_ssid[WIFI_SSID_STR_LENGTH] = WIFI_SSID_DEFAULT;
    char wifi_password[WIFI_PASSWORD_STR_LENGTH] = WIFI_PASSWORD_DEFAULT;
    char time_set[TIMEOFDAY_STR_LENGTH] = TIMEOFDAY_STR_DEFAULT;
    char date_set[DATE_STR_LENGTH] = DATE_STR_DEFAULT;
    // uint8_t wifi_channel = WIFI_CHANNEL_DEFAULT;
    // uint8_t wifi_max_connection = WIFI_MAX_CONN_DEFAULT;
};

/**
 * @brief Possible states for resetting wifi webpage
 */
enum Wifi_reset
{
    WIFI_NO_RESET,
    WIFI_RESET_CONFIG,
    WIFI_RESET_DEFAULTS,
};

#endif

/**
 * @}
 * @defgroup freertos
 * @brief FreeRTOS intertask communication
 * @{
 */

/**
 * @brief Access to config_main structure
 */
extern SemaphoreHandle_t config_mutex;

/**
 * @brief List of laptimes to save on sd card
 */
extern QueueHandle_t laptime_saved_queue_sd;

/**
 * @brief Queue passes current laptime from main logic laptimer_task to lcd_task and wifi_task
 */
extern QueueHandle_t laptime_current_queue_lcd;
extern QueueHandle_t laptime_current_queue_wifi;

/**
 * @brief Mutex used by tasks to read: laptime_list_top, laptime_list_last, laptime_list_driver
 */
extern SemaphoreHandle_t laptime_lists_mutex;

/**
 * @brief Queue used to send ip from wifi task to lcd task which displays ip address on screen
 */
extern QueueHandle_t ip_queue;

#ifdef __cplusplus

/**
 * @}
 * @defgroup globals
 * @brief Global variables used by all tasks
 */

/**
 * @brief Main configuration, tasks refresh local configuration using config_mutex
 */
extern Config config_main;

/**
 * @brief Laptimes sorted from fastest to slowest
 */
extern Laptime laptime_list_top[LAPTIME_LIST_SIZE_LOCAL];

/**
 * @brief Laptimes sorted from newest to oldest
 */
extern Laptime laptime_list_last[LAPTIME_LIST_SIZE_LOCAL];

/**
 * @brief List with fastest laptimes of each driver
 */
extern Laptime laptime_list_driver[DRIVER_MAX_COUNT];

/**
 * @brief Indicates if sdcard is active (inserted and initialized).
 * Set by sdcard_task
 */
extern volatile bool sd_active_flag;

/**
 * @brief Indicates stopped laptime, in this state current laptime is 0 and does not refesh.
 * Set by laptimer_task with button ISR.
 */
extern volatile bool stop_flag;

/**
 * @brief Indicates need of wifi reset.
 * Set by laptimer_task with button ISR and by wifi_task after reset.
 */
extern volatile Wifi_reset wifi_reset_flag;

/**
 * @brief Indicates current mode of wifi (Access point, station or off).
 * Set by wifi_task after wifi initialization.
 */
extern volatile wifi_mode_t wifi_mode_flag;

/**
 * @brief Indicates refresh of laptime lists on lcd display.
 * Set by laptimer_task after saving laptime and unset by lcd_task after refreshing lists.
 * @}
 */
extern volatile bool lists_refresh_lcd_flag;

#endif
