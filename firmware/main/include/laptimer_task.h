#pragma once

#include "main.h"

/// @brief Minimal time to save in ms
#define LAPTIME_MIN 500

/// @brief Time penalty added to current laptime by LAP_DOO_PIN and LAP_OC_PIN in ms
#define DOO_TIME_PENALTY (uint32_t)200
#define OC_TIME_PENALTY (uint32_t)1000

enum button_state
{
    BTN_STANDBY,
    BTN_HOLD_WAIT,
    BTN_HOLD_ACTION,
    BTN_RELEASED_ACTION,
    BTN_AFTER_HOLD,
};

struct Button_press
{
    volatile TickType_t time = 0;
    volatile button_state state = BTN_STANDBY;
};

void laptimer_task(void *args);

esp_err_t isr_init();
