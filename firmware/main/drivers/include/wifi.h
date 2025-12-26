#pragma once

#include "main.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t wifi_init(wifi_mode_t wifi_mode, char wifi_ssid[WIFI_SSID_STR_LENGTH], char wifi_password[WIFI_PASSWORD_STR_LENGTH]);
    esp_err_t wifi_reinit(wifi_mode_t wifi_mode, char wifi_ssid[WIFI_SSID_STR_LENGTH], char wifi_password[WIFI_PASSWORD_STR_LENGTH]);
    esp_err_t wifi_get_ip(char ip_string[52]);

#ifdef __cplusplus
}
#endif
