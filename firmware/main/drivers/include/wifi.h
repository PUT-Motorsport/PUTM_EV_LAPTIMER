#pragma once

#include "esp_err.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t wifi_init(wifi_mode_t wifi_mode, char wifi_ssid[32], char wifi_password[64]);
    esp_err_t wifi_reinit(wifi_mode_t wifi_mode, char wifi_ssid[32], char wifi_password[64]);

#ifdef __cplusplus
}
#endif
