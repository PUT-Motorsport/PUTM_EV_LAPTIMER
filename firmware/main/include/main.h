#pragma once

#include "freertos/idf_additions.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"

/**
 * @defgroup pinout_defines
 * @brief Pinout defines
 */

/**
 * @ingroup pinout_defines
 * @brief Input for first gate that starts new lap on negative edge
 */
#define LAP_GATE1_PIN ((gpio_num_t)CONFIG_LAP_GATE1_PIN)

/**
 * @ingroup pinout_defines
 * @brief Input for second gate that stops lap on negative edge in 2 gate mode
 */
#define LAP_GATE2_PIN ((gpio_num_t)CONFIG_LAP_GATE2_PIN)

/**
 * @ingroup pinout_defines
 * @brief Input for button that resets current laptime, tries to reinitialize sd card and checks status of LAP_MODE_PIN on negative edge
 */
#define LAP_RESET_PIN ((gpio_num_t)CONFIG_LAP_RESET_PIN)

/**
 * @ingroup pinout_defines
 * @brief Input for switching between 1 gate and 2 gate mode
 * LOW - 1 gate mode
 * HIGH - 2 gate mode
 */
#define LAP_MODE_PIN ((gpio_num_t)CONFIG_LAP_MODE_PIN)

/**
 * @ingroup pinout_defines
 * @brief Input for button that adds Down or Out time penalty to current laptime (2s for Endurance) on negative edge
 */
#define LAP_DOO_PIN ((gpio_num_t)CONFIG_LAP_DOO_PIN)

/**
 * @ingroup pinout_defines
 * @brief Input for button that adds Off-Course time penalty to current laptime (10s for Endurance) on negative edge
 */
#define LAP_OC_PIN ((gpio_num_t)CONFIG_LAP_OC_PIN)

/// @ingroup pinout_defines
/// @brief SDCARD pinout
#define SD_SPI_CLK CONFIG_SD_SPI_CLK
#define SD_SPI_MISO CONFIG_SD_SPI_MISO
#define SD_SPI_MOSI CONFIG_SD_SPI_MOSI
#define SD_SPI_CS CONFIG_SD_SPI_CS

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
#define LAPTIME_LIST_SIZE_WIFI 15

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

struct Penalty_data
{
    char *time;
    uint16_t oc_count;
    uint16_t doo_count;
};

/**
 * @defgroup freertos
 * @brief FreeRTOS intertask communication
 */

/**
 * @ingroup freertos
 * @brief Queue passes laptimes from main logic laptimer_task to sdcard_task
 */
extern QueueHandle_t sd_queue;

/**
 * @ingroup freertos
 * @brief Semaphore tries to reinit sd card when reset button is pushed
 * (may be changed to sd card adapter with card detection pin for better reliability)
 */
extern QueueHandle_t sd_reinit_semaphore;

/**
 * @ingroup freertos
 * @brief Queue passes current laptime from main logic laptimer_task to lcd_task and wifi_task
 */
extern QueueHandle_t lcd_laptime_current_queue;
extern QueueHandle_t wifi_laptime_current_queue;

extern QueueHandle_t lcd_laptime_penalty_semaphore;
extern QueueHandle_t wifi_laptime_penalty_semaphore;

/**
 * @ingroup freertos
 * @brief Semaphore allows lcd_task and wifi_task to read stored laptime lists
 */
extern QueueHandle_t lcd_laptime_lists_semaphore;
extern QueueHandle_t wifi_laptime_lists_semaphore;

/**
 * @ingroup freertos
 * @brief Queue passes active status flags from main logic laptimer_task to lcd_task and wifi_task
 * bool[0] - mode
 * bool[1] - stop_flag
 * bool[2] - sdcard_flag
 */
extern QueueHandle_t lcd_laptime_status_queue;
extern QueueHandle_t wifi_laptime_status_queue;

/**
 * @brief Global variables locally store laptime lists used by lcd_task and wifi_task
 * [0] - top laptimes list
 * [1] - last laptimes list
 */
extern char list_top_str[LAPTIME_LIST_SIZE_WIFI][LAPTIME_STR_LENGTH];
extern char list_last_str[LAPTIME_LIST_SIZE_WIFI][LAPTIME_STR_LENGTH];
extern char list_penalty_time_str[LAPTIME_LIST_SIZE_WIFI][PENALTY_TIME_STR_LENGTH];
extern char list_penalty_count_str[LAPTIME_LIST_SIZE_WIFI][PENALTY_COUNT_STR_LENGTH];

extern char penalty_time_str[11];
extern char penalty_count_str[25];
