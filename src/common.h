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
#define ALERT_EVENT 100
#define RESTORE_DATE 201
#define SECOND_ALERT 202
	
#define ROTATE_EVENT 203

#define CLOCK_STYLE_12H 1
#define CLOCK_STYLE_24H 2
	
#define MAX_EVENTS 15
#define ROT_MAX 5
	
#define STATUS_REQUEST 1
#define STATUS_REPLY 2
#define STATUS_ALERT_SET 3
	
#define	MAX_ALLOWABLE_ALERTS 10

#define BASIC_SIZE 21
#define START_DATE_SIZE 18
#define CLOSE_DATE_SIZE 6
#define CLOSE_DAY_NAME_SIZE 10

	
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
  char dayName[CLOSE_DAY_NAME_SIZE];
} CloseDay;

typedef struct {
  AppTimerHandle handle;
  bool active;
  char event_desc[BASIC_SIZE];
  char relative_desc[BASIC_SIZE]; 
  char location[BASIC_SIZE];
} TimerRecord;


#define ROTATE_EVENT_INTERVAL_MS 3005
#define ROTATE_EVENT_INTERVAL_OVERNIGHT_MS 10005

#define OVERNIGHT_START 0
#define OVERNIGHT_END 6
	
#define TODAY "Today"
#define TOMORROW "Tomorrow"
#define ALL_DAY "All day"
#define TEXT_NOW "Now"
#define TEXT_MINS "In %ld mins"
#define TEXT_HRS "In %ld hours"
	
void time_plus_day(PblTm *time, int daysToAdvance);
bool is_overnight();
void calendar_init(AppContextRef ctx);
void handle_calendar_timer(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie);
void draw_date();
void received_message(DictionaryIterator *received, void *context);
void set_status(int new_status_display);
void set_event_display(char *event_title, char *event_start_date, char *location, int num);
void set_battery(uint8_t state, int8_t level);
void set_invert_when_showing_event(bool invert);
void sync_timed_event(int tm_min, AppContextRef ctx);
int a_to_i(char *val, int len);
bool is_date_today(char *date);

#endif
