#include <pebble.h>

#define TIME_DIGIT_W (36)
#define TIME_DIGIT_H (42)
#define TIME_DIGIT_COLS (4)
#define TIME_DIGIT_ROWS (5)
#define TIME_DIGIT_BORDER (6)

#define TIME_BOTTOM_OFFSET (6 * 3)
#define BACKGROUND_COLOR (GColorBlue)
#define DIGIT_BACKGROUND_COLOR (GColorWhite)
#define DIGIT_COLOR (GColorBlack)
#define DIGIT_BORDER_COLOR (GColorBlack)
#define DIGIT_BORDER_HEIGHT (TIME_DIGIT_BORDER / 2)

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
