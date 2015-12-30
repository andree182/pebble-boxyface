#include <pebble.h>

#define WIDGET_BORDER (6)
#define BORDER_OFFSET (WIDGET_BORDER * 2)

#define TIME_DIGIT_W (36)
#define TIME_DIGIT_H (42)
#define TIME_DIGIT_COLS (4)
#define TIME_DIGIT_ROWS (5)

#define TIME_WIDGET_H (TIME_DIGIT_H + WIDGET_BORDER * 2)
#define BACKGROUND_COLOR (GColorBlue)
#define DIGIT_BACKGROUND_COLOR (GColorWhite)
#define DIGIT_COLOR (GColorBlack)
#define DIGIT_BORDER_COLOR (GColorBlack)

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
