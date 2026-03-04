#pragma once

#include "main.hpp"

enum Button_state
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
    volatile Button_state state = BTN_STANDBY;
};

void laptimer_task(void *args);

esp_err_t isr_init();
