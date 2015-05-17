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

static EVENT_TYPE g_events[MAX_EVENTS];
static EventInternal g_rot_events[ROT_MAX];
static BatteryStatus g_battery_status;

static uint8_t g_count;
static uint8_t g_received_rows;
static uint8_t g_max_entries = 0;
static uint8_t g_entry_no = 0;
static bool g_first_time = true;

static CloseDay g_close[7];
static int g_last_tm_mday = -1;

static uint8_t rotate_tick = 0;
static uint8_t rotate_change = MID_SECOND_PER_ROTATE;

static bool calendar_refresh_queued = false;
static bool initial_state = true;

static ConfigData config_data;
bool save_config_requested = false;

/*
 * Save the config data structure
 */
void save_config_data(void *data) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "save_config_data (%d)",  sizeof(config_data));
  int written = persist_write_data(PERSIST_CONFIG_KEY, &config_data, sizeof(config_data));
  if (written != sizeof(config_data)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "save_config_data error (%d)", written);
  }
  save_config_requested = false;
}

/*
 * Clear config if needed
 */
static void clear_config_data() {
  memset(&config_data, 0, sizeof(config_data));
  config_data.animate = true;
  config_data.day_name = true;
  config_data.invert = false;
  config_data.month_name = true;
  config_data.week_no = true;
}

/*
 * Read the config data (or create it if missing)
 */
void read_config_data() {
  if (persist_exists(PERSIST_CONFIG_KEY)) {
    int read = persist_read_data(PERSIST_CONFIG_KEY, &config_data, sizeof(config_data));
    if (read != sizeof(config_data)) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "read_config_data wrong size (%d)", read);
      clear_config_data();
    }
  } else {
    clear_config_data();
  }
}

/*
 * Provide config data structure to other units
 */
ConfigData *get_config_data() {
  return &config_data;
}

/*
 * Remember the configuration settings from the javascript
 */
static void request_persist_config() {
  if (!save_config_requested) {
    app_timer_register(PERSIST_CONFIG_MS, save_config_data, NULL);
    save_config_requested = true;
  }
}

/*
 * Make a calendar request
 */
#ifdef TEST_MODE
  static void calendar_request() {
    populate_dummy_data(g_events);
    calendar_refresh_queued = true;
    rotate_tick = rotate_change + 1;
    g_max_entries = 2;
  }
#else  
  static void calendar_request() {

    if (!bluetooth_connection_service_peek())
      return;

    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (!iter)
      return;

    dict_write_int8(iter, REQUEST_CALENDAR_KEY, -1);
    dict_write_uint8(iter, CALENDAR_RESPONSE_FORMAT_KEY, CALENDAR_RESPONSE_FORMAT_SELECTED);
    dict_write_end(iter);
    g_count = 0;
    g_received_rows = 0;
    app_message_outbox_send();
  }
#endif

/*
 * Make a battery status request
 */
static void battery_request() {

  if (!bluetooth_connection_service_peek())
    return;

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (!iter)
    return;

  dict_write_uint8(iter, REQUEST_BATTERY_KEY, 1);
  dict_write_end(iter);
  app_message_outbox_send();
}

/*
 * Make a config status request
 */
static void config_request() {

  if (!bluetooth_connection_service_peek())
    return;

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (!iter)
    return;

  dict_write_uint8(iter, REQUEST_SETTINGS_KEY, 1);
  dict_write_end(iter);
  app_message_outbox_send();
}

/*
 * Get the calendar running
 */
void calendar_init() {
  memset(&g_events, 0, sizeof(EVENT_TYPE) * MAX_EVENTS);
}

/*
 * Build a cache of dates vs day names
 */
static void ensure_close_day_cache() {

  time_t now = time(NULL);
  struct tm *now_tm = localtime(&now);

  struct tm fiddle;

  if (now_tm->tm_mday == g_last_tm_mday)
    return;

  g_last_tm_mday = now_tm->tm_mday;

  for (int i = 0; i < 7; i++) {
    memcpy(&fiddle, now_tm, sizeof(fiddle));
    if (i > 0)
      time_plus_day(&fiddle, i);
    strftime(g_close[i].date, CLOSE_DATE_SIZE, "%m/%d", &fiddle);
    strftime(g_close[i].day_name, CLOSE_DAY_NAME_SIZE, "%A", &fiddle);
  }
  strcpy(g_close[0].day_name, TODAY);
  strcpy(g_close[1].day_name, TOMORROW);
}

/*
 * Alter the raw date and time returned by iOS to be really nice on the eyes at a glance
 */
