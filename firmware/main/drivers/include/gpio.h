#pragma once

#include "driver/gpio.h"

#define LAP_GATE1_BTN GPIO_NUM_32
#define LAP_GATE2_BTN GPIO_NUM_33
#define LAP_RESET_BTN GPIO_NUM_25
#define LAP_MODE_BTN GPIO_NUM_26

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t gpio_init();

#ifdef __cplusplus
}
#endif