#pragma once

#include "defines.h"

#define LCD_RES_H 320
#define LCD_RES_V 240

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t lcd_init(void);

#ifdef __cplusplus
}
#endif
