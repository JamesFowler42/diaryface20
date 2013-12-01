#include "pebble.h"
#include "common.h"

Window *window;

TextLayer *text_date_layer;
TextLayer *text_time_layer;

Layer *line_layer;

GBitmap *icon_battery;
GBitmap *icon_status_1;
GBitmap *icon_status_2;
GBitmap *icon_status_3;
GBitmap *icon_battery_charge;
GBitmap *icon_entry_0;
GBitmap *icon_entry_1;
GBitmap *icon_entry_2;
GBitmap *icon_entry_3;
GBitmap *icon_entry_4;
GBitmap *icon_entry_5;
GBitmap *icon_pebble;

Layer *battery_layer;
Layer *status_layer;
Layer *entry_layer;
Layer *pebble_layer;

TextLayer *text_event_title_layer;
TextLayer *text_event_start_date_layer;
TextLayer *text_event_location_layer;
InverterLayer inverse_layer;

#ifdef INVERSE
InverterLayer full_inverse_layer;
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

/*
 * Unload and return what we have taken
 */
void window_unload(Window *window) {
  gbitmap_destroy(icon_battery);
  gbitmap_destroy(icon_battery_charge);	
  gbitmap_destroy(icon_status_1);	
  gbitmap_destroy(icon_status_2);	
  gbitmap_destroy(icon_status_3);	
  gbitmap_destroy(icon_entry_0);
  gbitmap_destroy(icon_entry_1);
  gbitmap_destroy(icon_entry_2);
  gbitmap_destroy(icon_entry_3);
  gbitmap_destroy(icon_entry_4);
  gbitmap_destroy(icon_entry_5);
  gbitmap_destroy(icon_pebble);
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
     graphics_draw_bitmap_in_rect(ctx, &icon_status_1.bmp, GRect(0, 0, 38, 12));
  } else if (g_status_display == STATUS_REPLY) {
     graphics_draw_bitmap_in_rect(ctx, &icon_status_2.bmp, GRect(0, 0, 38, 12));
  } else if (g_status_display == STATUS_ALERT_SET) {
     graphics_draw_bitmap_in_rect(ctx, &icon_status_3.bmp, GRect(0, 0, 38, 12));
  }
}

void set_status(int new_status_display) {
	g_status_display = new_status_display;
	layer_mark_dirty(&status_layer);
}

/*
 * Battery icon callback handler
 */
void battery_layer_update_callback(Layer *layer, GContext *ctx) {
  
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);

  if (g_static_state == 1 && g_static_level > 0 && g_static_level <= 100) {
    graphics_draw_bitmap_in_rect(ctx, &icon_battery.bmp, GRect(0, 0, 24, 12));
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(7, 4, (uint8_t)((g_static_level / 100.0) * 11.0), 4), 0, GCornerNone);
  } else if (g_static_state == 2 || g_static_state == 3) {
    graphics_draw_bitmap_in_rect(ctx, &icon_battery_charge.bmp, GRect(0, 0, 24, 12));
  }
}

/*
 * Set battery display
 */
void set_battery(uint8_t state, int8_t level) {
	g_static_state = state;
    g_static_level = level;	
	layer_mark_dirty(&battery_layer);
}

/*
 * Calendar entry callback handler
 */
void entry_layer_update_callback(Layer *layer, GContext *ctx) {
  	graphics_context_set_compositing_mode(ctx, GCompOpAssign);
	
	if (g_static_entry_no == 0)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_0.bmp, GRect(0, 0, 60, 12));
	else if (g_static_entry_no == 1)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_1.bmp, GRect(0, 0, 60, 12));
	else if (g_static_entry_no == 2)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_2.bmp, GRect(0, 0, 60, 12));
	else if (g_static_entry_no == 3)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_3.bmp, GRect(0, 0, 60, 12));
	else if (g_static_entry_no == 4)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_4.bmp, GRect(0, 0, 60, 12));
	else if (g_static_entry_no == 5)
    	graphics_draw_bitmap_in_rect(ctx, &icon_entry_5.bmp, GRect(0, 0, 60, 12));
}

/*
 * Display update in the event area
 */
void set_event_display(char *event_title, char *event_start_date, char *location, int num) {
	strncpy(g_event_title_static, event_title, BASIC_SIZE);
	strncpy(g_event_start_date_static, event_start_date, BASIC_SIZE);
	strncpy(g_location_static, location, BASIC_SIZE);
    text_layer_set_text(&text_event_title_layer, g_event_title_static);
    text_layer_set_text(&text_event_start_date_layer, g_event_start_date_static);
    text_layer_set_text(&text_event_location_layer, g_location_static);
    g_static_entry_no = num;
    layer_mark_dirty(&entry_layer);
}

/*
 * Pebble word callback handler
 */
void pebble_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  graphics_draw_bitmap_in_rect(ctx, &icon_pebble.bmp, GRect(0, 0, 27, 8));
}



/*
 * Display initialisation and further setup 
 */
