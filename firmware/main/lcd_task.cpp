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
#define PURPLE 0x1080
#define OLIVE 0x0084

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

static char driver_list_local[DRIVER_MAX_COUNT][DRIVER_TAG_LENGTH] = DRIVER_LIST_DEFAULT;
const int driver_color_list[DRIVER_MAX_COUNT] = {BLACK, BLUE, RED, YELLOW, GREEN, MAGENTA, BROWN, CYAN, PURPLE, OLIVE};

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

void lcd_print_tag(int16_t pos_x, int16_t pos_y, int16_t width, int16_t height, int16_t color)
{
    hagl_fill_rounded_rectangle(display, pos_x, pos_y, pos_x + width, pos_y + height, 5, (hagl_color_t)color);
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
    lcd_print_line(LCD_WIDTH / 2, LAPTIME_CURRENT_POS_Y + 25, LCD_WIDTH / 2,
                   LCD_HEIGHT, WHITE);
    lcd_print_str(LAPTIME_LISTS_POS_X, LAPTIME_CURRENT_POS_Y + 25, "OC:", UI_FONT, YELLOW);
    lcd_print_str(LAPTIME_LISTS_POS_X + UI_LETTER_WIDTH * 8, LAPTIME_CURRENT_POS_Y + 25, "DOO:", UI_FONT, YELLOW);
    lcd_print_str(LAPTIME_LISTS_POS_X + LCD_WIDTH / 2, LAPTIME_CURRENT_POS_Y + 25, "DRIVER:", UI_FONT, WHITE);
}

/**
 * @brief Prints status flag values received from queue on LCD
 */
void print_status()
{
    bool status_list[3] = {0};
    if (xQueueReceive(laptime_status_queue_lcd, status_list, 0) == pdTRUE)
    {
        if (!status_list[0])
        {
            lcd_print_str(LCD_WIDTH - UI_LETTER_WIDTH * 6 - PADDING, PADDING, "1 GATE ",
                          UI_FONT, GRAY);
        }
        else
        {
            lcd_print_str(LCD_WIDTH - UI_LETTER_WIDTH * 6 - PADDING, PADDING, "2 GATE ",
                          UI_FONT, GRAY);
        }
        if (status_list[1])
            lcd_print_str(LCD_WIDTH / 2, PADDING, "STOP", UI_FONT, RED);
        else
            lcd_print_str(LCD_WIDTH / 2, PADDING, "    ", UI_FONT, BLACK);

        if (status_list[2])
            lcd_print_str(LCD_WIDTH / 2 + UI_LETTER_WIDTH * 5, PADDING, "SD",
                          UI_FONT, GREEN);
        else
            lcd_print_str(LCD_WIDTH / 2 + UI_LETTER_WIDTH * 5, PADDING, "  ",
                          UI_FONT, BLACK);
    }
}

/**
 * @brief Prints current laptime received from queue on LCD
 */
