#pragma once

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_err.h"

#include <esp_wifi_types_generic.h>

#define WIFI_SSID_STR_LENGTH 32
#define WIFI_PASSWORD_STR_LENGTH 64
#define WIFI_IP_LENGTH 52

#define TIMEOFDAY_STR_LENGTH 9
#define DATE_STR_LENGTH 11