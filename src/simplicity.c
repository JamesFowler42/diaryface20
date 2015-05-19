/*
 * Diary Face
 *
 * Copyright (c) 2013-2015 James Fowler/Max Baeumle
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

static Window *window;

static GBitmap *icon_phone_battery;
static GBitmap *icon_status_1;
static GBitmap *icon_status_2;
static GBitmap *icon_phone_battery_charge;
static GBitmap *icon_entry_0;
static GBitmap *icon_entry_1;
static GBitmap *icon_entry_2;
static GBitmap *icon_entry_3;
static GBitmap *icon_entry_4;
static GBitmap *icon_entry_5;
static GBitmap *icon_pebble;
static GBitmap *icon_watch_battery;
static GBitmap *icon_watch_battery_charge;

static Layer *line_layer;
static Layer *scroll_layer;
static Layer *i_battery_layer;
static Layer *p_battery_layer;
static Layer *status_layer;
static Layer *entry_layer;
static Layer *pebble_layer;
static Layer *bullet_layer;
static Layer *time_layer;

static TextLayer *text_date_layer;
static TextLayer *text_time_layer;
static TextLayer *text_time_layer2;
static TextLayer *text_event_title_layer;
static TextLayer *text_event_start_date_layer;
static TextLayer *text_event_location_layer;

static struct PropertyAnimation* prop_animation_out = NULL;
static struct PropertyAnimation* clock_animation_out = NULL;
static struct PropertyAnimation* prop_animation_in = NULL;
static struct PropertyAnimation* clock_animation_in = NULL;

#ifndef PBL_COLOR
  static InverterLayer *full_inverse_layer;
#endif

static int g_last_tm_mday_date = -1;
static int g_status_display = 0;

// Variables used for text display areas - Pebble hates these to take off and move
static char g_event_title_static[member_size(EVENT_TYPE, title)];
static char g_event_start_date_static[member_size(EventInternal, start_date)];
static char g_location_static[member_size(EVENT_TYPE, location)];
static char g_disp_event_title_static[member_size(EVENT_TYPE, title)];
static char g_disp_event_start_date_static[member_size(EventInternal, start_date)];
static char g_disp_location_static[member_size(EVENT_TYPE, location)];

static char g_time_text[6];
static char g_date_text[17];
static uint8_t g_static_state = 0;
static int8_t g_static_level = -1;
static int g_static_entry_no = 0;
static int g_disp_static_entry_no = 0;
static uint8_t battery_level;
static bool battery_plugged;
static bool first_tick = true;
static GColor g_disp_static_color;
static GColor g_static_color;

/*
 * Unload and return what we have taken
 */
static void window_unload(Window *window) {

  layer_destroy(line_layer);
  layer_destroy(scroll_layer);
  layer_destroy(i_battery_layer);
  layer_destroy(p_battery_layer);
  layer_destroy(status_layer);
  layer_destroy(entry_layer);
  layer_destroy(pebble_layer);
  #ifdef PBL_COLOR
    layer_destroy(bullet_layer);
  #endif
  layer_destroy(time_layer);

  #ifndef PBL_COLOR
    inverter_layer_destroy(full_inverse_layer);
  #endif

  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_time_layer);
  #ifdef PBL_COLOR
    text_layer_destroy(text_time_layer2);
  #endif
  text_layer_destroy(text_event_title_layer);
  text_layer_destroy(text_event_start_date_layer);
  text_layer_destroy(text_event_location_layer);

  if (clock_animation_in != NULL) {
    if (animation_is_scheduled((struct Animation *) clock_animation_in))
      animation_unschedule((struct Animation *) clock_animation_in);
    property_animation_destroy(clock_animation_in);
  }

  if (prop_animation_in != NULL) {
    if (animation_is_scheduled((struct Animation *) prop_animation_in))
      animation_unschedule((struct Animation *) prop_animation_in);
    property_animation_destroy(prop_animation_in);
  }

  if (clock_animation_out != NULL) {
    if (animation_is_scheduled((struct Animation *) clock_animation_out))
      animation_unschedule((struct Animation *) clock_animation_out);
    property_animation_destroy(clock_animation_out);
  }

  if (prop_animation_out != NULL) {
    if (animation_is_scheduled((struct Animation *) prop_animation_out))
      animation_unschedule((struct Animation *) prop_animation_out);
    property_animation_destroy(prop_animation_out);
  }

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
static void line_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, TOP_LINE);
  graphics_draw_line(ctx, GPoint(0, 87), GPoint(54, 87));
  graphics_draw_line(ctx, GPoint(88, 87), GPoint(143, 87));
    
  graphics_context_set_stroke_color(ctx, BOTTOM_LINE); 
  graphics_draw_line(ctx, GPoint(0, 88), GPoint(54, 88));
  graphics_draw_line(ctx, GPoint(88, 88), GPoint(143, 88));
}

