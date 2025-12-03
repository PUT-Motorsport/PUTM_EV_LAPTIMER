#pragma once

#include "esp_err.h"

#include "main.h"

struct Laptime
{
    volatile uint16_t count = 1;
    volatile uint64_t time = 0;
    uint64_t penalty_time = 0;
    uint16_t doo_count = 0;
    uint16_t oc_count = 0;
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
