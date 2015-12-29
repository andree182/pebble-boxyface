#include "cleanface.h"

static Window *window;
static Layer *digitsLayer;
static DigitSlot digitSlots[4];


static void update_digits_layer(Layer *layer, GContext *ctx) {
	GRect r;
	graphics_context_set_fill_color(ctx, DIGIT_BACKGROUND_COLOR);

	r = layer_get_bounds(digitsLayer);
	graphics_fill_rect(ctx, GRect(0, 0, r.size.w, r.size.h), 0, GCornerNone);

	graphics_context_set_fill_color(ctx, DIGIT_BORDER_COLOR);
	graphics_fill_rect(ctx, GRect(0, 0, r.size.w, DIGIT_BORDER_HEIGHT), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(0, r.size.h - DIGIT_BORDER_HEIGHT, r.size.w, DIGIT_BORDER_HEIGHT), 0, GCornerNone);
}

static void update_digit_slot(Layer *layer, GContext *ctx) {
	DigitSlot *slot = *(DigitSlot**)layer_get_data(layer);
	int col, row;
	const int texel_w = (TIME_DIGIT_W - 2 * TIME_DIGIT_BORDER) / TIME_DIGIT_COLS;
	const int texel_h = (TIME_DIGIT_H - 2 * TIME_DIGIT_BORDER) / TIME_DIGIT_ROWS;

	if (slot->curDigit < 0)
		return;

	graphics_context_set_fill_color(ctx, DIGIT_COLOR);

	for (row = 0; row < TIME_DIGIT_ROWS; row++) {
		char v = digits[slot->curDigit][row];
		for (col = 0; col < TIME_DIGIT_COLS; col++) {
			if (v & (1 << (TIME_DIGIT_COLS - col - 1))) {
				graphics_fill_rect(ctx,
					GRect(
						TIME_DIGIT_BORDER + col * texel_w,
						TIME_DIGIT_BORDER + row * texel_h,
						texel_w, texel_h
					),
					0, GCornerNone
				);
			}
		}
	}
}

static void display_value(unsigned short value, unsigned short layer_offset, bool leading_zero) {
	for (int col = 1; col >= 0; col--) {
		DigitSlot *slot = &digitSlots[layer_offset + col];
		slot->curDigit = value % 10;
		if ((slot->curDigit == 0) && (col == 0) && !leading_zero)
			slot->curDigit = -1;
		value /= 10;
	}
}

static unsigned short handle_12_24(unsigned short hour) {
	if (clock_is_24h_style()) {
		return hour;
	}

	hour %= 12;
	return (hour != 0) ? hour : 12;
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	display_value(handle_12_24(tick_time->tm_hour), 0, true);
	display_value(tick_time->tm_min, 2, true);
	layer_mark_dirty(digitsLayer);
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	unsigned i;
	struct tm *tick_time;
	DigitSlot *slot;

	window_set_background_color(window, BACKGROUND_COLOR);

	digitsLayer = layer_create(
		GRect(
			0, bounds.size.h - TIME_BOTTOM_OFFSET - TIME_DIGIT_H - DIGIT_BORDER_HEIGHT * 2,
			bounds.size.w, TIME_DIGIT_H + DIGIT_BORDER_HEIGHT * 2
		)
	);
	layer_set_update_proc(digitsLayer, update_digits_layer);
	layer_add_child(window_get_root_layer(window), digitsLayer);

	for (i = 0; i < sizeof(digitSlots) / sizeof(digitSlots[0]); i++) {
		slot = &digitSlots[i];

		slot->curDigit = 0;
		slot->layer = layer_create_with_data(
				GRect(i * TIME_DIGIT_W, DIGIT_BORDER_HEIGHT,
					  TIME_DIGIT_W, TIME_DIGIT_H),
				sizeof(slot)
			);

		*(DigitSlot **)layer_get_data(slot->layer) = slot;
		layer_set_update_proc(slot->layer, update_digit_slot);

		layer_add_child(digitsLayer, slot->layer);
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

	for (i = 0; i < sizeof(digitSlots) / sizeof(digitSlots[0]); i++) {
		layer_remove_from_parent(digitSlots[i].layer);
		layer_destroy(digitSlots[i].layer);
	}
	layer_destroy(digitsLayer);
}

static void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	const bool animated = true;
	window_stack_push(window, animated);
	display_value(10, 0, false);

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
