/*
This file is part of Boxyface.
Copyright 2015 Andrej Krutak

Boxyface is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Boxyface is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with boxyface.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "watchface.h"

static Window *window;
static Layer *digitsLayer, *ampmLayer, *calendarLayers[2], *batteryLayer;
static DigitSlot digitSlots[4], calendarSlots[2];
static TextLayer *calendarYMLayer, *calendarWDayLayer;
static int align =
#if defined(PBL_ROUND)
	0;
#else
	-1;
#endif

static int hourLeadingZero = true;
static int isTimeAmPm;
static int showBatteryStatus = true;
static int ignore12h =
#if defined(PBL_ROUND)
	true;
#else
	false;
#endif

#if defined(PBL_BW)
static GBitmap *grayTexture;

static void update_root_layer(Layer *layer, GContext *ctx) {
	GRect r;

	r = layer_get_bounds(layer);
	graphics_draw_bitmap_in_rect(ctx, grayTexture, r);
}
#endif

static void update_bordered_layer(Layer *layer, GContext *ctx, int top, int bottom, int side) {
	GRect r = layer_get_bounds(layer);

	graphics_context_set_fill_color(ctx, DIGIT_BACKGROUND_COLOR);
	graphics_fill_rect(ctx, GRect(0, 0, r.size.w, r.size.h), 0, GCornerNone);

	graphics_context_set_fill_color(ctx, DIGIT_BORDER_COLOR);
	if (top)
		graphics_fill_rect(ctx, GRect(0, 0, r.size.w, WIDGET_BORDER), 0, GCornerNone);
	if (bottom)
		graphics_fill_rect(ctx, GRect(0, r.size.h - WIDGET_BORDER, r.size.w, WIDGET_BORDER), 0, GCornerNone);
	if (side) {
		graphics_fill_rect(ctx, GRect(0, 0, WIDGET_BORDER, r.size.h), 0, GCornerNone);
		graphics_fill_rect(ctx, GRect(r.size.w - WIDGET_BORDER, 0, r.size.w, r.size.h), 0, GCornerNone);
	}
}

static void update_sideborder_layer(Layer *layer, GContext *ctx) {
	update_bordered_layer(layer, ctx, 0, 0, 1);
}
static void update_bottomborder_layer(Layer *layer, GContext *ctx) {
	update_bordered_layer(layer, ctx, 0, 1, 0);
}
static void update_halfborder_layer(Layer *layer, GContext *ctx) {
	update_bordered_layer(layer, ctx, 1, 1, 0);
}

static void update_fullborder_layer(Layer *layer, GContext *ctx) {
	update_bordered_layer(layer, ctx, 1, 1, 1);
}

static void update_battery_layer(Layer *layer, GContext *ctx) {
	GRect r = layer_get_bounds(layer);
	BatteryChargeState charge_state = battery_state_service_peek();
	int i, count;
	int curPos;
	int step;

	if (!showBatteryStatus)
		return;

	graphics_context_set_fill_color(ctx, DIGIT_BORDER_COLOR);

	count = r.size.w * (100 - charge_state.charge_percent) / 100 / WIDGET_BORDER;
	step = (charge_state.is_charging ? 2 : 1);
	for (i = 0, curPos = r.size.w - WIDGET_BORDER;
		i < count;
		i += step, curPos -= step * WIDGET_BORDER
	) {
		graphics_fill_rect(ctx, GRect(curPos, 0, WIDGET_BORDER, r.size.w), 0, GCornerNone);
	}
}

static void update_ampm_layer(Layer *layer, GContext *ctx) {
	GRect r = layer_get_bounds(layer);

	graphics_context_set_fill_color(ctx, DIGIT_BORDER_COLOR);
	if (isTimeAmPm == 0) {
		graphics_fill_rect(ctx, GRect(0, 0, r.size.w, 3 * WIDGET_BORDER), 0, GCornerNone);
	} else if (isTimeAmPm == 1) {
		graphics_fill_rect(ctx, GRect(0, 2 * WIDGET_BORDER, r.size.w, 3 * WIDGET_BORDER), 0, GCornerNone);
	}
}

static void update_digit_slot(Layer *layer, GContext *ctx) {
	DigitSlot *slot = *(DigitSlot**)layer_get_data(layer);
	int col, row;
	const int texel_w = TIME_DIGIT_W / (TIME_DIGIT_COLS + 2);
	const int texel_h = TIME_DIGIT_H / (TIME_DIGIT_ROWS + 2);

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
	if (clock_is_24h_style() || ignore12h) {
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
	layer_mark_dirty(calendarLayers[0]);
	layer_mark_dirty(calendarLayers[1]);
}

static void battery_handler(BatteryChargeState charge_state)
{
	(void)charge_state;
	layer_mark_dirty(batteryLayer);
}

static void window_load(Window *window) {
	Layer *windowLayer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(windowLayer);
	unsigned i;
	struct tm *tickTime;
	DigitSlot *slot;
	int digitsLayerHPos, digitsLayerVPos;
	int calendarLayerHPos, calendarLayerVPos;
	int calendarDigitHPos, calendarDigitVPos;
	int batteryLayerPos;

#if defined(PBL_COLOR)
	window_set_background_color(window, BACKGROUND_COLOR);
#elif defined(PBL_BW)
	grayTexture = gbitmap_create_with_resource(RESOURCE_ID_GRAY_BG);
	layer_set_update_proc(window_get_root_layer(window), update_root_layer);
#endif

	digitsLayerHPos = bounds.size.w / 2 - TIME_WIDGET_W / 2;
	calendarLayerHPos = bounds.size.w / 2 - CALENDAR_WIDGET_W / 2;
	if (align == -1) {
		/* Clock on top */
		digitsLayerVPos = BORDER_OFFSET;
		calendarLayerVPos = bounds.size.h - BORDER_OFFSET - CALENDAR_WIDGET_H;
		batteryLayerPos = 0;
	} else if (align == 0) {
		/* Clock in the middle */
		digitsLayerVPos = bounds.size.h / 2 - TIME_WIDGET_H / 2;
		batteryLayerPos = digitsLayerVPos - BORDER_OFFSET;
	} else {
		/* Clock on the bottom */
		digitsLayerVPos = bounds.size.h - BORDER_OFFSET - TIME_WIDGET_H;
		calendarLayerVPos = BORDER_OFFSET;
		batteryLayerPos = bounds.size.h - BORDER_OFFSET;
	}

	/* Clock */
	digitsLayer = layer_create(
		GRect(0, digitsLayerVPos, bounds.size.w, TIME_WIDGET_H)
	);
	layer_set_update_proc(digitsLayer, update_halfborder_layer);
	layer_add_child(window_get_root_layer(window), digitsLayer);

	for (i = 0; i < sizeof(digitSlots) / sizeof(digitSlots[0]); i++) {
		slot = &digitSlots[i];

		slot->curDigit = 0;
		slot->layer = layer_create_with_data(
				GRect(digitsLayerHPos + i * TIME_DIGIT_W, WIDGET_BORDER,
					  TIME_DIGIT_W, TIME_DIGIT_H),
				sizeof(slot)
			);

		*(DigitSlot **)layer_get_data(slot->layer) = slot;
		layer_set_update_proc(slot->layer, update_digit_slot);

		layer_add_child(digitsLayer, slot->layer);
	}
	ampmLayer = layer_create(
		GRect(0, 2 * WIDGET_BORDER, WIDGET_BORDER, 5 * WIDGET_BORDER)
	);
	layer_set_update_proc(ampmLayer, update_ampm_layer);
	layer_add_child(digitsLayer, ampmLayer);

	batteryLayer = layer_create(
		GRect(0, batteryLayerPos, bounds.size.w, BORDER_OFFSET)
	);
	layer_set_update_proc(batteryLayer, update_battery_layer);
	layer_add_child(window_get_root_layer(window), batteryLayer);

	/* Calendar */
	if (align == 0) {
		calendarLayers[0] = layer_create(
			GRect(0, 0, bounds.size.w, CALENDAR_TEXT_H +
#if defined(PBL_ROUND)
				2 *
#endif
				WIDGET_BORDER
			)
		);
		layer_set_update_proc(calendarLayers[0], update_bottomborder_layer);
		layer_add_child(window_get_root_layer(window), calendarLayers[0]);

		calendarLayerHPos = bounds.size.h / 2 + TIME_WIDGET_H / 2;
		calendarLayers[1] = layer_create(
			GRect(bounds.size.w / 2 - CALENDAR_WIDGET_W / 2, calendarLayerHPos,
				  CALENDAR_WIDGET_W, bounds.size.h - calendarLayerHPos)
		);
		layer_set_update_proc(calendarLayers[1], update_sideborder_layer);
		layer_add_child(window_get_root_layer(window), calendarLayers[1]);

		calendarYMLayer = text_layer_create(
#if defined(PBL_ROUND)
			GRect(0, WIDGET_BORDER - 3, bounds.size.w, CALENDAR_TEXT_H)
#else
			GRect(0, -3, bounds.size.w, CALENDAR_TEXT_H)
#endif
		);
		calendarWDayLayer = text_layer_create(
			GRect(
				WIDGET_BORDER, TIME_DIGIT_H - TIME_DIGIT_TEXEL_SIZE,
				CALENDAR_W, CALENDAR_TEXT_H
			)
		);
		calendarDigitVPos = 0;
		calendarDigitHPos = WIDGET_BORDER;
	} else {
		calendarLayers[0] = layer_create(
			GRect(calendarLayerHPos, calendarLayerVPos, CALENDAR_WIDGET_W, CALENDAR_WIDGET_H)
		);
		layer_set_update_proc(calendarLayers[0], update_fullborder_layer);
		layer_add_child(window_get_root_layer(window), calendarLayers[0]);
		calendarLayers[1] = calendarLayers[0];

		calendarYMLayer = text_layer_create(
			GRect(WIDGET_BORDER, WIDGET_BORDER, CALENDAR_W, CALENDAR_TEXT_H)
		);
		calendarWDayLayer = text_layer_create(
			GRect(
				WIDGET_BORDER, CALENDAR_WIDGET_H / 2 + TIME_DIGIT_H / 2 - TIME_DIGIT_TEXEL_SIZE,
				CALENDAR_W, CALENDAR_TEXT_H
			)
		);

		calendarDigitVPos = CALENDAR_WIDGET_H / 2 - TIME_DIGIT_H / 2;
		calendarDigitHPos = WIDGET_BORDER;
	}
	for (i = 0; i < sizeof(calendarSlots) / sizeof(calendarSlots[0]); i++) {
		slot = &calendarSlots[i];

		slot->curDigit = 0;
		slot->layer = layer_create_with_data(
				GRect(
					calendarDigitHPos + i * TIME_DIGIT_W, calendarDigitVPos,
					TIME_DIGIT_W, TIME_DIGIT_H
				), sizeof(slot)
			);

		*(DigitSlot **)layer_get_data(slot->layer) = slot;
		layer_set_update_proc(slot->layer, update_digit_slot);

		layer_add_child(calendarLayers[1], slot->layer);
	}
	text_layer_set_font(calendarYMLayer, fonts_get_system_font(CALENDAR_TEXT_FONT));
	text_layer_set_text_alignment(calendarYMLayer, GTextAlignmentCenter);
	layer_add_child(calendarLayers[0], text_layer_get_layer(calendarYMLayer));
	text_layer_set_font(calendarWDayLayer, fonts_get_system_font(CALENDAR_TEXT_FONT));
	text_layer_set_text_alignment(calendarWDayLayer, GTextAlignmentCenter);
	layer_add_child(calendarLayers[1], text_layer_get_layer(calendarWDayLayer));

	// initial values
	time_t temp;
	temp = time(NULL);
	tickTime = localtime(&temp);
	tick_handler(tickTime, MINUTE_UNIT);
}

static void window_unload(Window *window) {
	unsigned i;

	battery_state_service_unsubscribe();
	tick_timer_service_unsubscribe();

	for (i = 0; i < sizeof(digitSlots) / sizeof(digitSlots[0]); i++) {
		layer_remove_from_parent(digitSlots[i].layer);
		layer_destroy(digitSlots[i].layer);
	}
	layer_remove_from_parent(ampmLayer);
	layer_destroy(ampmLayer);
	layer_destroy(digitsLayer);

	for (i = 0; i < sizeof(calendarSlots) / sizeof(calendarSlots[0]); i++) {
		layer_remove_from_parent(calendarSlots[i].layer);
		layer_destroy(calendarSlots[i].layer);
	}
	layer_remove_from_parent(text_layer_get_layer(calendarYMLayer));
	text_layer_destroy(calendarYMLayer);
	layer_remove_from_parent(text_layer_get_layer(calendarWDayLayer));
	text_layer_destroy(calendarWDayLayer);
	layer_destroy(calendarLayers[0]);
	if (calendarLayers[0] != calendarLayers[1])
		layer_destroy(calendarLayers[1]);
	layer_destroy(batteryLayer);

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
	battery_state_service_subscribe(battery_handler);
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