/*
 * Status icon callback handler
 */
static void status_layer_update_callback(Layer *layer, GContext *ctx) {

  graphics_context_set_compositing_mode(ctx, GCompOpAssign);

  if (g_status_display == STATUS_REQUEST) {
    graphics_draw_bitmap_in_rect(ctx, icon_status_1, GRect(0, 0, 15, 15));
  } else if (g_status_display == STATUS_REPLY) {
    graphics_draw_bitmap_in_rect(ctx, icon_status_2, GRect(0, 0, 15, 15));
  }
}

/*
 * Set the status
 */
void set_status(int new_status_display) {
  g_status_display = new_status_display;
  layer_mark_dirty(status_layer);
}

/*
 * iPhone Battery icon callback handler
 */
static void i_battery_layer_update_callback(Layer *layer, GContext *ctx) {

  graphics_context_set_compositing_mode(ctx, GCompOpAssign);

  if (g_static_state == 1 && g_static_level > 0 && g_static_level <= 100) {
    graphics_draw_bitmap_in_rect(ctx, icon_phone_battery, GRect(0, 0, 30, 15));
    graphics_context_set_stroke_color(ctx, BATTERY_STROKE);
    graphics_context_set_fill_color(ctx, BATTERY_FILL);
    graphics_fill_rect(ctx, GRect(14, 5, g_static_level / 9, 4), 0, GCornerNone);
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
    graphics_context_set_stroke_color(ctx, BATTERY_STROKE);
    graphics_context_set_fill_color(ctx, BATTERY_FILL);
    graphics_fill_rect(ctx, GRect(16, 5, battery_level / 9, 4), 0, GCornerNone);
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
static void entry_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);

  if (g_disp_static_entry_no == 0)
    graphics_draw_bitmap_in_rect(ctx, icon_entry_0, GRect(0, 0, 64, 15));
  else if (g_disp_static_entry_no == 1)
    graphics_draw_bitmap_in_rect(ctx, icon_entry_1, GRect(0, 0, 64, 15));
  else if (g_disp_static_entry_no == 2)
    graphics_draw_bitmap_in_rect(ctx, icon_entry_2, GRect(0, 0, 64, 15));
  else if (g_disp_static_entry_no == 3)
    graphics_draw_bitmap_in_rect(ctx, icon_entry_3, GRect(0, 0, 64, 15));
  else if (g_disp_static_entry_no == 4)
    graphics_draw_bitmap_in_rect(ctx, icon_entry_4, GRect(0, 0, 64, 15));
  else if (g_disp_static_entry_no == 5)
    graphics_draw_bitmap_in_rect(ctx, icon_entry_5, GRect(0, 0, 64, 15));
}

/*
 * Bullet callback handler
 */
