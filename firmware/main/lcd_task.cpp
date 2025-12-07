#include "lcd_task.h"

#include "hagl.h"
#include "hagl_hal.h"
#include "font10x20-ISO8859-1.h"
#include "font9x15-ISO8859-1.h"

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

#define PADDING 10
#define LAPTIME_LISTS_POS_X 10
#define LAPTIME_LISTS_POS_Y 100
#define LAPTIME_LISTS_SPACING 20
#define LAPTIME_CURRENT_POS_X (LCD_WIDTH / 2 - LAPTIME_CURRENT_LETTER_WIDTH * 7)
#define LAPTIME_CURRENT_POS_Y 45

#define LAPTIME_CURRENT_FONT font10x20_ISO8859_1
#define LAPTIME_LISTS_FONT font10x20_ISO8859_1
#define UI_FONT font9x15_ISO8859_1

#define LAPTIME_CURRENT_LETTER_WIDTH 10
#define LAPTIME_LISTS_LETTER_WIDTH 10
#define UI_LETTER_WIDTH 9

hagl_backend_t display_struct;
hagl_backend_t *display = &display_struct;

/// @brief Interface for hagl functions
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

/**
 * @brief Prints static ui elements on LCD
 */
void print_ui()
{
    lcd_print_str(PADDING, PADDING, "CURRENT LAP", UI_FONT, WHITE);
    lcd_print_str(LAPTIME_LISTS_POS_X, LAPTIME_LISTS_POS_Y,
                  "LAST 5 LAPS", UI_FONT,
                  WHITE);
    lcd_print_str(LAPTIME_LISTS_POS_X + LCD_WIDTH / 2, LAPTIME_LISTS_POS_Y,
                  "TOP  5 LAPS", UI_FONT,
                  WHITE);
    lcd_print_line(0, LAPTIME_LISTS_POS_Y - PADDING, LCD_WIDTH, LAPTIME_LISTS_POS_Y - PADDING,
                   WHITE);
    lcd_print_line(LCD_WIDTH / 2, LAPTIME_LISTS_POS_Y - PADDING, LCD_WIDTH / 2,
                   LCD_HEIGHT, WHITE);
}

/**
 * @brief Prints status flag values received from queue on LCD
 */
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

/**
 * @brief Prints current laptime received from queue on LCD
 */
void print_current_laptime()
{
    char laptime_current_str[LAPTIME_STR_LENGTH] = "--, --:--.--";
    if (xQueueReceive(lcd_laptime_current_queue, laptime_current_str, 0) != pdTRUE)
        return;
    lcd_print_str(LAPTIME_CURRENT_POS_X, LAPTIME_CURRENT_POS_Y, laptime_current_str, LAPTIME_CURRENT_FONT, WHITE);
}

void print_penalty()
{
    if (xSemaphoreTake(lcd_laptime_penalty_semaphore, 0) == pdTRUE)
    {
        lcd_print_str(LAPTIME_CURRENT_POS_X + LAPTIME_CURRENT_LETTER_WIDTH * 13, LAPTIME_CURRENT_POS_Y, penalty_time_str, UI_FONT, YELLOW);
        lcd_print_str(LAPTIME_LISTS_POS_X, LAPTIME_CURRENT_POS_Y + 25, penalty_count_str, UI_FONT, YELLOW);
        xSemaphoreGive(lcd_laptime_penalty_semaphore);
    }
}

/**
 * @brief Reads laptime lists from global variable after receiving samphore and prints them on LCD
 */
void print_laptime_lists()
{
    if (xSemaphoreTake(lcd_laptime_lists_semaphore, 0) != pdTRUE)
        return;
    for (int i = 0; i < LAPTIME_LIST_SIZE_LCD; i++)
    {
        lcd_print_str(LCD_WIDTH / 2 + LAPTIME_LISTS_POS_X,
                      LAPTIME_LISTS_POS_Y + LAPTIME_LISTS_SPACING + i * LAPTIME_LISTS_SPACING, list_top_str[i],
                      LAPTIME_LISTS_FONT, WHITE);
        lcd_print_str(LAPTIME_LISTS_POS_X, LAPTIME_LISTS_POS_Y + LAPTIME_LISTS_SPACING + i * LAPTIME_LISTS_SPACING, list_last_str[i],
                      LAPTIME_LISTS_FONT, WHITE);
    }
    xSemaphoreGive(lcd_laptime_lists_semaphore);
}

/**
 * @brief LCD task initializes screen and graphics library,
 * then prints values received from laptimer_task on screen
 */
void lcd_task(void *args)
{

    lcd_init();
    lcd_clear();
    lcd_set_clip(0, 0, LCD_WIDTH, LCD_HEIGHT);
    print_ui();

    for (;;)
    {
        print_current_laptime();
        print_penalty();
        print_laptime_lists();
        print_status();
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}