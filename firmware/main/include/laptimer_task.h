#pragma once

#include "esp_err.h"

#include "main.h"

class Laptime
{
public:
    volatile uint16_t count = 1;
    volatile uint64_t time = 0;
    uint64_t penalty_time = 0;
    uint16_t doo_count = 0;
    uint16_t oc_count = 0;

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

    void convert_string(char laptime_str[LAPTIME_STRING_LENGTH], size_t size)
    {
        if (laptime_str == NULL)
            return;

        if (this->time == 0)
        {
            snprintf(laptime_str, size, "--, --:--:--");
            return;
        }

        unsigned int mm = (this->time / 6000) % 60;
        unsigned int ss = (this->time / 100) % 60;
        unsigned int ms = this->time % 100;
        if (size == LAPTIME_STRING_LENGTH)
            snprintf(laptime_str, size, "%02u, %02u:%02u:%02u",
                     this->count, mm, ss, ms);
    }
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