static void bullet_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  graphics_context_set_text_color(ctx, g_disp_static_color);
  graphics_draw_text(ctx, "•", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 0, 12, 20), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void update_event_display() {
  strncpy(g_disp_event_title_static, g_event_title_static, sizeof(g_disp_event_title_static));
  strncpy(g_disp_event_start_date_static, g_event_start_date_static, sizeof(g_disp_event_start_date_static));
  strncpy(g_disp_location_static, g_location_static, sizeof(g_disp_location_static));
  text_layer_set_text(text_event_title_layer, g_disp_event_title_static);
  text_layer_set_text(text_event_start_date_layer, g_disp_event_start_date_static);
  text_layer_set_text(text_event_location_layer, g_disp_location_static);
  g_disp_static_entry_no = g_static_entry_no;
  g_disp_static_color = g_static_color;
  layer_mark_dirty(entry_layer);
}

/**
 * First phase of event display animation complete
 */
static void animation_stopped(Animation *animation, bool finished, void *data) {
  if (finished) {
    update_event_display();

    if (prop_animation_in != NULL) {
      if(animation_is_scheduled((struct Animation *) prop_animation_in)) {
        animation_unschedule((struct Animation *) prop_animation_in);
      }
      prop_animation_in = NULL;
    }
    
    if (prop_animation_in == NULL) {
      GRect to_rect = SCROLL_IN;
      GRect from_rect = SCROLL_OUT;
      prop_animation_in = property_animation_create_layer_frame(scroll_layer, &from_rect, &to_rect);
      animation_set_duration((Animation*) prop_animation_in, 300);
    }
    animation_schedule((Animation*) prop_animation_in);
  }
}

/*
 * Display update in the event area
 */
void set_event_display(char *event_title, char *event_start_date, char *location, int num, GColor color) {

  // Already showing nothing and being asked to show nothing again is not
  // a good reason to animate anything
  if (g_static_entry_no == num && num == 0) {
    return;
  }

  strncpy(g_event_title_static, event_title, sizeof(g_event_title_static));
  strncpy(g_event_start_date_static, event_start_date, sizeof(g_event_start_date_static));
  strncpy(g_location_static, location, sizeof(g_location_static));
  g_static_entry_no = num;
  g_static_color = color;
  

  if (!get_config_data()->animate) {
    update_event_display();
    return;
  }

  if (prop_animation_out != NULL) {
    if (animation_is_scheduled((struct Animation *) prop_animation_out)) {
      animation_unschedule((struct Animation *) prop_animation_out);
    }
    prop_animation_out = NULL;
  }

  if (prop_animation_out == NULL) {
    GRect to_rect = SCROLL_OUT_LEFT;
    GRect from_rect = SCROLL_IN;
    prop_animation_out = property_animation_create_layer_frame(scroll_layer, &from_rect, &to_rect);
    animation_set_duration((Animation*) prop_animation_out, 300);
    animation_set_curve((Animation*) prop_animation_out, AnimationCurveEaseInOut);
    animation_set_handlers((Animation*) prop_animation_out, (AnimationHandlers ) { .stopped = (AnimationStoppedHandler) animation_stopped, }, NULL /* callback data */);
  }
  animation_schedule((Animation*) prop_animation_out);
}

/*
 * Pebble word callback handler
 */
static void pebble_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  graphics_draw_bitmap_in_rect(ctx, icon_pebble, GRect(0, 0, 27, 8));
}

#ifndef PBL_COLOR
  /*
   * Screen inverse setting
   */
  void set_screen_inverse_setting() {
    layer_set_hidden(inverter_layer_get_layer(full_inverse_layer), !get_config_data()->invert);
  }
#endif

/*
 * Display initialisation and further setup 
 */
