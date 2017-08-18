/*
 * Copyright (C) 2017 Karl Otness
 *
 * This file is part of KBarInfo.
 *
 * KBarInfo is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KBarInfo is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KBarInfo. If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>

#include "../utils/state.h"
#include "../utils/debug.h"
#include "time.h"

#define TIME_FMT "%a %b %d - %I:%M %p %Z"
#define TIME_MIN_SLEEP 5
#define TIME_MAX_SLEEP 60

static gboolean kbar_time_tick(void *data);

kbar_widget_state kbar_time_state = {0};
guint kbar_time_timer = 0;

gboolean kbar_time_init() {
  kbar_widget_state_init(&kbar_time_state);
  kbar_time_timer = 0;
  kbar_time_tick(NULL);
}

void kbar_time_free() {
  g_source_remove(kbar_time_timer);
  kbar_widget_state_release(&kbar_time_state);
}

static gboolean kbar_time_tick(void *data) {
  GDateTime *g_time = g_date_time_new_now_local();
  if(!g_time) {
    kbar_err_printf("Error getting current time\n");
    return G_SOURCE_CONTINUE;
  }
  // Compute how long to sleep before next wakeup
  gint sec = g_date_time_get_second(g_time);
  gint wait_secs = 60 - sec;
  if(wait_secs < TIME_MIN_SLEEP) {
    wait_secs = TIME_MIN_SLEEP;
  }
  else if(wait_secs > TIME_MAX_SLEEP) {
    wait_secs = TIME_MAX_SLEEP;
  }
  // Re-add timer call with new timeout
  kbar_time_timer = g_timeout_add_seconds(wait_secs, &kbar_time_tick, NULL);
  // Produce new time string
  gchar *time_str = g_date_time_format(g_time, TIME_FMT);
  if(!time_str) {
    kbar_err_printf("Error producing time string\n");
    g_date_time_unref(g_time);
    return G_SOURCE_CONTINUE;
  }
  // Set new value
  g_string_assign(kbar_time_state.text, time_str);
  // Free allocated values
  g_free(time_str);
  g_date_time_unref(g_time);
  kbar_print_bar_state();
  return G_SOURCE_REMOVE;
}
