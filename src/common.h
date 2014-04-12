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

#ifndef common_h
#define common_h

#include "pebble.h"

//#define INVERSE

#define RECONNECT_KEY 0
#define REQUEST_CALENDAR_KEY 1
#define CLOCK_STYLE_KEY 2
#define CALENDAR_RESPONSE_KEY 3
#define REQUEST_BATTERY_KEY 8
#define BATTERY_RESPONSE_KEY 9
#define REQUEST_SETTINGS_KEY 27
#define SETTINGS_RESPONSE_KEY 28

#define SETTINGS_KEY_INVERSE 200
#define SETTINGS_KEY_ANIMATE 201
#define SETTINGS_KEY_DAY_NAME 202
#define SETTINGS_KEY_MONTH_NAME 203
#define SETTINGS_KEY_WEEK_NO 204

#define CLOCK_STYLE_12H 1
#define CLOCK_STYLE_24H 2

#define MAX_EVENTS 15
#define ROT_MAX 5

#define STATUS_REQUEST 1
#define STATUS_REPLY 2

#define BASIC_SIZE 21
#define START_DATE_SIZE 18
#define CLOSE_DATE_SIZE 6
#define CLOSE_DAY_NAME_SIZE 10

#define PERSIST_CONFIG_KEY 12434
#define PERSIST_CONFIG_MS 500

typedef struct {
  bool invert;
  bool animate;
  bool day_name;
  bool month_name;
  bool week_no;
} ConfigData;

typedef struct {
  uint8_t index;
  char title[BASIC_SIZE];
  bool has_location;
  char location[BASIC_SIZE];
  bool all_day;
  char start_date[START_DATE_SIZE];
  int32_t alarms[2];
} Event;

typedef struct {
  uint8_t state;
  int8_t level;
} BatteryStatus;

typedef struct {
  char date[CLOSE_DATE_SIZE];
  char day_name[CLOSE_DAY_NAME_SIZE];
} CloseDay;

#define MIN_SECOND_PER_ROTATE 4
#define MID_SECOND_PER_ROTATE 10
#define MAX_SECOND_PER_ROTATE 30

#define TODAY "Today"
#define TOMORROW "Tomorrow"
#define ALL_DAY "All day"

#define SCROLL_IN GRect(0,16,144,75)
#define SCROLL_OUT GRect(143,16,144,75)
#define SCROLL_OUT_LEFT GRect(-144,16,144,74)

#define CLOCK_IN GRect(0, 112, 143, 168-112)
#define CLOCK_OUT GRect(0, 168, 143, 168-112)

void time_plus_day(struct tm *time, int daysToAdvance);
bool is_overnight();
void calendar_init();
void handle_calendar_timer(void *data);
void draw_date();
void received_message(DictionaryIterator *received, void *context);
void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);
void set_status(int new_status_display);
void set_event_display(char *event_title, char *event_start_date, char *location, int num);
void set_battery(uint8_t state, int8_t level);
void second_timer();
void minute_timer(int tm_min);
int a_to_i(char *val, int len);
bool is_date_today(char *date);
void accel_data_handler(AccelData *data, uint32_t num_samples);
void set_screen_inverse_setting();
void save_config_data(void *data);
void read_config_data();
ConfigData *get_config_data();
void date_update();

#endif
