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

#ifdef TEST_MODE
  void populate_dummy_data(EVENT_TYPE *g_events) {
    g_events->index = 1;
    strcpy(g_events->title, "Alien autopsy");
    g_events->has_location = true;
    strcpy(g_events->location, "Area 51");
    g_events->all_day = false;
    g_events->start_date = time(NULL);
    g_events->end_date = time(NULL) + 3600;
    g_events->alarms[0] = 20;
    g_events->alarms[1] = 300;
    #ifdef PBL_COLOR
      g_events->calendar_has_color = true;
      g_events->calendar_color[0] = 255;
      g_events->calendar_color[1] = 128;
      g_events->calendar_color[2] = 0;
    #endif
    g_events++;
    g_events->index = 2;
    strcpy(g_events->title, "Banana bending");
    g_events->has_location = true;
    strcpy(g_events->location, "Banana farm");
    g_events->all_day = false;
    g_events->start_date = time(NULL)+ 4000;
    g_events->end_date = time(NULL) + 5000;
    g_events->alarms[0] = 20;
    g_events->alarms[1] = 300;
    #ifdef PBL_COLOR
      g_events->calendar_has_color = true;
      g_events->calendar_color[0] = 0;
      g_events->calendar_color[1] = 255;
      g_events->calendar_color[2] = 0;
    #endif
  }
#endif
