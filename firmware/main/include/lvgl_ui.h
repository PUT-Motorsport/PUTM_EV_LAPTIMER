#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void ui_init(void);

    void ui_update_status(bool sd_on, int wifi_mode, const char *ip_str, bool two_gates, bool stop_flag);
    void ui_update_current_lap(const char *time_str, const char *penalty_str, int oc_count, int doo_count, const char *driver_name);
    void ui_update_top_lap(int index, const char *lap_str, const char *time_str, const char *driver_str);
    void ui_update_last_lap(int index, const char *lap_str, const char *time_str, const char *driver_str);

#ifdef __cplusplus
}
#endif
