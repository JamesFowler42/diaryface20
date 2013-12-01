#include "pebble.h"
#include "common.h"

/*
 * Leap year routine
 */
bool isLeapYear(int year)
{
    if (year%400==0) return true;
    if (year%100==0) return false;
    return (year%4==0);
}

/*
 * Crude add days routine
 */
void time_plus_day(struct tm *time, int daysToAdvance) {
  
  int daysPerMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  
  int year = time->tm_year + 1900;
  int month = time->tm_mon + 1;
  int day = time->tm_mday;
  int wday = time->tm_wday;
 
  for (int i=0;i<daysToAdvance;i++)
  {
      day++;
      if (day > daysPerMonth[month-1] || ( month==2 && day==29 && !isLeapYear(year) ) )
      {
          day = 1;
          month++;
          if (month==13)
          {
              month = 1;
              year++;
          }
      }
	  wday++;
	  if (wday > 6)
		  wday = 0;
  }

  time->tm_year = year - 1900;
  time->tm_mon = month - 1;
  time->tm_mday = day;
  time->tm_wday = wday;
	
}

/*
 * Is overnight
 */
bool is_overnight() {
	time_t now = time(NULL);
	struct tm *time = localtime(&now);
	return (time.tm_hour >= OVERNIGHT_START && time.tm_hour <= OVERNIGHT_END);
}

/*
 * Crude conversion of character strings to integer
 */
int a_to_i(char *val, int len) {
	int result = 0;
	for (int i=0; i < len; i++) {
		if (val[i] < '0' || val[i] > '9')
			break;
		result = result * 10;
		result = result + (val[i]-'0');
	}
	return result;
}

/*
 * is the date provided today?
 */
bool is_date_today(char *date) {
	char temp[6];
	time_t now = time(NULL);
	struct tm *time = localtime(&now);
    strftime(temp, sizeof(temp), "%m/%d", &time);
	if (strncmp(date, temp, 5) == 0)
		return true;
	else
		return false;
}