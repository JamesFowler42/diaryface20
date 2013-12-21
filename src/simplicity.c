/*
 * Diary Face
 *
 * Copyright (c) 2013 James Fowler/Max Baeumle
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "pebble.h"
#include "common.h"

Window *window;

TextLayer *text_date_layer;
TextLayer *text_time_layer;

Layer *line_layer;

GBitmap *icon_phone_battery;
GBitmap *icon_status_1;
GBitmap *icon_status_2;
GBitmap *icon_phone_battery_charge;
GBitmap *icon_entry_0;
GBitmap *icon_entry_1;
GBitmap *icon_entry_2;
GBitmap *icon_entry_3;
GBitmap *icon_entry_4;
GBitmap *icon_entry_5;
GBitmap *icon_pebble;
GBitmap *icon_watch_battery;
GBitmap *icon_watch_battery_charge;

Layer *i_battery_layer;
Layer *p_battery_layer;
Layer *status_layer;
Layer *entry_layer;
Layer *pebble_layer;

TextLayer *text_event_title_layer;
TextLayer *text_event_start_date_layer;
TextLayer *text_event_location_layer;

#ifdef INVERSE
InverterLayer *full_inverse_layer;
#endif

int g_last_tm_mday_date = -1;
int g_status_display = 0;

// Variables used for text display areas - Pebble hates these to take off and move
char g_event_title_static[BASIC_SIZE];
char g_event_start_date_static[BASIC_SIZE];
char g_location_static[BASIC_SIZE];
char g_time_text[BASIC_SIZE];
char g_date_text[BASIC_SIZE];
uint8_t g_static_state = 0;
int8_t g_static_level = -1;
int g_static_entry_no = 0;
static uint8_t battery_level;
static bool battery_plugged;

/*
 * Unload and return what we have taken
 */
void window_unload(Window *window) {
  gbitmap_destroy(icon_phone_battery);
  gbitmap_destroy(icon_phone_battery_charge);
  gbitmap_destroy(icon_status_1);	
  gbitmap_destroy(icon_status_2);	
  gbitmap_destroy(icon_entry_0);
  gbitmap_destroy(icon_entry_1);
  gbitmap_destroy(icon_entry_2);
  gbitmap_destroy(icon_entry_3);
  gbitmap_destroy(icon_entry_4);
  gbitmap_destroy(icon_entry_5);
  gbitmap_destroy(icon_pebble);
  gbitmap_destroy(icon_watch_battery);
  gbitmap_destroy(icon_watch_battery_charge);
}

/*
 * Centre line callback handler
 */
void line_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);

  graphics_draw_line(ctx, GPoint(0, 87), GPoint(54, 87));
  graphics_draw_line(ctx, GPoint(0, 88), GPoint(54, 88));
  graphics_draw_line(ctx, GPoint(88, 87), GPoint(143, 87));
  graphics_draw_line(ctx, GPoint(88, 88), GPoint(143, 88));
}

/*
 * Status icon callback handler
 */
void status_layer_update_callback(Layer *layer, GContext *ctx) {
  
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
	
  if (g_status_display == STATUS_REQUEST) {
     graphics_draw_bitmap_in_rect(ctx, icon_status_1, GRect(0, 0, 15, 15));
  } else if (g_status_display == STATUS_REPLY) {
     graphics_draw_bitmap_in_rect(ctx, icon_status_2, GRect(0, 0, 15, 15));
  }
}

void set_status(int new_status_display) {
	g_status_display = new_status_display;
	layer_mark_dirty(status_layer);
}

/*
 * iPhone Battery icon callback handler
 */
void i_battery_layer_update_callback(Layer *layer, GContext *ctx) {
  
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);

  if (g_static_state == 1 && g_static_level > 0 && g_static_level <= 100) {
    graphics_draw_bitmap_in_rect(ctx, icon_phone_battery, GRect(0, 0, 30, 15));
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(14, 5, (uint8_t)((g_static_level / 100.0) * 11.0), 4), 0, GCornerNone);
  } else if (g_static_state == 2 || g_static_state == 3) {
    graphics_draw_bitmap_in_rect(ctx, icon_phone_battery_charge, GRect(0, 0, 30, 15));
  }
}

/*
 * Pebble Battery icon callback handler
 */
