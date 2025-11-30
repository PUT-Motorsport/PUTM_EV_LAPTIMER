#include "lcd_task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/idf_additions.h"

#include "hagl.h"
#include "hagl_hal.h"
#include "font10x20-ISO8859-1.h"
#include "font6x10-ISO8859-1.h"

#include <cwchar>

#define LCD_WIDTH 320
#define LCD_HEIGHT 240

#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x1F00
#define BRED 0x1FF8
#define GRED 0xE0FF
#define GBLUE 0xFF07
#define RED 0x00F8
#define MAGENTA 0x1FF8
#define GREEN 0xE007
#define CYAN 0xFF7F
#define YELLOW 0xE0FF
#define BROWN 0x40BC
#define BRRED 0x07FC
#define GRAY 0x3084

#define PADDING 5
#define LAPLIST_POS_X 5
#define LAPLIST_POS_Y 40
#define LAPLIST_SPACING 15
#define CURRENT_LAPTIME_POS_X 5
#define CURRENT_LAPTIME_POS_Y 15

#define CURRENT_LAPTIME_FONT font10x20_ISO8859_1
#define UI_FONT font6x10_ISO8859_1

#define CURRENT_LAPTIME_LETTER_WIDTH 11
#define UI_LETTER_WIDTH 5

hagl_backend_t display_struct;
hagl_backend_t *display = &display_struct;

void lcd_init() { hagl_hal_init(display); }

void lcd_clear() { hagl_clear(display); }
void lcd_copy() { hagl_flush(display); }

bool lcd_print_str(int16_t pos_x, int16_t pos_y, const char *string,
                   const unsigned char *font, int16_t color)
{
    if (string == NULL || pos_y > LCD_HEIGHT || pos_x > LCD_WIDTH)
        return 1;
    hagl_put_text(display, string, pos_x, pos_y, color, font);
    return 0;
}

bool lcd_print_line(int16_t pos_x0, int16_t pos_y0, int16_t pos_x1,
                    int16_t pos_y1, int16_t color)
{
    if (pos_x0 > LCD_WIDTH || pos_x1 > LCD_WIDTH || pos_y0 > LCD_HEIGHT ||
        pos_y1 > LCD_HEIGHT || pos_x0 > pos_x1 || pos_y0 > pos_y1)
        return 1;
    hagl_draw_line(display, pos_x0, pos_y0, pos_x1, pos_y1, color);
    return 0;
}

void lcd_set_clip(int16_t pos_x0, int16_t pos_y0, int16_t pos_x1, int16_t pos_y1)
{
    hagl_set_clip(display, pos_x0, pos_y0, pos_x1, pos_y1);
}

void print_ui()
{
    lcd_print_str(PADDING, PADDING, "CURRENT LAP", UI_FONT, WHITE);
    lcd_print_str(LAPLIST_POS_X, LAPLIST_POS_Y,
                  "LAST 5 LAPS", UI_FONT,
                  WHITE);
    lcd_print_str(LAPLIST_POS_X + LCD_WIDTH / 2, LAPLIST_POS_Y,
                  "TOP  5 LAPS", UI_FONT,
                  WHITE);
    lcd_print_line(0, LAPLIST_POS_Y - PADDING, LCD_WIDTH, LAPLIST_POS_Y - PADDING,
                   WHITE);
    lcd_print_line(LCD_WIDTH / 2, LAPLIST_POS_Y - PADDING, LCD_WIDTH / 2,
                   LCD_HEIGHT, WHITE);
}

void print_status()
{
    bool status[3] = {false};
    if (xQueueReceive(lcd_laptime_status_queue, status, 0) != pdTRUE)
        return;
    bool mode = status[0];
    bool stop_flag = status[1];
    bool sd_flag = status[2];
    switch (mode)
    {
    case false:
        lcd_print_str(LCD_WIDTH - UI_LETTER_WIDTH * 6 - PADDING, PADDING, "1 GATE ",
                      UI_FONT, GRAY);
        break;
    case true:
        lcd_print_str(LCD_WIDTH - UI_LETTER_WIDTH * 6 - PADDING, PADDING, "2 GATE ",
                      UI_FONT, GRAY);
        break;
    default:
        break;
    }

    if (stop_flag)
        lcd_print_str(LCD_WIDTH / 2, PADDING, "STOP", UI_FONT, RED);
    else
        lcd_print_str(LCD_WIDTH / 2, PADDING, "    ", UI_FONT, BLACK);

    if (sd_flag)
        lcd_print_str(LCD_WIDTH / 2 + UI_LETTER_WIDTH * 5, PADDING, "SD",
                      UI_FONT, GREEN);
    else
        lcd_print_str(LCD_WIDTH / 2 + UI_LETTER_WIDTH * 5, PADDING, "  ",
                      UI_FONT, BLACK);
}

void print_current_laptime()
{
    static char laptime_current_str[LAPTIME_STRING_LENGTH] = "--,--:--:--";
    if (xQueueReceive(lcd_laptime_current_queue, laptime_current_str, 0) != pdTRUE)
        return;
    lcd_print_str(CURRENT_LAPTIME_POS_X, CURRENT_LAPTIME_POS_Y, laptime_current_str, CURRENT_LAPTIME_FONT, WHITE);
}

void print_laptime_lists()
{
    if (xSemaphoreTake(lcd_laptime_lists_semaphore, 0) != pdTRUE)
        return;
    for (int i = 0; i < LAPTIME_LIST_SIZE_LCD; i++)
    {
        lcd_print_str(LAPLIST_POS_X, LAPLIST_POS_Y + LAPLIST_SPACING + i * LAPLIST_SPACING, lcd_list_buffer[1][i],
                      UI_FONT, WHITE);
        lcd_print_str(LCD_WIDTH / 2 + LAPLIST_POS_X,
                      LAPLIST_POS_Y + LAPLIST_SPACING + i * LAPLIST_SPACING, lcd_list_buffer[0][i],
                      UI_FONT, WHITE);
    }
    xSemaphoreGive(lcd_laptime_lists_semaphore);
}

void lcd_task(void *args)
{

    lcd_init();
    lcd_clear();
    lcd_set_clip(0, 0, LCD_WIDTH, LCD_HEIGHT);
    print_ui();

    for (;;)
    {
        print_current_laptime();
        print_laptime_lists();
        print_status();
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}