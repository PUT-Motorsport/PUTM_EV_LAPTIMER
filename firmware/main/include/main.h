#pragma once

#include "freertos/idf_additions.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include <esp_wifi_types_generic.h>

/**
 * @defgroup pinout_defines
 * @brief Pinout defines
 */

/**
 * @ingroup pinout_defines
 * @brief Input for first gate that starts new lap on negative edge
 */
#define LAP_GATE1_PIN ((gpio_num_t)CONFIG_GATE1_PIN)

/**
 * @ingroup pinout_defines
 * @brief Input for second gate that stops lap on negative edge in 2 gate mode
 */
#define LAP_GATE2_PIN ((gpio_num_t)CONFIG_GATE2_PIN)

/**
 * @ingroup pinout_defines
 * @brief Input for button that resets current laptime, tries to reinitialize sd card and checks status of LAP_MODE_PIN on negative edge
 */
#define LAP_RESET_PIN ((gpio_num_t)CONFIG_STOP_PIN)

/**
 * @ingroup pinout_defines
 * @brief Input for switching between 1 gate and 2 gate mode
 * LOW - 1 gate mode
 * HIGH - 2 gate mode
 */
#define LAP_MODE_PIN ((gpio_num_t)CONFIG_MODE_PIN)

/**
 * @ingroup pinout_defines
 * @brief Input for button that adds Down or Out time penalty to current laptime (2s for Endurance) on negative edge
 */
#define LAP_DOO_PIN ((gpio_num_t)CONFIG_DOO_PIN)

/**
 * @ingroup pinout_defines
 * @brief Input for button that adds Off-Course time penalty to current laptime (10s for Endurance) on negative edge
 */
#define LAP_OC_PIN ((gpio_num_t)CONFIG_OC_PIN)

#define DRIVER_SELECT_PIN ((gpio_num_t)CONFIG_DRIVER_SELECT_PIN)

/// @ingroup pinout_defines
/// @brief SDCARD pinout
#define SD_SPI_CLK CONFIG_SD_SPI_CLK
#define SD_SPI_MISO CONFIG_SD_SPI_MISO
#define SD_SPI_MOSI CONFIG_SD_SPI_MOSI
#define SD_SPI_CS CONFIG_SD_SPI_CS

#define SD_CD CONFIG_SD_CD

/// @ingroup pinout_defines
/// @brief LCD pinout
#define LCD_SPI_CLK CONFIG_MIPI_DISPLAY_PIN_CLK
#define LCD_SPI_MOSI CONFIG_MIPI_DISPLAY_PIN_MOSI
#define LCD_SPI_CS CONFIG_MIPI_DISPLAY_PIN_CS
#define LCD_DC CONFIG_MIPI_DISPLAY_PIN_DC
#define LCD_RESET CONFIG_MIPI_DISPLAY_PIN_RST
#define LCD_BL CONFIG_MIPI_DISPLAY_PIN_BL

/// @brief SPI_HOST defines for peripherals
#define SPI_SD_HOST SPI2_HOST
#define SPI_LCD_HOST CONFIG_MIPI_DISPLAY_SPI_HOST

/// @brief Size of local stored best/last laptime list
#define LAPTIME_LIST_SIZE_LOCAL 50

/// @brief Size of lcd displayed best/last laptime list
#define LAPTIME_LIST_SIZE_LCD 5

/// @brief Size of wifi displayed best/last laptime list
#define LAPTIME_LIST_SIZE_WIFI 50

/// @brief Minimal time to save in ms
#define LAPTIME_MIN 500

/// @brief Length of string needed to store converted laptime
#define LAPTIME_STR_LENGTH 21

#define PENALTY_TIME_STR_LENGTH 11

#define PENALTY_COUNT_STR_LENGTH 25

/// @brief Number of status flags displayed on LCD and webpage (gate mode, stop flag, sd flag)
#define STATUS_LIST_LENGTH 3

/// @brief Time penalty added to current laptime by LAP_DOO_PIN and LAP_OC_PIN in ms
#define DOO_TIME_PENALTY (uint32_t)200
#define OC_TIME_PENALTY (uint32_t)1000

