#include <pebble.h>

#define TIME_DIGIT_W (36)
#define TIME_DIGIT_H (42)
#define TIME_BOTTOM_OFFSET (6)

static Window *window;
static GBitmap *digit_bitmaps[10];
static BitmapLayer *digit_layers[4];

const int DIGIT_RES[10] = {
	RESOURCE_ID_IMAGE_N0, RESOURCE_ID_IMAGE_N1, RESOURCE_ID_IMAGE_N2,
	RESOURCE_ID_IMAGE_N3, RESOURCE_ID_IMAGE_N4, RESOURCE_ID_IMAGE_N5,
	RESOURCE_ID_IMAGE_N6, RESOURCE_ID_IMAGE_N7, RESOURCE_ID_IMAGE_N8,
	RESOURCE_ID_IMAGE_N9
};

static unsigned short handle_12_24(unsigned short hour) {
	if (clock_is_24h_style()) {
		return hour;
	}

	hour %= 12;
	return (hour != 0) ? hour : 12;
}

static void display_value(unsigned short value, unsigned short layer_offset, bool leading_zero) {
	for (int col = 1; col >= 0; col--) {
		bitmap_layer_set_bitmap(
			digit_layers[layer_offset + col],
			digit_bitmaps[value % 10]
		);
		value /= 10;
	}
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	display_value(handle_12_24(tick_time->tm_hour), 0, true);
	display_value(tick_time->tm_min, 2, true);
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	int i;
	struct tm *tick_time;

	window_set_background_color(window, GColorBlack);

	for (i = 0; i <= 9; i++) {
		digit_bitmaps[i] = gbitmap_create_with_resource(DIGIT_RES[i]);
	}
	
	for (i = 0; i < 4; i++) {
		digit_layers[i] =
			bitmap_layer_create(
				GRect(i * TIME_DIGIT_W, bounds.size.h - TIME_BOTTOM_OFFSET - TIME_DIGIT_H,
					  TIME_DIGIT_W, TIME_DIGIT_H)
			);
		bitmap_layer_set_bitmap(digit_layers[i], digit_bitmaps[0]);
		layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(digit_layers[i]));
	}

	// initial values
	time_t temp;
	temp = time(NULL);
	tick_time = localtime(&temp);
	tick_handler(tick_time, MINUTE_UNIT);
}

static void window_unload(Window *window) {
	unsigned i;

	tick_timer_service_unsubscribe();

	for (i = 0; i < sizeof(digit_layers) / sizeof(digit_layers[0]); i++) {
		layer_remove_from_parent(bitmap_layer_get_layer(digit_layers[i]));
		bitmap_layer_destroy(digit_layers[i]);
	}

	for (i = 0; i < sizeof(digit_bitmaps) / sizeof(digit_bitmaps[0]); i++) {
		gbitmap_destroy(digit_bitmaps[i]);
	}
}

static void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	const bool animated = true;
	window_stack_push(window, animated);

	tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler) tick_handler);	
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
