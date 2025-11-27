#pragma once

#include "esp_err.h"

#include "main.h"

struct Laptime
{
    volatile unsigned int count;
    volatile unsigned long time;
};

struct Laptime_list
{
    Laptime *list_top;
    Laptime *list_last;
};

enum Lapmode
{
    ONE_GATE_MODE,
    TWO_GATE_MODE,
};

void laptimer_task(void *args);

void laptime_convert_string(Laptime laptime, char *laptime_str, size_t size);

esp_err_t isr_init();
