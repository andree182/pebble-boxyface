#include "watchface.h"

static Window *window;
static Layer *digitsLayer, *calendarLayer;
static DigitSlot digitSlots[4], calendarSlots[2];
static TextLayer *calendarYMLayer, *calendarWDayLayer;
static int align = -1;
static int hourLeadingZero = true;
static int isTimeAmPm;
#if defined(PBL_BW)
static GBitmap *grayTexture;

static void update_root_layer(Layer *layer, GContext *ctx) {
	GRect r;

	r = layer_get_bounds(layer);
	graphics_draw_bitmap_in_rect(ctx, grayTexture, r);
}
#endif

static void update_calendar_layer(Layer *layer, GContext *ctx) {
	GRect r;
	graphics_context_set_fill_color(ctx, DIGIT_BACKGROUND_COLOR);

	r = layer_get_bounds(layer);
	graphics_fill_rect(ctx, GRect(0, 0, r.size.w, r.size.h), 0, GCornerNone);

	graphics_context_set_fill_color(ctx, DIGIT_BORDER_COLOR);
	graphics_fill_rect(ctx, GRect(0, 0, r.size.w, WIDGET_BORDER), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(0, r.size.h - WIDGET_BORDER, r.size.w, WIDGET_BORDER), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(0, 0, WIDGET_BORDER, r.size.h), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(r.size.w - WIDGET_BORDER, 0, r.size.w, r.size.h), 0, GCornerNone);
}

static void update_digits_layer(Layer *layer, GContext *ctx) {
	GRect r;
	r = layer_get_bounds(layer);

	graphics_context_set_fill_color(ctx, DIGIT_BACKGROUND_COLOR);
	graphics_fill_rect(ctx, GRect(0, 0, r.size.w, r.size.h), 0, GCornerNone);

	graphics_context_set_fill_color(ctx, DIGIT_BORDER_COLOR);
	graphics_fill_rect(ctx, GRect(0, 0, r.size.w, WIDGET_BORDER), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(0, r.size.h - WIDGET_BORDER, r.size.w, WIDGET_BORDER), 0, GCornerNone);

	if (isTimeAmPm == 0) {
		graphics_fill_rect(ctx, GRect(0, 2 * WIDGET_BORDER, WIDGET_BORDER, 3 * WIDGET_BORDER), 0, GCornerNone);
	} else if (isTimeAmPm == 1) {
		graphics_fill_rect(ctx, GRect(0, 4 * WIDGET_BORDER, WIDGET_BORDER, 3 * WIDGET_BORDER), 0, GCornerNone);
	}
}

static void update_digit_slot(Layer *layer, GContext *ctx) {
	DigitSlot *slot = *(DigitSlot**)layer_get_data(layer);
	int col, row;
	const int texel_w = (TIME_DIGIT_W - 2 * WIDGET_BORDER) / TIME_DIGIT_COLS;
	const int texel_h = (TIME_DIGIT_H - 2 * WIDGET_BORDER) / TIME_DIGIT_ROWS;

	if (slot->curDigit < 0)
		return;

	graphics_context_set_fill_color(ctx, DIGIT_COLOR);

	for (row = 0; row < TIME_DIGIT_ROWS; row++) {
		char v = digits[slot->curDigit][row];
		for (col = 0; col < TIME_DIGIT_COLS; col++) {
			if (v & (1 << (TIME_DIGIT_COLS - col - 1))) {
				graphics_fill_rect(ctx,
					GRect(
						WIDGET_BORDER + col * texel_w,
						WIDGET_BORDER + row * texel_h,
						texel_w, texel_h
					),
					0, GCornerNone
				);
			}
		}
	}
}

static void display_value(DigitSlot *slots, unsigned short value, unsigned short layer_offset, bool leadingZero) {
	for (int col = 1; col >= 0; col--) {
		DigitSlot *slot = &slots[layer_offset + col];
		slot->curDigit = value % 10;
		if ((slot->curDigit == 0) && (col == 0) && !leadingZero)
			slot->curDigit = -1;
		value /= 10;
	}
}

static unsigned short handle_12_24(unsigned short hour, int *ampm, int *leadingZero) {
	if (clock_is_24h_style()) {
		*leadingZero = hourLeadingZero;
		*ampm = -1;
		return hour;
	} else {
		*leadingZero = 0;
		*ampm = (hour < 12) ? 0 : 1;
	}

	hour %= 12;
	return (hour != 0) ? hour : 12;
}

