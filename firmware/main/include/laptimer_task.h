#pragma once

#include "esp_err.h"

#include <stddef.h>

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

extern Laptime laptime_current;
extern Laptime laptime_saved;

extern volatile Lapmode lap_mode;

extern volatile bool stop_flag;

void laptimer_task(void *args);

// lap_mode_t lap_mode_check();
// void laptime_reset(laptime_t *laptime);
// void laptime_save_local(laptime_t *laptime);
// void laptime_convert(laptime_t laptime, char *laptime_str, size_t size);
// void laptime_save_uart(char *laptime_str, size_t size);

// void show_laptime(laptime_t laptime, int pos_x, int pos_y, int font);
// void show_ui();
// void show_status();
// void show_laptime_lists();

esp_err_t isr_init();
