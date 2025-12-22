#pragma once

#include "main.h"

/// @brief Size of wifi displayed best/last laptime list
#define LAPTIME_LIST_SIZE_WIFI 50

#ifdef __cplusplus
extern "C"
{
#endif

    void wifi_task(void *args);

#ifdef __cplusplus
}
#endif