static void p_battery_layer_update_callback(Layer *layer, GContext *ctx) {

	graphics_context_set_compositing_mode(ctx, GCompOpAssign);
	if (!battery_plugged) {
		graphics_draw_bitmap_in_rect(ctx, icon_watch_battery, GRect(0, 0, 35, 15));
		graphics_context_set_stroke_color(ctx, GColorWhite);
		graphics_context_set_fill_color(ctx, GColorBlack);
		graphics_fill_rect(ctx, GRect(16, 5, (uint8_t)((battery_level / 100.0) * 11.0), 4), 0, GCornerNone);
	} else {
		graphics_draw_bitmap_in_rect(ctx, icon_watch_battery_charge, GRect(0, 0, 35, 15));
	}
}

/*
 * Battery state change
 */
static void battery_state_handler(BatteryChargeState charge) {
	battery_level = charge.charge_percent;
	battery_plugged = charge.is_plugged;
	layer_mark_dirty(p_battery_layer);
}

/*
 * Set battery display
 */
void set_battery(uint8_t state, int8_t level) {
	g_static_state = state;
    g_static_level = level;	
	layer_mark_dirty(i_battery_layer);
}

/*
 * Calendar entry callback handler
 */
void entry_layer_update_callback(Layer *layer, GContext *ctx) {
  	graphics_context_set_compositing_mode(ctx, GCompOpAssign);
	
	if (g_static_entry_no == 0)
    	graphics_draw_bitmap_in_rect(ctx, icon_entry_0, GRect(0, 0, 64, 15));
	else if (g_static_entry_no == 1)
    	graphics_draw_bitmap_in_rect(ctx, icon_entry_1, GRect(0, 0, 64, 15));
	else if (g_static_entry_no == 2)
    	graphics_draw_bitmap_in_rect(ctx, icon_entry_2, GRect(0, 0, 64, 15));
	else if (g_static_entry_no == 3)
    	graphics_draw_bitmap_in_rect(ctx, icon_entry_3, GRect(0, 0, 64, 15));
	else if (g_static_entry_no == 4)
    	graphics_draw_bitmap_in_rect(ctx, icon_entry_4, GRect(0, 0, 64, 15));
	else if (g_static_entry_no == 5)
    	graphics_draw_bitmap_in_rect(ctx, icon_entry_5, GRect(0, 0, 64, 15));
}

/*
 * Display update in the event area
 */
void set_event_display(char *event_title, char *event_start_date, char *location, int num) {
	strncpy(g_event_title_static, event_title, BASIC_SIZE);
	strncpy(g_event_start_date_static, event_start_date, BASIC_SIZE);
	strncpy(g_location_static, location, BASIC_SIZE);
    text_layer_set_text(text_event_title_layer, g_event_title_static);
    text_layer_set_text(text_event_start_date_layer, g_event_start_date_static);
    text_layer_set_text(text_event_location_layer, g_location_static);
    g_static_entry_no = num;
    layer_mark_dirty(entry_layer);
}

/*
 * Pebble word callback handler
 */
void pebble_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  graphics_draw_bitmap_in_rect(ctx, icon_pebble, GRect(0, 0, 27, 8));
}



/*
 * Display initialisation and further setup 
 */