static void window_load(Window *window) {

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
  text_date_layer = text_layer_create(GRect(-5, 94, 154, 168-94));
  text_layer_set_text_color(text_date_layer, DATE_COLOR);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_date_layer));
  
  time_layer = layer_create(CLOCK_IN);
  layer_add_child(window_get_root_layer(window), time_layer);

  #ifdef PBL_COLOR
    // Time shadow
    text_time_layer2 = text_layer_create(GRect(2,2,140,56));
    text_layer_set_text_color(text_time_layer2, GColorLightGray);
    text_layer_set_background_color(text_time_layer2, GColorClear);
    text_layer_set_font(text_time_layer2, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
    text_layer_set_text_alignment(text_time_layer2, GTextAlignmentCenter);
    layer_add_child(time_layer, text_layer_get_layer(text_time_layer2));
  #endif

  // Time
  text_time_layer = text_layer_create(GRect(0,0,140,56));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
  layer_add_child(time_layer, text_layer_get_layer(text_time_layer));

  // Line
  line_layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_get_root_layer(window), line_layer);

  scroll_layer = layer_create(SCROLL_IN);
  layer_add_child(window_get_root_layer(window), scroll_layer);

  // Location
  text_event_location_layer = text_layer_create(GRect(1, 54-16, 144 - 1, 27));
  text_layer_set_text_color(text_event_location_layer, GColorWhite);
  text_layer_set_background_color(text_event_location_layer, GColorClear);
  text_layer_set_font(text_event_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(scroll_layer, text_layer_get_layer(text_event_location_layer));

  // Date
  text_event_start_date_layer = text_layer_create(GRect(1, 36-16, 144 - 1, 27));
  text_layer_set_text_color(text_event_start_date_layer, GColorWhite);
  text_layer_set_background_color(text_event_start_date_layer, GColorClear);
  text_layer_set_font(text_event_start_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(scroll_layer, text_layer_get_layer(text_event_start_date_layer));

  // Event title
  #ifdef PBL_COLOR
    GRect title_text_xy = GRect(13, 0, 144 - 13, 27);
  #else
    GRect title_text_xy = GRect(1, 0, 144 - 1, 27);
  #endif    
  
  text_event_title_layer = text_layer_create(title_text_xy);
  text_layer_set_text_color(text_event_title_layer, GColorWhite);
  text_layer_set_background_color(text_event_title_layer, GColorClear);
  text_layer_set_font(text_event_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(scroll_layer, text_layer_get_layer(text_event_title_layer));
  
  #ifdef PBL_COLOR
    // Bullet calendar
    bullet_layer = layer_create(GRect(1,3,12,20));
    layer_set_update_proc(bullet_layer, bullet_layer_update_callback);
    layer_add_child(scroll_layer, bullet_layer);
  #endif

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

  #ifndef PBL_COLOR
    // Configurable inverse
    full_inverse_layer = inverter_layer_create(GRect(0, 0, 144, 168));
    layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(full_inverse_layer));
    set_screen_inverse_setting();
  #endif

  // Put everything in an initial state
  set_battery(1, 50);
  set_status(STATUS_REQUEST);
  set_event_display("", "", "", 0, GColorBlack);

  // Make sure the timers start but don't all go off together
  calendar_init();
}

static void update_clock_display() {
  clock_copy_time_string(g_time_text, sizeof(g_time_text));
  if (g_time_text[4] == ' ')
    g_time_text[4] = '\0';
  text_layer_set_text(text_time_layer, g_time_text);
  #ifdef PBL_COLOR
      text_layer_set_text(text_time_layer2, g_time_text);
  #endif
}

/*
 * End of first phase of time animation - out of view so update time
 */
static void clock_animation_stopped(Animation *animation, bool finished, void *data) {
  if (finished) {
    update_clock_display();

    if (clock_animation_in != NULL) {
      if (animation_is_scheduled((struct Animation *) clock_animation_in)) {
        animation_unschedule((struct Animation *) clock_animation_in);
      }
      clock_animation_in = NULL;
    }

    if (clock_animation_in == NULL) {
      GRect to_rect = CLOCK_IN;
      GRect from_rect = CLOCK_OUT;
      clock_animation_in = property_animation_create_layer_frame(time_layer, &from_rect, &to_rect);
      animation_set_duration((Animation*) clock_animation_in, 400);
    }
    animation_schedule((Animation*) clock_animation_in);
  }
}

/*
 * Display update in the event area
 */
static void update_clock() {

  if (!get_config_data()->animate) {
    update_clock_display();
    return;
  }

  if (clock_animation_out != NULL) {
    if (animation_is_scheduled((struct Animation *) clock_animation_out)) {
    animation_unschedule((struct Animation *) clock_animation_out);
    }
    clock_animation_out = NULL;
  }

  if (clock_animation_out == NULL) {
    GRect to_rect = CLOCK_OUT;
    GRect from_rect = CLOCK_IN;
    clock_animation_out = property_animation_create_layer_frame(time_layer, &from_rect, &to_rect);
    animation_set_duration((Animation*) clock_animation_out, 400);
    animation_set_curve((Animation*) clock_animation_out, AnimationCurveEaseInOut);
    animation_set_handlers((Animation*) clock_animation_out, (AnimationHandlers ) { .stopped = (AnimationStoppedHandler) clock_animation_stopped, }, NULL /* callback data */);
  }
  animation_schedule((Animation*) clock_animation_out);
}

/*
 * Date according to various formats
 */
void date_update() {
  time_t time_now = time(NULL);
  struct tm *tick_time = localtime(&time_now);

  char date_format[20];
  if (get_config_data()->day_name) {
    if (get_config_data()->month_name) {
      strncpy(date_format, "%a, %b %e", sizeof(date_format));
    } else {
      strncpy(date_format, "%A %e", sizeof(date_format));
    }
  } else {
    if (get_config_data()->month_name) {
      strncpy(date_format, "%B %e", sizeof(date_format));
    } else {
      strncpy(date_format, "%e", sizeof(date_format));
    }
  }
  if (get_config_data()->week_no) {
    char temp[7];
    strftime(temp, sizeof(temp), "%W", tick_time);
    int week = a_to_i(temp, 2) + 1;
    snprintf(temp, sizeof(temp), " (%d)", week);
    strncat(date_format, temp, sizeof(date_format));
  }

  strftime(g_date_text, sizeof(g_date_text), date_format, tick_time);
  #ifdef TEST_MODE
    strcpy(g_date_text, "Wed, May 18 (28)");
  #endif
  
  text_layer_set_text(text_date_layer, g_date_text);

}

/*
 * Clock tick
 */
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {

  second_timer();

  if (tick_time->tm_sec != 0 && !first_tick)
    return;

  first_tick = false;

  // Only update the date when it's changed.
  if (tick_time->tm_mday != g_last_tm_mday_date) {
    g_last_tm_mday_date = tick_time->tm_mday;
    date_update();
  }

  update_clock();

  minute_timer(tick_time->tm_min);
}

/*
 * Set everything up
 */
static void init(void) {

  read_config_data();

  window = window_create();
  window_set_background_color(window, GColorBlack);
  #ifndef PBL_COLOR
    window_set_fullscreen(window, true);
  #endif
  window_set_window_handlers(window, (WindowHandlers ) { .load = window_load, .unload = window_unload });

  const int inbound_size = app_message_inbox_size_maximum();
  const int outbound_size = app_message_outbox_size_maximum();

  app_message_open(inbound_size, outbound_size);

  window_stack_push(window, true);

  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);

  app_message_register_inbox_received(received_message);
  app_message_register_outbox_failed(out_failed_handler);

  // Accelerometer
  accel_data_service_subscribe(10, &accel_data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);

  battery_state_service_subscribe(battery_state_handler);

}

/*
 * Close final stuff down
 */
static void deinit() {
  save_config_data(NULL);
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  accel_data_service_unsubscribe();
  battery_state_service_unsubscribe();
}

/*
 * Mainness has been restored
 */
int main(void) {
  init();
  app_event_loop();
  deinit();
}