void print_current_laptime()
{
    static Laptime laptime_current;

    if (xQueueReceive(laptime_current_queue_lcd, &laptime_current, 0) != pdTRUE)
        return;

    char laptime_current_str[LAPTIME_STR_LENGTH] = {0};
    char laptime_penalty_str[PENALTY_TIME_STR_LENGTH] = {0};
    char laptime_oc_str[PENALTY_COUNT_STR_LENGTH] = {0};
    char laptime_doo_str[PENALTY_COUNT_STR_LENGTH] = {0};

    laptime_current.convert_string(laptime_current_str, sizeof(laptime_current_str));
    laptime_current.penalty_string(laptime_penalty_str, sizeof(laptime_penalty_str));
    snprintf(laptime_oc_str, sizeof(laptime_oc_str), "%3u", laptime_current.oc_count);
    snprintf(laptime_doo_str, sizeof(laptime_doo_str), "%3u", laptime_current.doo_count);

    lcd_print_str(LAPTIME_CURRENT_POS_X, LAPTIME_CURRENT_POS_Y, laptime_current_str, LAPTIME_CURRENT_FONT, WHITE);
    lcd_print_str(LAPTIME_CURRENT_POS_X + LAPTIME_CURRENT_LETTER_WIDTH * 13, LAPTIME_CURRENT_POS_Y, laptime_penalty_str, UI_FONT, YELLOW);
    lcd_print_str(LAPTIME_LISTS_POS_X + UI_LETTER_WIDTH * 4, LAPTIME_CURRENT_POS_Y + 25, laptime_oc_str, UI_FONT, YELLOW);
    lcd_print_str(LAPTIME_LISTS_POS_X + UI_LETTER_WIDTH * 13, LAPTIME_CURRENT_POS_Y + 25, laptime_doo_str, UI_FONT, YELLOW);
    lcd_print_str(LAPTIME_LISTS_POS_X + UI_LETTER_WIDTH * 26, LAPTIME_CURRENT_POS_Y + 25, driver_list_local[laptime_current.driver_id], UI_FONT, WHITE);
    lcd_print_tag(LAPTIME_LISTS_POS_X + UI_LETTER_WIDTH * 30, LAPTIME_CURRENT_POS_Y + 25, 25, 10, driver_color_list[laptime_current.driver_id]);
}

/**
 * @brief Reads laptime lists from global variable after receiving samphore and prints them on LCD
 */
void print_laptime_lists()
{
    if (xSemaphoreTake(laptime_lists_mutex, 0) != pdTRUE)
        return;
    char laptime_top_str[LAPTIME_STR_LENGTH] = {0};
    char laptime_last_str[LAPTIME_STR_LENGTH] = {0};
    char laptime_driver_str[LAPTIME_STR_LENGTH] = {0};

    for (int i = 0; i < LAPTIME_LIST_SIZE_LCD; i++)
    {
        laptime_list_top[i].convert_string(laptime_top_str, sizeof(laptime_top_str));
        laptime_list_last[i].convert_string(laptime_last_str, sizeof(laptime_last_str));
        laptime_list_driver[i].convert_string(laptime_driver_str, sizeof(laptime_driver_str));

        lcd_print_str(LCD_WIDTH / 2 + LAPTIME_LISTS_POS_X,
                      LAPTIME_LISTS_POS_Y + LAPTIME_LISTS_SPACING + i * LAPTIME_LISTS_SPACING, laptime_top_str,
                      LAPTIME_LISTS_FONT, WHITE);
        lcd_print_tag(LCD_WIDTH / 2 + LAPTIME_LISTS_POS_X + LAPTIME_CURRENT_LETTER_WIDTH * 12 + 10, LAPTIME_LISTS_POS_Y + LAPTIME_LISTS_SPACING + i * LAPTIME_LISTS_SPACING + 5, 8, 8, driver_color_list[laptime_list_top[i].driver_id]);
        lcd_print_str(LAPTIME_LISTS_POS_X, LAPTIME_LISTS_POS_Y + LAPTIME_LISTS_SPACING + i * LAPTIME_LISTS_SPACING, laptime_last_str,
                      LAPTIME_LISTS_FONT, WHITE);
        lcd_print_tag(LAPTIME_LISTS_POS_X + LAPTIME_CURRENT_LETTER_WIDTH * 12 + 10, LAPTIME_LISTS_POS_Y + LAPTIME_LISTS_SPACING + i * LAPTIME_LISTS_SPACING + 5, 8, 8, driver_color_list[laptime_list_last[i].driver_id]);
    }
    xSemaphoreGive(laptime_lists_mutex);
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
        if (xSemaphoreTake(driver_list_mutex, 0) == pdTRUE)
        {
            memcpy(driver_list_local, driver_list, sizeof(driver_list_local));
            xSemaphoreGive(driver_list_mutex);
        }

        print_current_laptime();
        print_laptime_lists();
        print_status();
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}