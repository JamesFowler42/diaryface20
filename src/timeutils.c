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
	struct tm *time_tm = localtime(&now);
    strftime(temp, sizeof(temp), "%m/%d", time_tm);
	if (strncmp(date, temp, 5) == 0)
		return true;
	else
		return false;
}
