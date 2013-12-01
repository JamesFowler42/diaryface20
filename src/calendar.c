#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "common.h"
	
Event g_events[MAX_EVENTS];
Event g_rot_events[ROT_MAX];
TimerRecord g_timer_rec[MAX_EVENTS * 2];
BatteryStatus g_battery_status;

uint8_t g_count;
uint8_t g_received_rows;
uint8_t g_max_entries = 0;
uint8_t g_alerts_issued = 0;
uint8_t g_entry_no = 0;
bool g_first_time = true;

CloseDay g_close[7];
int g_last_tm_mday = -1;

AppContextRef g_app_context;

bool g_showing_alert = false;

/*
 * Make a calendar request
 */
void calendar_request() {

  DictionaryIterator *iter;
  app_message_out_get(&iter);
  if (!iter) 
	return;
		
  dict_write_int8(iter, REQUEST_CALENDAR_KEY, -1);
  dict_write_uint8(iter, CLOCK_STYLE_KEY, CLOCK_STYLE_24H);
  g_count = 0;
  g_received_rows = 0;
  app_message_out_send();
  app_message_out_release();
  set_status(STATUS_REQUEST);
}

/*
 * Make a battery status request
 */
void battery_request() {

  DictionaryIterator *iter;
  app_message_out_get(&iter);
  if (!iter) 
	return;

  dict_write_uint8(iter, REQUEST_BATTERY_KEY, 1);
  app_message_out_send();
  app_message_out_release();
}

/*
 * Get the calendar running
 */
void calendar_init(AppContextRef ctx) {
  g_app_context = ctx;
  for (uint8_t i=0; i < (MAX_EVENTS * 2); i++) {
     g_timer_rec[i].active = false;
  }	
  memset(&g_events, 0, sizeof(Event) * MAX_EVENTS);
  app_timer_send_event(ctx, ROTATE_EVENT_INTERVAL_MS, ROTATE_EVENT);
}

/*
 * Build a cache of dates vs day names
 */
void ensure_close_day_cache() {

	PblTm time;
	get_time(&time);
	
	if (time.tm_mday == g_last_tm_mday)
		return;
	
	g_last_tm_mday = time.tm_mday;
	
	for (int i=0; i < 7; i++) {
  	  get_time(&time);
	  if (i>0)
	    time_plus_day(&time, i);
	  string_format_time(g_close[i].date, CLOSE_DATE_SIZE, "%m/%d", &time);
	  string_format_time(g_close[i].dayName, CLOSE_DAY_NAME_SIZE, "%A", &time);
	}
	strcpy(g_close[0].dayName, TODAY);
	strcpy(g_close[1].dayName, TOMORROW);
}

/*
 * Alter the raw date and time returned by iOS to be really nice on the eyes at a glance
 */
void modify_calendar_time(char *output, int outlen, char *date, bool all_day) {
	
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
	for (int i=0; i < 7; i++) {
		if (strncmp(g_close[i].date, date, 5) == 0) {
			strncpy(temp, g_close[i].dayName, sizeof(temp));
			found = true;
			break;
		}
	}

	// If not found then show the month and the day
	if (!found) {
		PblTm time;
	    get_time(&time);
		time.tm_mday = a_to_i(&date[3],2);
		time.tm_mon = a_to_i(&date[0],2) - 1;
		string_format_time(temp, sizeof(temp), "%b %e -", &time);
	}
	// Change the format based on whether there is a timestamp
	if (all_day)
		snprintf(output, outlen, "%s %s", temp, ALL_DAY);
	else
	    snprintf(output, outlen, "%s %s", temp, &date[time_position]);
}

/*
 * Queue an alert
 */
void queue_alert(int num, int32_t alarm_time, char *title, int32_t alert_event, char *location) {
  // work out relative time
  char relative_temp[21];
  if (alarm_time == 0)
       strncpy(relative_temp, TEXT_NOW, sizeof(relative_temp));
  else if (alarm_time <  3600)
       snprintf(relative_temp, sizeof(relative_temp), TEXT_MINS, alarm_time / 60);
  else 
       snprintf(relative_temp, sizeof(relative_temp), TEXT_HRS, alarm_time / 3600);

  // Create an alert
  g_timer_rec[num].handle = app_timer_send_event(g_app_context, alert_event, ALERT_EVENT + num);
  g_timer_rec[num].active = true;
  strncpy(g_timer_rec[num].event_desc, title, BASIC_SIZE); 
  strncpy(g_timer_rec[num].relative_desc, relative_temp, BASIC_SIZE);
  strncpy(g_timer_rec[num].location, location, BASIC_SIZE);
}

