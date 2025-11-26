#pragma once

#include "main.h"

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

#define CURRENT_LAPTIME_FONT 20
#define UI_FONT 8

#define CURRENT_LAPTIME_LETTER_WIDTH 11
#define UI_LETTER_WIDTH 5

void lcd_task(void *args);