#define DRIVER_MAX_COUNT 10

#define DRIVER_TAG_LENGTH 4

#define DRIVER_LIST_DEFAULT {"---", "AAA", "BBB", "CCC"}

enum Lap_mode
{
    ONE_GATE_MODE,
    TWO_GATE_MODE,
};

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

    void reset()
    {
        this->time = 0;
        this->penalty_time = 0;
        this->doo_count = 0;
        this->oc_count = 0;
    }

    void new_lap()
    {
        this->count++;
        this->reset();
    }

    /**
     * @brief Converts laptime from int measured in 10ms to string that is ready to display
     * @param laptime_str String for converted laptime
     * @param size Size of string
     */
    void convert_string(char laptime_str[LAPTIME_STR_LENGTH], size_t size)
    {
        if (laptime_str == NULL)
            return;

        if (this->time == 0)
        {
            snprintf(laptime_str, size, "--, --:--.--");
            return;
        }

        unsigned int mm = (this->time / 6000) % 60;
        unsigned int ss = (this->time / 100) % 60;
        unsigned int ms = this->time % 100;
        if (size == LAPTIME_STR_LENGTH)
            snprintf(laptime_str, size, "%02u, %02u:%02u:%02u",
                     this->count, mm, ss, ms);
    }

    void convert_string_time(char laptime_str[LAPTIME_STR_LENGTH], size_t size)
    {
        if (laptime_str == NULL)
            return;

        if (this->time == 0)
        {
            snprintf(laptime_str, size, "--:--.--");
            return;
        }

        unsigned int mm = (this->time / 6000) % 60;
        unsigned int ss = (this->time / 100) % 60;
        unsigned int ms = this->time % 100;
        if (size == LAPTIME_STR_LENGTH)
            snprintf(laptime_str, size, "%02u:%02u:%02u", mm, ss, ms);
    }

    void penalty_string(char laptime_str[PENALTY_TIME_STR_LENGTH], size_t size)
    {
        if (laptime_str == NULL)
            return;

        if (this->time == 0)
        {
            snprintf(laptime_str, size, "+00:00");
            return;
        }

        unsigned int mm = (this->penalty_time / 6000) % 60;
        unsigned int ss = (this->penalty_time / 100) % 60;
        if (size >= 11)
        {
            snprintf(laptime_str, size, "+%02u:%02u", mm, ss);
        }
    }
};

struct Driver_list
{
    char list[DRIVER_MAX_COUNT][DRIVER_TAG_LENGTH] = DRIVER_LIST_DEFAULT;
    uint8_t driver_count = 3;
};

struct Config
{
    Lap_mode lap_mode = ONE_GATE_MODE;
    Driver_list driver_list;

    wifi_mode_t wifi_mode = WIFI_MODE_STA;
    char wifi_ssid[32] = "iPhone (Hubert)";
    char wifi_password[64] = "Mateusz123";
    uint8_t wifi_channel = 1;
    uint8_t wifi_max_connection = 3;
};

#endif

/**
 * @defgroup freertos
 * @brief FreeRTOS intertask communication
 */

extern SemaphoreHandle_t config_mutex;

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
 * bool[0] - mode
 * bool[1] - stop_flag
 * bool[2] - sdcard_flag
 */
extern QueueHandle_t laptime_status_queue_lcd;
extern QueueHandle_t laptime_status_queue_wifi;

#ifdef __cplusplus

// extern Driver_list driver_list_main;

extern Config config_main;

// const char laptime_current_default_str[] = "--, --:--.--";
// const char laptime_penalty_default_str[] = "+00:00";
// const char laptime_oc_default_str[] = "0";
// const char laptime_doo_default_str[] = "0";

extern Laptime laptime_list_top[LAPTIME_LIST_SIZE_LOCAL];
extern Laptime laptime_list_last[LAPTIME_LIST_SIZE_LOCAL];
extern Laptime laptime_list_driver[DRIVER_MAX_COUNT];
#endif

/**
 * @brief Global variable indicates stopped laptime, set true by LAP_RESET_PIN and set false by LAP_GATE1_PIN
 */
extern volatile bool stop_flag;

extern bool sd_active_flag;
