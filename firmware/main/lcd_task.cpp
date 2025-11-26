#include "lcd_task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/idf_additions.h"

#include "hagl.h"
#include "hagl_hal.h"
#include "font6x9.h"
#include "font9x15-ISO8859-1.h"

#include <cwchar>

hagl_backend_t display_struct;
hagl_backend_t *display = &display_struct;

void lcd_init() { hagl_hal_init(display); }

void lcd_clear() { hagl_clear(display); }
void lcd_copy() { hagl_flush(display); }

bool lcd_print_wstr(int16_t pos_x, int16_t pos_y, const wchar_t *string,
                    int font_size, int16_t color)
{
    if (string == NULL || pos_y > LCD_HEIGHT || pos_x > LCD_WIDTH)
        return 1;
    const unsigned char *font;
    switch (font_size)
    {
    case 8:
        font = font6x9;
        break;
    // case 12:
    //     font = &Font12;
    //     break;
    case 16:
        font = font6x9;
        break;
    // case 20:
    //     font = font9x15_ISO8859_1;
    //     break;
    // case 24:
    //     font = &Font24;
    //     break;
    default:
        return 1;
    }
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
    lcd_print_wstr(PADDING, PADDING, L"CURRENT LAP", UI_FONT, WHITE);
    lcd_print_wstr(LAPLIST_POS_X, LAPLIST_POS_Y,
                   L"LAST 5 LAPS", UI_FONT,
                   WHITE);
    lcd_print_wstr(LAPLIST_POS_X + LCD_WIDTH / 2, LAPLIST_POS_Y,
                   L"TOP  5 LAPS", UI_FONT,
                   WHITE);
    lcd_print_line(0, LAPLIST_POS_Y - PADDING, LCD_WIDTH, LAPLIST_POS_Y - PADDING,
                   WHITE);
    lcd_print_line(LCD_WIDTH / 2, LAPLIST_POS_Y - PADDING, LCD_WIDTH / 2,
                   LCD_HEIGHT, WHITE);
}

void print_status()
{
    static bool status[3] = {false};
    xQueueReceive(lcd_laptime_status_queue, status, 0);
    bool mode = status[0];
    bool stop_flag = status[1];
    bool sd_flag = status[2];
    switch (mode)
    {
    case false:
        lcd_print_wstr(LCD_WIDTH - UI_LETTER_WIDTH * 6 - PADDING, PADDING, L"1 GATE ",
                       UI_FONT, GRAY);
        break;
    case true:
        lcd_print_wstr(LCD_WIDTH - UI_LETTER_WIDTH * 6 - PADDING, PADDING, L"2 GATE ",
                       UI_FONT, GRAY);
        break;
    default:
        break;
    }

    if (stop_flag)
        lcd_print_wstr(LCD_WIDTH / 2, PADDING, L"STOP", UI_FONT, RED);
    else
        lcd_print_wstr(LCD_WIDTH / 2, PADDING, L"    ", UI_FONT, BLACK);

    if (sd_flag)
        lcd_print_wstr(LCD_WIDTH / 2 + UI_LETTER_WIDTH * 5, PADDING, L"SD",
                       UI_FONT, GREEN);
    else
        lcd_print_wstr(LCD_WIDTH / 2 + UI_LETTER_WIDTH * 5, PADDING, L"  ",
                       UI_FONT, BLACK);
}

// bool lcdIsBusy() { return dmaBusyFlag; }

// void lcdSetFree() { dmaBusyFlag = false; }

void print_current_laptime()
{
    static wchar_t laptime_current_wstr[LAPTIME_STRING_LENGTH] = L"--,--:--:--";
    xQueueReceive(lcd_laptime_current_queue, laptime_current_wstr, 0);
    lcd_print_wstr(CURRENT_LAPTIME_POS_X, CURRENT_LAPTIME_POS_Y, laptime_current_wstr, CURRENT_LAPTIME_FONT, WHITE);
}

void print_laptime_lists()
{
    static wchar_t list_wch[2][LAPTIME_LIST_SIZE_LCD][LAPTIME_STRING_LENGTH];
    xQueueReceive(lcd_laptime_lists_queue, list_wch, 0);
    for (int i = 0; i < LAPTIME_LIST_SIZE_LCD; i++)
    {
        lcd_print_wstr(LAPLIST_POS_X, LAPLIST_POS_Y + LAPLIST_SPACING + i * LAPLIST_SPACING, list_wch[0][i],
                       UI_FONT, WHITE);
        lcd_print_wstr(LCD_WIDTH / 2 + LAPLIST_POS_X,
                       LAPLIST_POS_Y + LAPLIST_SPACING + i * LAPLIST_SPACING, list_wch[1][i],
                       UI_FONT, WHITE);
    }
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