static void modify_calendar_time(char *output, int outlen, char *date, bool all_day) {

  // When "Show next events" is turned off in the app:
  // MM/dd
  // When "Show next events" is turned on:
  // MM/dd/yy

  // If all_day is false, time is added like so:
  // MM/dd(/yy) H:mm
  // If clock style is 12h, AM/PM is added:
  // MM/dd(/yy) H:mm a

  // Build a list of dates and day names closest to the current date
  ensure_close_day_cache();

  int time_position = 9;
  if (date[5] != '/')
    time_position = 6;

  // Find the date in the list prepared
  char temp[12];
  bool found = false;
  for (int i = 0; i < 7; i++) {
    if (strncmp(g_close[i].date, date, 5) == 0) {
      strncpy(temp, g_close[i].day_name, sizeof(temp));
      found = true;
      break;
    }
  }

  // If not found then show the month and the day
  if (!found) {
    time_t now = time(NULL);
    struct tm *now_tm = localtime(&now);
    struct tm fiddle;
    memcpy(&fiddle, now_tm, sizeof(fiddle));
    fiddle.tm_mday = a_to_i(&date[3], 2);
    fiddle.tm_mon = a_to_i(&date[0], 2) - 1;
    strftime(temp, sizeof(temp), "%b %e -", &fiddle);
  }
  // Change the format based on whether there is a timestamp
  if (all_day)
    snprintf(output, outlen, "%s %s", temp, ALL_DAY);
  else
    snprintf(output, outlen, "%s %s", temp, &date[time_position]);
}

/*
 * Convert from transport data structure to internal structure
 */
static void map_if_data_to_internal(EventInternal *rot, EVENT_TYPE *iface) {
  strncpy(rot->title, iface->title, member_size(EVENT_TYPE,title));
  rot->has_location = iface->has_location;
  strncpy(rot->location, iface->location, member_size(EVENT_TYPE,location));
  rot->all_day = iface->all_day;
  #ifdef PBL_COLOR
    if (iface->calendar_has_color) {
      rot->color = GColorFromRGB(iface->calendar_color[0],iface->calendar_color[1],iface->calendar_color[2] );
    } else {
      rot->color = DEFAULT_CALENDAR_COLOR;
    }
  #endif
  struct tm *start_date_tm = localtime(&(iface->start_date));  
  // MM/dd(/yy) H:mm
  if (clock_is_24h_style()) {
    strftime(rot->start_date, member_size(EventInternal, start_date), "%m/%d/%y %H:%M", start_date_tm); 
  } else {
    strftime(rot->start_date, member_size(EventInternal, start_date), "%m/%d/%y %l:%M %P", start_date_tm); 
  }
}

/*
 * Prepare events for rotation display
 */
static void process_rot_events() {
  char event_date[member_size(EventInternal, start_date)];
  if (g_max_entries == 0)
    return;
  for (uint8_t i = 0; i < ROT_MAX; i++) {
    map_if_data_to_internal(&g_rot_events[i], &g_events[i]);
  }
  uint8_t limit = ROT_MAX < g_max_entries ? ROT_MAX : g_max_entries;
  for (uint8_t i = 0; i < limit; i++) {
    // Process the date into something nicer to read
    modify_calendar_time(event_date, sizeof(event_date), g_rot_events[i].start_date, g_rot_events[i].all_day);
    strncpy(g_rot_events[i].start_date, event_date, sizeof(g_rot_events[i].start_date));
  }
}

/*
 * Clear the event display area
 */
static void clear_event() {
  set_event_display("", "", "", 0, GColorBlack);
}

/*
 * Show new details in the event display area
 */
static void show_event(uint8_t num) {
  uint8_t i = num - 1;
  set_event_display(g_rot_events[i].title, g_rot_events[i].start_date, g_rot_events[i].has_location ? g_rot_events[i].location : "", num, g_rot_events[i].color);
}

/*
 * Decide which event to show. Works in a cycle. Clears the display if there is a calendar event outstanding. The phone could
 * have been offline for some while and hence what is being displayed could be inaccurate.
 */
static void show_next_event() {
  // Else get processing
  if (g_max_entries == 0) {
    clear_event();
  } else {
    g_entry_no++;
    if (g_entry_no > g_max_entries || g_entry_no > ROT_MAX)
      g_entry_no = 1;
    show_event(g_entry_no);
  }
}

/*
 * Outgoing message failed handler
 */
void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "App Msg Send Fail %d", reason);
  set_status(STATUS_REQUEST);
}

/*
 * Messages incoming from the phone
 */
