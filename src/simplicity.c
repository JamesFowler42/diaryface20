#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "common.h"

#define MY_UUID { 0x69, 0x8B, 0x3E, 0x04, 0xB1, 0x2E, 0x4F, 0xF5, 0xBF, 0xAD, 0x1B, 0xE6, 0xBD, 0xFE, 0xB4, 0xD7 }
PBL_APP_INFO(MY_UUID, "Diary Face", "Max Baeumle/Fowler", 1, 2 /* App version */, DEFAULT_MENU_ICON, APP_INFO_WATCH_FACE);

AppContextRef app_context;

Window window;

TextLayer text_date_layer;
TextLayer text_time_layer;

Layer line_layer;

HeapBitmap icon_battery;
HeapBitmap icon_status_1;
HeapBitmap icon_status_2;
HeapBitmap icon_status_3;
HeapBitmap icon_battery_charge;
HeapBitmap icon_entry_0;
HeapBitmap icon_entry_1;
HeapBitmap icon_entry_2;
HeapBitmap icon_entry_3;
HeapBitmap icon_entry_4;
HeapBitmap icon_entry_5;
HeapBitmap icon_pebble;

Layer battery_layer;
Layer status_layer;
Layer entry_layer;
Layer pebble_layer;

TextLayer text_event_title_layer;
TextLayer text_event_start_date_layer;
TextLayer text_event_location_layer;
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
  heap_bitmap_deinit(&icon_battery);
  heap_bitmap_deinit(&icon_battery_charge);	
  heap_bitmap_deinit(&icon_status_1);	
  heap_bitmap_deinit(&icon_status_2);	
  heap_bitmap_deinit(&icon_status_3);	
  heap_bitmap_deinit(&icon_entry_0);
  heap_bitmap_deinit(&icon_entry_1);
  heap_bitmap_deinit(&icon_entry_2);
  heap_bitmap_deinit(&icon_entry_3);
  heap_bitmap_deinit(&icon_entry_4);
  heap_bitmap_deinit(&icon_entry_5);
  heap_bitmap_deinit(&icon_pebble);
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
void handle_init(AppContextRef ctx) {
  app_context = ctx;

  // Window
  window_init(&window, "Window");
  window_set_window_handlers(&window, (WindowHandlers){
    .unload = window_unload,
  });
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  // Resources
  resource_init_current_app(&APP_RESOURCES);
  heap_bitmap_init(&icon_battery, RESOURCE_ID_BATTERY_ICON);
  heap_bitmap_init(&icon_battery_charge, RESOURCE_ID_BATTERY_CHARGE_ICON);	
  heap_bitmap_init(&icon_status_1, RESOURCE_ID_IMAGE_STATUS_1);
  heap_bitmap_init(&icon_status_2, RESOURCE_ID_IMAGE_STATUS_2);
  heap_bitmap_init(&icon_status_3, RESOURCE_ID_IMAGE_STATUS_3);
  heap_bitmap_init(&icon_entry_0, RESOURCE_ID_ENTRY_0);
  heap_bitmap_init(&icon_entry_1, RESOURCE_ID_ENTRY_1);
  heap_bitmap_init(&icon_entry_2, RESOURCE_ID_ENTRY_2);
  heap_bitmap_init(&icon_entry_3, RESOURCE_ID_ENTRY_3);
  heap_bitmap_init(&icon_entry_4, RESOURCE_ID_ENTRY_4);
  heap_bitmap_init(&icon_entry_5, RESOURCE_ID_ENTRY_5);
  heap_bitmap_init(&icon_pebble, RESOURCE_ID_PEBBLE);

  // Date
  text_layer_init(&text_date_layer, window.layer.frame);
  text_layer_set_text_color(&text_date_layer, GColorWhite);
  text_layer_set_background_color(&text_date_layer, GColorClear);
  layer_set_frame(&text_date_layer.layer, GRect(0, 94, 143, 168-94));
  text_layer_set_font(&text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  text_layer_set_text_alignment	(&text_date_layer,GTextAlignmentCenter);
  layer_add_child(&window.layer, &text_date_layer.layer);

  // Time
  text_layer_init(&text_time_layer, window.layer.frame);
  text_layer_set_text_color(&text_time_layer, GColorWhite);
  text_layer_set_background_color(&text_time_layer, GColorClear);
  layer_set_frame(&text_time_layer.layer, GRect(0, 112, 143, 168-112));
  text_layer_set_font(&text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  text_layer_set_text_alignment	(&text_time_layer,GTextAlignmentCenter);
  layer_add_child(&window.layer, &text_time_layer.layer);

  // Line
  layer_init(&line_layer, window.layer.frame);
  line_layer.update_proc = &line_layer_update_callback;
  layer_add_child(&window.layer, &line_layer);
 
  // Event title
  text_layer_init(&text_event_title_layer, GRect(1, 18, window.layer.bounds.size.w - 1, 21));
  text_layer_set_text_color(&text_event_title_layer, GColorWhite);
  text_layer_set_background_color(&text_event_title_layer, GColorClear);
  text_layer_set_font(&text_event_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(&window.layer, &text_event_title_layer.layer);

  // Date 
  text_layer_init(&text_event_start_date_layer, GRect(1, 36, window.layer.bounds.size.w - 1, 21));
  text_layer_set_text_color(&text_event_start_date_layer, GColorWhite);
  text_layer_set_background_color(&text_event_start_date_layer, GColorClear);
  text_layer_set_font(&text_event_start_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(&window.layer, &text_event_start_date_layer.layer);

  // Location
  text_layer_init(&text_event_location_layer, GRect(1, 54, window.layer.bounds.size.w - 1, 21));
  text_layer_set_text_color(&text_event_location_layer, GColorWhite);
  text_layer_set_background_color(&text_event_location_layer, GColorClear);
  text_layer_set_font(&text_event_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(&window.layer, &text_event_location_layer.layer);

  // Battery
  GRect frame;
  frame.origin.x = 120;
  frame.origin.y = 5;
  frame.size.w = 24;
  frame.size.h = 12;

  layer_init(&battery_layer, frame);
  battery_layer.update_proc = &battery_layer_update_callback;
  layer_add_child(&window.layer, &battery_layer);

  // Status 	
  GRect frameb;
  frameb.origin.x = 80;
  frameb.origin.y = 6;
  frameb.size.w = 38;
  frameb.size.h = 12;

  layer_init(&status_layer, frameb);
  status_layer.update_proc = &status_layer_update_callback;
  layer_add_child(&window.layer, &status_layer);

  // Calendar entry indicator	
  GRect framec;
  framec.origin.x = 2;
  framec.origin.y = 6;
  framec.size.w = 60;
  framec.size.h = 12;

  layer_init(&entry_layer, framec);
  entry_layer.update_proc = &entry_layer_update_callback;
  layer_add_child(&window.layer, &entry_layer);
	
  // Pebble word
  GRect framed;
  framed.origin.x = 58;
  framed.origin.y = 83;
  framed.size.w = 27;
  framed.size.h = 8;

  layer_init(&pebble_layer, framed);
  pebble_layer.update_proc = &pebble_layer_update_callback;
  layer_add_child(&window.layer, &pebble_layer);

  // Layer to invert when showing event
  inverter_layer_init(&inverse_layer, GRect(0, 0, window.layer.bounds.size.w, 78));
  layer_set_hidden(&inverse_layer.layer, true);
  layer_add_child(&window.layer, &inverse_layer.layer);
	
  // Put everything in an initial state
  set_battery(0,-1); 
  set_status(STATUS_REQUEST);
  set_event_display("", "", "", 0);
	
  // Make sure the timers start but don't all go off together
  calendar_init(ctx);
	
  // Configurable inverse
  #ifdef INVERSE
  inverter_layer_init(&full_inverse_layer, GRect(0, 0, window.layer.bounds.size.w, window.layer.bounds.size.h));
  layer_add_child(&window.layer, &full_inverse_layer.layer);
  #endif
}

/*
 * Highlight the event
 */
void set_invert_when_showing_event(bool invert) {
  layer_set_hidden(&inverse_layer.layer, !invert);
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
