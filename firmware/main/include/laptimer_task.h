#pragma once

#include "esp_err.h"

#include "main.h"

enum btn_long_state
{
    BTN_STANDBY,
    BTN_HOLD_WAIT,
    BTN_HOLD_ACTION,
    BTN_RELEASED_ACTION,
    BTN_AFTER_HOLD,
};

void laptimer_task(void *args);

esp_err_t isr_init();