void received_message(DictionaryIterator *received, void *context) {
  EVENT_TYPE temp_event;

  Tuple *tuple = dict_find(received, RECONNECT_KEY);
  if (tuple) {
    calendar_request();
  }

  // Gather the bits of a calendar together
  tuple = dict_find(received, CALENDAR_RESPONSE_KEY);

  if (tuple) {
    uint8_t i, j;

    if (g_count > g_received_rows) {
      i = g_received_rows;
      j = 0;
    } else {
      g_count = tuple->value->data[0];
      i = 0;
      j = 1;
    }

    while (i < g_count && j < tuple->length) {
      memcpy(&temp_event, &tuple->value->data[j], sizeof(EVENT_TYPE));
      if (temp_event.index < MAX_EVENTS)
        memcpy(&g_events[temp_event.index], &temp_event, sizeof(EVENT_TYPE));

      i++;
      j += sizeof(EVENT_TYPE);
    }

    g_received_rows = i;

    if (g_count == g_received_rows) {
      set_status(STATUS_REPLY);
      g_max_entries = g_count;
      calendar_refresh_queued = true;
      battery_request();
    }
  }

  tuple = dict_find(received, BATTERY_RESPONSE_KEY);

  if (tuple) {
    memcpy(&g_battery_status, &tuple->value->data[0], sizeof(BatteryStatus));
    set_battery(g_battery_status.state, g_battery_status.level);
    config_request();
  }

  tuple = dict_find(received, SETTINGS_RESPONSE_KEY);

  if (tuple) {
    // read specified keys
    #ifndef PBL_COLOR
      tuple = dict_find(received, SETTINGS_KEY_INVERSE);

      if (tuple) { // be prepared to get null
        config_data.invert = tuple->value->uint8 != 0;
        request_persist_config();
        set_screen_inverse_setting();
      }
    #endif

    tuple = dict_find(received, SETTINGS_KEY_ANIMATE);

    if (tuple) { // be prepared to get null
      config_data.animate = tuple->value->uint8 != 0;
      request_persist_config();
    }

    tuple = dict_find(received, SETTINGS_KEY_DAY_NAME);

    if (tuple) { // be prepared to get null
      config_data.day_name = tuple->value->uint8 != 0;
      request_persist_config();
      date_update();
    }

    tuple = dict_find(received, SETTINGS_KEY_MONTH_NAME);

    if (tuple) { // be prepared to get null
      config_data.month_name = tuple->value->uint8 != 0;
      request_persist_config();
      date_update();
    }

    tuple = dict_find(received, SETTINGS_KEY_WEEK_NO);

    if (tuple) { // be prepared to get null
      config_data.week_no = tuple->value->uint8 != 0;
      request_persist_config();
      date_update();
    }
  }

}

/*
 * Update event timer
 */
void minute_timer(int tm_min) {

  // First time
  if (g_first_time) {
    g_first_time = false;
    calendar_request();
    return;
  }

  // Only on the 10 minute clicks (importantly includes midnight)
  if (tm_min % 10 != 0)
    return;

  calendar_request();
}

/**
 * Rotate timer
 */
void second_timer() {

  // rotate display
  rotate_tick++;
  if ((rotate_tick >= rotate_change) ||
      (initial_state && calendar_refresh_queued)) {
    if (calendar_refresh_queued) {
      initial_state = false;
      calendar_refresh_queued = false;
      process_rot_events();
    }
    rotate_tick = 0;
    show_next_event();
    if (rotate_change < MAX_SECOND_PER_ROTATE && g_entry_no == 1)
      rotate_change++;
  }
}

/*
 * Can't be bothered to play with negative numbers
 */
static uint16_t scale_accel(int16_t val) {
  int16_t retval = 4000 + val;
  if (retval < 0)
    retval = 0;
  return retval;
}

/*
 * Has pebble been tapped?
 */
void accel_data_handler(AccelData *data, uint32_t num_samples) {
  // Average the data
  uint32_t avg_x = 0;
  uint32_t avg_y = 0;
  uint32_t avg_z = 0;
  AccelData *dx = data;
  for (uint32_t i = 0; i < num_samples; i++, dx++) {
    avg_x += scale_accel(dx->x);
    avg_y += scale_accel(dx->y);
    avg_z += scale_accel(dx->z);
  }
  avg_x /= num_samples;
  avg_y /= num_samples;
  avg_z /= num_samples;

  // Work out deviations
  uint16_t biggest = 0;
  AccelData *d = data;
  for (uint32_t i = 0; i < num_samples; i++, d++) {
    uint16_t x = scale_accel(d->x);
    uint16_t y = scale_accel(d->y);
    uint16_t z = scale_accel(d->z);

    if (x < avg_x)
      x = avg_x - x;
    else
      x -= avg_x;

    if (y < avg_y)
      y = avg_y - y;
    else
      y -= avg_y;

    if (z < avg_z)
      z = avg_z - z;
    else
      z -= avg_z;

    // Store the largest deviation in the period
    if (x > biggest)
      biggest = x;
    if (y > biggest)
      biggest = y;
    if (z > biggest)
      biggest = z;

  }

  // Waggle wrist and we start rotating events faster
  if (biggest >= 200) {
    rotate_change = MIN_SECOND_PER_ROTATE;
  }

}

