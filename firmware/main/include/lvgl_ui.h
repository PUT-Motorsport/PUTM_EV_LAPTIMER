#pragma once

#include "main.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    void ui_init(void);

    void ui_update_sd(bool sd_status);
    void ui_update_wifi(wifi_mode_t wifi_mode, const char ip_str[WIFI_IP_LENGTH]);
    void ui_update_gates_mode(bool two_gates_mode);
    void ui_update_stop_status(bool stop_flag);

    void ui_update_current_lap(const char *time_str, const char *penalty_str, int lap_count, int oc_count, int doo_count, const char *driver_name, int driver_id);
    void ui_update_top_lap(int index, const char *lap_str, const char *time_str, const char *driver_str, int driver_id);
    void ui_update_last_lap(int index, const char *lap_str, const char *time_str, const char *driver_str, int driver_id);

#ifdef __cplusplus
}
#endif
