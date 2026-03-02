#pragma once

#include "main.h"

#include "driver/gpio.h"

/**
 * @defgroup pinout_defines
 * @brief Pinout defines
 * @{
 */

/// @brief Input for first gate that starts new lap on negative edge
#define LAP_GATE1_PIN ((gpio_num_t)CONFIG_GATE1_PIN)

/// @brief Input for second gate that stops lap on negative edge in 2 gate mode
#define LAP_GATE2_PIN ((gpio_num_t)CONFIG_GATE2_PIN)

/// @brief Input for button that resets current laptime and stops timer
#define LAP_RESET_PIN ((gpio_num_t)CONFIG_STOP_PIN)

/// @brief Input for button that adds Down or Out time penalty to current laptime (2s for Endurance) on quick press and removes penalty on long press
#define LAP_DOO_PIN ((gpio_num_t)CONFIG_DOO_PIN)

/// @brief Input for button that adds Off-Course time penalty to current laptime (10s for Endurance) on quick press and removes penalty on long press
#define LAP_OC_PIN ((gpio_num_t)CONFIG_OC_PIN)

/// @brief Input for button that changes current driver
#define DRIVER_SELECT_PIN ((gpio_num_t)CONFIG_DRIVER_SELECT_PIN)

/// @brief Input for button that resets wifi with settings from actual config on quick press and resets wifi with default settings on long press
#define WIFI_PIN ((gpio_num_t)CONFIG_WIFI_PIN)

/// @brief LCD pinout
#define LCD_SPI_CLK CONFIG_MIPI_DISPLAY_PIN_CLK
#define LCD_SPI_MOSI CONFIG_MIPI_DISPLAY_PIN_MOSI
#define LCD_SPI_CS CONFIG_MIPI_DISPLAY_PIN_CS
#define LCD_DC CONFIG_MIPI_DISPLAY_PIN_DC
#define LCD_RESET CONFIG_MIPI_DISPLAY_PIN_RST
#define LCD_BL CONFIG_MIPI_DISPLAY_PIN_BL
/// @}

/// @brief SPI_HOST defines for peripherals
#define SPI_SD_HOST SPI2_HOST
#define SPI_LCD_HOST CONFIG_MIPI_DISPLAY_SPI_HOST

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t gpio_init();

#ifdef __cplusplus
}
#endif