static void tick_handler(struct tm *tickTime, TimeUnits unitsChanged)
{
	static char ym[16];
	const char *months[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	const char *dows[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	int leadingZero;
	int hour;

	hour = handle_12_24(tickTime->tm_hour, &isTimeAmPm, &leadingZero);

	display_value(digitSlots, hour, 0, leadingZero);
	display_value(digitSlots, tickTime->tm_min, 2, true);
	display_value(calendarSlots, tickTime->tm_mday, 0, hourLeadingZero);

	snprintf(ym, sizeof(ym), "%04d %s", tickTime->tm_year + 1900, months[tickTime->tm_mon]);
	text_layer_set_text(calendarYMLayer, ym);
	text_layer_set_text(calendarWDayLayer, dows[tickTime->tm_wday]);

	layer_mark_dirty(digitsLayer);
	layer_mark_dirty(calendarLayer);
}

static void window_load(Window *window) {
	Layer *windowLayer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(windowLayer);
	unsigned i;
	struct tm *tickTime;
	DigitSlot *slot;
	int digitsLayerPos;
	int calendarLayerVPos;
	int calendarLayerHPos;

#if defined(PBL_COLOR)
	window_set_background_color(window, BACKGROUND_COLOR);
#elif defined(PBL_BW)
	grayTexture = gbitmap_create_with_resource(RESOURCE_ID_GRAY_BG);
	layer_set_update_proc(window_get_root_layer(window), update_root_layer);
#endif

	calendarLayerHPos = bounds.size.w / 2 - CALENDAR_WIDGET_W / 2;
	if (align == -1) {
		/* Clock on top */
		digitsLayerPos = BORDER_OFFSET;
		calendarLayerVPos = bounds.size.h - BORDER_OFFSET - CALENDAR_WIDGET_H;
	} else if (align == 0) {
		/* Clock in the middle */
		digitsLayerPos = bounds.size.h / 2 - TIME_WIDGET_H / 2;
		calendarLayerVPos = BORDER_OFFSET;
	} else {
		/* Clock on the bottom */
		digitsLayerPos = bounds.size.h - BORDER_OFFSET - TIME_WIDGET_H;
		calendarLayerVPos = BORDER_OFFSET;
	}

	/* Clock */
	digitsLayer = layer_create(
		GRect(0, digitsLayerPos, bounds.size.w, TIME_WIDGET_H)
	);
	layer_set_update_proc(digitsLayer, update_digits_layer);
	layer_add_child(window_get_root_layer(window), digitsLayer);

	for (i = 0; i < sizeof(digitSlots) / sizeof(digitSlots[0]); i++) {
		slot = &digitSlots[i];

		slot->curDigit = 0;
		slot->layer = layer_create_with_data(
				GRect(i * TIME_DIGIT_W, WIDGET_BORDER,
					  TIME_DIGIT_W, TIME_DIGIT_H),
				sizeof(slot)
			);

		*(DigitSlot **)layer_get_data(slot->layer) = slot;
		layer_set_update_proc(slot->layer, update_digit_slot);

		layer_add_child(digitsLayer, slot->layer);
	}

	/* Calendar */
	calendarLayer = layer_create(
		GRect(calendarLayerHPos, calendarLayerVPos, CALENDAR_WIDGET_W, CALENDAR_WIDGET_H)
	);
	layer_set_update_proc(calendarLayer, update_calendar_layer);
	layer_add_child(window_get_root_layer(window), calendarLayer);
	for (i = 0; i < sizeof(calendarSlots) / sizeof(calendarSlots[0]); i++) {
		slot = &calendarSlots[i];

		slot->curDigit = 0;
		slot->layer = layer_create_with_data(
				GRect(
					WIDGET_BORDER + i * TIME_DIGIT_W, CALENDAR_WIDGET_H / 2 - TIME_DIGIT_H / 2,
					TIME_DIGIT_W, TIME_DIGIT_H
				), sizeof(slot)
			);

		*(DigitSlot **)layer_get_data(slot->layer) = slot;
		layer_set_update_proc(slot->layer, update_digit_slot);

		layer_add_child(calendarLayer, slot->layer);
	}
	calendarYMLayer = text_layer_create(
		GRect(WIDGET_BORDER, WIDGET_BORDER, CALENDAR_W, CALENDAR_TEXT_H)
	);
	text_layer_set_font(calendarYMLayer, fonts_get_system_font(CALENDAR_TEXT_FONT));
	text_layer_set_text_alignment(calendarYMLayer, GTextAlignmentCenter);
	layer_add_child(calendarLayer, text_layer_get_layer(calendarYMLayer));
	calendarWDayLayer = text_layer_create(
		GRect(
			WIDGET_BORDER, CALENDAR_WIDGET_H / 2 + TIME_DIGIT_H / 2 - 4, // HACK: the text would ba too low otherwise
			CALENDAR_W, CALENDAR_TEXT_H
		)
	);
	text_layer_set_font(calendarWDayLayer, fonts_get_system_font(CALENDAR_TEXT_FONT));
	text_layer_set_text_alignment(calendarWDayLayer, GTextAlignmentCenter);
	layer_add_child(calendarLayer, text_layer_get_layer(calendarWDayLayer));

	// initial values
	time_t temp;
	temp = time(NULL);
	tickTime = localtime(&temp);
	tick_handler(tickTime, MINUTE_UNIT);
}

static void window_unload(Window *window) {
	unsigned i;

	tick_timer_service_unsubscribe();

	for (i = 0; i < sizeof(digitSlots) / sizeof(digitSlots[0]); i++) {
		layer_remove_from_parent(digitSlots[i].layer);
		layer_destroy(digitSlots[i].layer);
	}
	layer_destroy(digitsLayer);

	for (i = 0; i < sizeof(calendarSlots) / sizeof(calendarSlots[0]); i++) {
		layer_remove_from_parent(calendarSlots[i].layer);
		layer_destroy(calendarSlots[i].layer);
	}
	layer_remove_from_parent(text_layer_get_layer(calendarYMLayer));
	text_layer_destroy(calendarYMLayer);
	layer_remove_from_parent(text_layer_get_layer(calendarWDayLayer));
	text_layer_destroy(calendarWDayLayer);
	layer_destroy(calendarLayer);

#if defined(PBL_BW)
	gbitmap_destroy(grayTexture);
#endif
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