/*
 * Do we need an alert? if so schedule one. 
 */
int determine_if_alarm_needed(int num) {
  // Copy the event for easy access
  Event event;	
  memcpy(&event, &g_events[num], sizeof(Event));
	
  // Alarms set
  int alarms_set = 0;
	
  // Ignore all day events
	if (event.all_day) {
	  return alarms_set; 
	}

  // Is the event today
  if (is_date_today(event.start_date) == false) {
	  return alarms_set;
  }
 	
  // Does the event have an alarm
  if (event.alarms[0] == -1 && event.alarms[1] == -1) {
	  return alarms_set;
  }

  // Compute the event start time as a figure in ms
  int time_position = 9;
  if (event.start_date[5] != '/')
		time_position = 6;

  int hour = a_to_i(&event.start_date[time_position],2);
  int minute_position = time_position + 3;
  if (event.start_date[time_position + 1] == ':')
	  minute_position = time_position + 2;
  int minute = a_to_i(&event.start_date[minute_position],2);
	
  uint32_t event_in_ms = (hour * 3600 + minute * 60) * 1000;
	
  // Get now as ms
  PblTm time;
  get_time(&time);
  uint32_t now_in_ms = (time.tm_hour * 3600 + time.tm_min * 60 + time.tm_sec) * 1000;

  // First alart
  if (event.alarms[0] != -1) {
	  
  	// Work out the alert interval  
  	int32_t alert_event = event_in_ms - now_in_ms - (event.alarms[0] * 1000);

  	// If this is negative then we are after the alert period
  	if (alert_event >= 0) {
		
		// Make sure we have the resources for another alert
		g_alerts_issued++;
		if (g_alerts_issued > MAX_ALLOWABLE_ALERTS)	
			return alarms_set;
		
		// Queue alert
		queue_alert(num, event.alarms[0], event.title, alert_event, event.has_location ? event.location : "");
		alarms_set++;
    }
  }

  // Second alart
  if (event.alarms[1] != -1) {

    // Work out the alert interval  
  	int32_t alert_event = event_in_ms - now_in_ms - (event.alarms[1] * 1000);

  	// If this is negative then we are after the alert period
  	if (alert_event >= 0) {

		// Make sure we have the resources for another alert
		g_alerts_issued++;
		if (g_alerts_issued > MAX_ALLOWABLE_ALERTS)	
			return alarms_set;

		// Queue alert
		queue_alert(num + 15, event.alarms[1], event.title, alert_event, event.has_location ? event.location : "");
		alarms_set++;
    }
  }

  return alarms_set;
}

/*
 * Clear existing timers
 */
void clear_timers() {
	for (int i=0; i < (MAX_EVENTS * 2); i++) {
		if (g_timer_rec[i].active) {
			g_timer_rec[i].active = false;
			app_timer_cancel_event(g_app_context, g_timer_rec[i].handle);
		}
	}
}

/*
 * Work through events returned from iphone
 */
void process_events() {
  if (g_max_entries == 0) {
    clear_timers();	
  } else {
    clear_timers();	
	int alerts = 0;
	g_alerts_issued = 0;  
    for (uint8_t i = 0; i < g_max_entries; i++) 
	  alerts = alerts + determine_if_alarm_needed(i);
	if (alerts > 0) 
		set_status(STATUS_ALERT_SET);
   }
}

/*
 * Prepare events for rotation display
 */
void process_rot_events() {
	char event_date[BASIC_SIZE];
	if (g_max_entries == 0)
		return;
	memcpy(&g_rot_events, &g_events, sizeof(Event) * ROT_MAX);
	uint8_t limit = ROT_MAX < g_max_entries ? ROT_MAX : g_max_entries;
	for (uint8_t i = 0; i < limit; i++) {
  		// Process the date into something nicer to read
  		modify_calendar_time(event_date, BASIC_SIZE, g_rot_events[i].start_date, g_rot_events[i].all_day); 
      	strncpy(g_rot_events[i].start_date, event_date, START_DATE_SIZE);
	}
}

/*
 * Clear the event display area
 */
void clear_event() {
  set_event_display("", "", "", 0);
}

/*
 * Show new details in the event display area
 */
