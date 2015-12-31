#include <pebble.h>

#define WIDGET_BORDER (6)
#define BORDER_OFFSET (WIDGET_BORDER * 3 / 2)

#define TIME_DIGIT_W (36)
#define TIME_DIGIT_H (42)
#define TIME_DIGIT_COLS (4)
#define TIME_DIGIT_ROWS (5)

#define TIME_WIDGET_H (TIME_DIGIT_H + WIDGET_BORDER * 2)
#define BACKGROUND_COLOR (GColorBlue)
#define DIGIT_BACKGROUND_COLOR (GColorWhite)
#define DIGIT_COLOR (GColorBlack)
#define DIGIT_BORDER_COLOR (GColorBlack)

#define CALENDAR_TEXT_FONT FONT_KEY_GOTHIC_18_BOLD
#define CALENDAR_TEXT_H (18)
#define CALENDAR_W (TIME_DIGIT_W * 4)
#define CALENDAR_H (TIME_DIGIT_H + 2 * CALENDAR_TEXT_H)

// #define CALENDAR_WIDGET_W (CALENDAR_W + WIDGET_BORDER * 2)
#define CALENDAR_WIDGET_W (CALENDAR_W)
#define CALENDAR_WIDGET_H (CALENDAR_H + WIDGET_BORDER * 2)

typedef struct {
	Layer *layer;
	int   curDigit;
} DigitSlot;

const char digits[][5] = {
	{
		0b1111,
		0b1001,
		0b1001,
		0b1001,
		0b1111,
	},
	{
		0b0110,
		0b0010,
		0b0010,
		0b0010,
		0b0010,
	},
	{
		0b1111,
		0b0001,
		0b1111,
		0b1000,
		0b1111,
	},
	{
		0b1111,
		0b0001,
		0b1111,
		0b0001,
		0b1111,
	},
	{
		0b1001,
		0b1001,
		0b1111,
		0b0001,
		0b0001,
	},
	{
		0b1111,
		0b1000,
		0b1111,
		0b0001,
		0b1111,
	},
	{
		0b1000,
		0b1000,
		0b1111,
		0b1001,
		0b1111,
	},
	{
		0b1111,
		0b0001,
		0b0001,
		0b0001,
		0b0001,
	},
	{
		0b1111,
		0b1001,
		0b1111,
		0b1001,
		0b1111,
	},
	{
		0b1111,
		0b1001,
		0b1111,
		0b0001,
		0b0001,
	},
};
