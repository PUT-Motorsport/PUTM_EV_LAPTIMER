#define LCD_RES_H 320
#define LCD_RES_V 240

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_ili9341.h"

#ifdef __cplusplus
extern "C"
{
#endif

    lv_disp_t *lcd_init(void);

#ifdef __cplusplus
}
#endif