void window_load(Window *window) {

  // Resources
  icon_battery = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_ICON);
  icon_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_CHARGE_ICON);	
  icon_status_1 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATUS_1);
  icon_status_2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATUS_2);
  icon_status_3 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATUS_3);
  icon_entry_0 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_0);
  icon_entry_1 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_1);
  icon_entry_2 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_2);
  icon_entry_3 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_3);
  icon_entry_4 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_4);
  icon_entry_5 = gbitmap_create_with_resource(RESOURCE_ID_ENTRY_5);
  icon_pebble = gbitmap_create_with_resource(RESOURCE_ID_PEBBLE);

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
  text_event_title_layer = text_layer_create(GRect(1, 18, window.layer.bounds.size.w - 1, 21));
  text_layer_set_text_color(text_event_title_layer, GColorWhite);
  text_layer_set_background_color(text_event_title_layer, GColorClear);
  text_layer_set_font(text_event_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_title_layer));

  // Date 
  text_event_start_date_layer = text_layer_create(GRect(1, 36, window.layer.bounds.size.w - 1, 21));
  text_layer_set_text_color(text_event_start_date_layer, GColorWhite);
  text_layer_set_background_color(text_event_start_date_layer, GColorClear);
  text_layer_set_font(text_event_start_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_start_date_layer));

  // Location
  text_event_location_layer = text_layer_init(GRect(1, 54, window.layer.bounds.size.w - 1, 21));
  text_layer_set_text_color(text_event_location_layer, GColorWhite);
  text_layer_set_background_color(text_event_location_layer, GColorClear);
  text_layer_set_font(text_event_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_event_location_layer));

  // Battery
  GRect frame;
  frame.origin.x = 120;
  frame.origin.y = 5;
  frame.size.w = 24;
  frame.size.h = 12;

  battery_layer = layer_create(frame);
  layer_set_update_proc(battery_layer, battery_layer_update_callback);
  layer_add_child(window_get_root_layer(window), battery_layer);

  // Status 	
  GRect frameb;
  frameb.origin.x = 80;
  frameb.origin.y = 6;
  frameb.size.w = 38;
  frameb.size.h = 12;

  status_layer = layer_create(frameb);
  layer_set_update_proc(status_layer, status_layer_update_callback);
  layer_add_child(window_get_root_layer(window), status_layer);

  // Calendar entry indicator	
  GRect framec;
  framec.origin.x = 2;
  framec.origin.y = 6;
  framec.size.w = 60;
  framec.size.h = 12;

  entry_layer = layer_create(framec);
  layer_set_update_proc(entry_layer, entry_layer_update_callback);
  layer_add_child(window_get_root_layer(window), entry_layer);
	
  // Pebble word
  GRect framed;
  framed.origin.x = 58;
  framed.origin.y = 83;
  framed.size.w = 27;
  framed.size.h = 8;

  pebble_layer = layer_create(framed);
  layer_set_update_proc(pebble_layer, pebble_layer_update_callback);
  layer_add_child(window_get_root_layer(window), pebble_layer);

  // Layer to invert when showing event
  inverse_layer = inverter_layer_create(GRect(0, 0, window.layer.bounds.size.w, 78));
  layer_set_hidden(inverter_layer_get_layer(inverse_layer), true);
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverse_layer));
	
  // Put everything in an initial state
  set_battery(0,-1); 
  set_status(STATUS_REQUEST);
  set_event_display("", "", "", 0);
	
  // Make sure the timers start but don't all go off together
  calendar_init(ctx);
	
  // Configurable inverse
  #ifdef INVERSE
  full_inverse_layer = inverter_layer_create(GRect(0, 0, window.layer.bounds.size.w, window.layer.bounds.size.h));
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(full_inverse_layer));
  #endif
}

/*
 * Highlight the event
 */
void set_invert_when_showing_event(bool invert) {
  layer_set_hidden(inverter_layer_get_layer(inverse_layer), !invert);
}

/*
 * Clock tick
 */
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {

    // Only update the date when it's changed.
	if (t->tick_time->tm_mday != g_last_tm_mday_date) {
		g_last_tm_mday_date = t->tick_time->tm_mday;
        string_format_time(g_date_text, BASIC_SIZE, "%a, %b %e", t->tick_time);
        text_layer_set_text(&text_date_layer, g_date_text);
	}

    string_format_time(g_time_text, BASIC_SIZE, "%R", t->tick_time);

    text_layer_set_text(&text_time_layer, g_time_text);
	
	sync_timed_event(t->tick_time->tm_min, ctx);
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

  const bool animated = true;
  window_stack_push(window, animated);

  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
}

int main(void) {
 init();
 app_event_loop();
 deinit();
}

/*
 * Main - or at least the pebble land main
 */
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .messaging_info = (PebbleAppMessagingInfo){
      .buffer_sizes = {
        .inbound = 124,
        .outbound = 256,
      },
      .default_callbacks.callbacks = {
        .in_received = received_message,
      },
    },
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    },
    .timer_handler = &handle_calendar_timer,
  };
  app_event_loop(params, &handlers);
}