void show_event(uint8_t num) {
  uint8_t i = num - 1;
  set_event_display(g_rot_events[i].title, g_rot_events[i].start_date, g_rot_events[i].has_location ? g_rot_events[i].location : "", num);
}

/*
 * Decide which event to show. Works in a cycle. Clears the display if there is a calendar event outstanding. The phone could
 * have been offline for some while and hence what is being displayed could be inaccurate.
 */
void show_next_event() {
  // Don't mess up an alert that is underway
  if (g_showing_alert)
	  return;
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
 * Messages incoming from the phone
 */
void received_message(DictionaryIterator *received, void *context) {
   Event temp_event;
	
   Tuple *tuple = dict_find(received, RECONNECT_KEY);
   if (tuple) {
	 vibes_short_pulse();
	 calendar_request();	
   }
	
   // Gather the bits of a calendar together	
   tuple = dict_find(received, CALENDAR_RESPONSE_KEY);
	  
   if (tuple) {
	    set_status(STATUS_REPLY);
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
    	    memcpy(&temp_event, &tuple->value->data[j], sizeof(Event));
			if (temp_event.index < MAX_EVENTS)
      	        memcpy(&g_events[temp_event.index], &temp_event, sizeof(Event));

      	    i++;
      	    j += sizeof(Event);
        }

        g_received_rows = i;

        if (g_count == g_received_rows) {
			g_max_entries = g_count;
			process_events();
			process_rot_events();
			battery_request();
	    }
	}
	
	tuple = dict_find(received, BATTERY_RESPONSE_KEY);

    if (tuple) {
        memcpy(&g_battery_status, &tuple->value->data[0], sizeof(BatteryStatus));
		set_battery(g_battery_status.state, g_battery_status.level);
    }

}

/*
 * Clock syncronised timer
 */
void sync_timed_event(int tm_min, AppContextRef ctx) {
	
	// Only on the 10 minute clicks (importantly includes midnight)
	if (tm_min % 10 != 0 && !g_first_time)
		return;

	if (g_first_time) {
		g_first_time = false;
		calendar_request();
	} else {
		// Delay for 35 seconds after the minute to ensure that alerts have already happened
		app_timer_send_event(ctx, 35000, REQUEST_CALENDAR_KEY);	
	}

}


/*
 * Timer handling. Includes a hold off for a period of time if there is resource contention
 */
void handle_calendar_timer(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie) {
	
  // If we're rotating the visible event, get on with it. Slower overnight to save power
  if (cookie == ROTATE_EVENT) {
	  // Clobber the timer
	  app_timer_cancel_event(app_ctx, handle);
	  
	  // Show next event 
      show_next_event();
	  
	  // Kick off new timer
	  if (is_overnight()) 
    	app_timer_send_event(app_ctx, ROTATE_EVENT_INTERVAL_OVERNIGHT_MS, cookie);
	  else
    	app_timer_send_event(app_ctx, ROTATE_EVENT_INTERVAL_MS, cookie);
	
	  // Retire from active duty
	  return;
  }
	
  // Show the alert and let the world know
  if (cookie >= ALERT_EVENT && cookie <= (ALERT_EVENT + (MAX_EVENTS * 2))) {
	  app_timer_cancel_event(app_ctx, handle);
	  int num = cookie - ALERT_EVENT;
	  if (g_timer_rec[num].active == false)
		  return; // Already had the data for this event deleted - cannot show it.
	  g_timer_rec[num].active = false;
	  g_showing_alert = true;
      set_event_display(g_timer_rec[num].event_desc, g_timer_rec[num].relative_desc, g_timer_rec[num].location, 0);
	  set_invert_when_showing_event(true);
	  app_timer_send_event(app_ctx, 30000, RESTORE_DATE);	
	  app_timer_send_event(app_ctx, 15000, SECOND_ALERT);	
	  vibes_double_pulse();
      light_enable_interaction();
	  return;
  }

  // Let us know again
  if (cookie == SECOND_ALERT) {
      app_timer_cancel_event(app_ctx, handle);
      vibes_double_pulse();
      light_enable_interaction();
      return;
   }
      
  // Put the date back into the display area	
  if (cookie == RESTORE_DATE) {
	  app_timer_cancel_event(app_ctx, handle);
	  g_showing_alert = false;
	  show_next_event();
	  set_invert_when_showing_event(false);
	  return;
  }

  // Fire the calendar request
  if (cookie == REQUEST_CALENDAR_KEY) {
	  app_timer_cancel_event(app_ctx, handle);
	  calendar_request();
  }

}