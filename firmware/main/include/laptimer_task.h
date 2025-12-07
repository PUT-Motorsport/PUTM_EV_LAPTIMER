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

    bool penalty_check(bool btn_state, bool *press_flag_state, TickType_t press_time, uint32_t penalty)
    {
        uint16_t *count;
        switch (penalty)
        {
        case DOO_TIME_PENALTY:
            count = &(this->doo_count);
            break;
        case OC_TIME_PENALTY:
            count = &(this->oc_count);
            break;
        default:
            count = &(this->doo_count);
            break;
        }

        if (btn_state == 0 && *press_flag_state == false)
        {
            if (press_time > pdMS_TO_TICKS(1000))
            {
                if (this->penalty_time >= penalty && *count > 0)
                {
                    (*count)--;
                    this->penalty_time -= penalty;
                }
                *press_flag_state = true;
            }
        }
        else if (btn_state == 1 && *press_flag_state == true)
        {
            *press_flag_state = false;
            return false;
        }
        else if (*press_flag_state == false)
        {
            (*count)++;
            this->penalty_time += penalty;
            return false;
        }
        return true;
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

enum Lapmode
{
    ONE_GATE_MODE,
    TWO_GATE_MODE,
};

void laptimer_task(void *args);

void laptime_convert_string(Laptime laptime, char *laptime_str, size_t size);

esp_err_t isr_init();