void window_load(Window *window) {

  // Resources
  icon_phone_battery = gbitmap_create_with_resource(RESOURCE_ID_PHONE_BATTERY_ICON);
  icon_phone_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_PHONE_BATTERY_CHARGE_ICON);
  icon_status_1 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATUS_1);
  icon_status_2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATUS_2);
  icon_entry_0 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_0);
  icon_entry_1 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_1);
  icon_entry_2 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_2);
  icon_entry_3 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_3);
  icon_entry_4 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_4);
  icon_entry_5 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_5);
  icon_pebble = gbitmap_create_with_resource(RESOURCE_ID_PEBBLE);
  icon_watch_battery = gbitmap_create_with_resource(RESOURCE_ID_WATCH_BATTERY_ICON);
  icon_watch_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_WATCH_BATTERY_CHARGE_ICON);

  // Date
  text_date_layer =	text_layer_create(GRect(0, 94, 143, 168-94));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_date_layer));

  // Time 
  text_time_layer = text_layer_create(GRect(0, 112, 143, 168-112));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  text_layer_set_text_alignment	(text_time_layer,GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_time_layer));

  // Line
  line_layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_get_root_layer(window), line_layer);
 
  // Event title
  text_event_title_layer = text_layer_create(GRect(1, 16, 144 - 1, 21));
  text_layer_set_text_color(text_event_title_layer, GColorWhite);
  text_layer_set_background_color(text_event_title_layer, GColorClear);
  text_layer_set_font(text_event_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_title_layer));

  // Date 
  text_event_start_date_layer = text_layer_create(GRect(1, 36, 144 - 1, 21));
  text_layer_set_text_color(text_event_start_date_layer, GColorWhite);
  text_layer_set_background_color(text_event_start_date_layer, GColorClear);
  text_layer_set_font(text_event_start_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_start_date_layer));

  // Location
  text_event_location_layer = text_layer_create(GRect(1, 54, 144 - 1, 21));
  text_layer_set_text_color(text_event_location_layer, GColorWhite);
  text_layer_set_background_color(text_event_location_layer, GColorClear);
  text_layer_set_font(text_event_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_location_layer));

  // iPhone Battery
  g_static_state = 1;
  g_static_level = 50;
  i_battery_layer = layer_create(GRect(79,0,30,15));
  layer_set_update_proc(i_battery_layer, i_battery_layer_update_callback);
  layer_add_child(window_get_root_layer(window), i_battery_layer);
	
  // Pebble Battery
  BatteryChargeState initial = battery_state_service_peek();
  battery_level = initial.charge_percent;
  battery_plugged = initial.is_plugged;
  p_battery_layer = layer_create(GRect(109,0,35,15));
  layer_set_update_proc(p_battery_layer, p_battery_layer_update_callback);
  layer_add_child(window_get_root_layer(window), p_battery_layer);

  // Status 	
  status_layer = layer_create(GRect(64,0,15,15));
  layer_set_update_proc(status_layer, status_layer_update_callback);
  layer_add_child(window_get_root_layer(window), status_layer);

  // Calendar entry indicator	
  entry_layer = layer_create(GRect(0,0,64,15));
  layer_set_update_proc(entry_layer, entry_layer_update_callback);
  layer_add_child(window_get_root_layer(window), entry_layer);
	
  // Pebble word
  pebble_layer = layer_create(GRect(58,83,27,8));
  layer_set_update_proc(pebble_layer, pebble_layer_update_callback);
  layer_add_child(window_get_root_layer(window), pebble_layer);
	
  // Put everything in an initial state
  set_battery(1,50);
  set_status(STATUS_REQUEST);
  set_event_display("", "", "", 0);
	
  // Make sure the timers start but don't all go off together
  calendar_init();
	
  // Configurable inverse
  #ifdef INVERSE
  full_inverse_layer = inverter_layer_create(GRect(0, 0, 144, 168));
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(full_inverse_layer));
  #endif
}

/*
 * Clock tick
 */
void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {

	second_timer();

	if (tick_time->tm_sec == 0)
		return;

    // Only update the date when it's changed.
	if (tick_time->tm_mday != g_last_tm_mday_date) {
		g_last_tm_mday_date = tick_time->tm_mday;
        strftime(g_date_text, BASIC_SIZE, "%a, %b %e", tick_time);
        text_layer_set_text(text_date_layer, g_date_text);
	}

    strftime(g_time_text, BASIC_SIZE, "%R", tick_time);

    text_layer_set_text(text_time_layer, g_time_text);
	
	minute_timer(tick_time->tm_min);
}

static void init(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_fullscreen(window, true);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  const int inbound_size = app_message_inbox_size_maximum();
  const int outbound_size = app_message_outbox_size_maximum();

  app_message_open(inbound_size, outbound_size);

  window_stack_push(window, true);

  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);

  app_message_register_inbox_received(received_message);
	
  // Accelerometer
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  accel_data_service_subscribe(10, &accel_data_handler);
	
  battery_state_service_subscribe(battery_state_handler);

}

static void deinit() {
	tick_timer_service_unsubscribe();
	app_message_deregister_callbacks();
	accel_data_service_unsubscribe();
}

int main(void) {
 init();
 app_event_loop();
 deinit();
}

