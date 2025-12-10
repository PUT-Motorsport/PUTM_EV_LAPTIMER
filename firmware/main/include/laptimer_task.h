#pragma once

#include "esp_err.h"

#include "main.h"

class Laptime
{
public:
    volatile uint16_t count = 1;
    volatile uint64_t time = 0;

    uint32_t penalty_time = 0;
    uint16_t doo_count = 0;
    uint16_t oc_count = 0;

    int16_t driver_id = 0;
    char *driver_tag = "XXX";

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

    void penalty_string(char laptime_str[11], size_t size)
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

struct Laptime_list
{
    Laptime list_top[LAPTIME_LIST_SIZE_LOCAL] = {0};
    Laptime list_last[LAPTIME_LIST_SIZE_LOCAL] = {0};
};

enum btn_long_state
{
    BTN_STANDBY,
    BTN_HOLD_WAIT,
    BTN_HOLD_ACTION,
    BTN_RELEASED_ACTION,
    BTN_AFTER_HOLD,
};

void laptimer_task(void *args);

void laptime_convert_string(Laptime laptime, char *laptime_str, size_t size);

esp_err_t isr_init();
