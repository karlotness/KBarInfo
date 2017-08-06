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
#include <time.h>
#include <stdio.h>

#include "../utils/state.h"
#include "time.h"

#define TIME_FMT "%a %b %d - %I:%M %p %Z"

static gboolean kbar_time_tick(void *data);

kbar_widget_state kbar_time_state = {0};
guint kbar_time_timer = 0;

gboolean kbar_time_init() {
  kbar_widget_state_init(&kbar_time_state);
  kbar_time_timer = g_timeout_add_seconds(60, &kbar_time_tick, NULL);
  kbar_time_tick(NULL);
}

void kbar_time_free() {
  g_source_remove(kbar_time_timer);
  kbar_widget_state_release(&kbar_time_state);
}

static gboolean kbar_time_tick(void *data) {
  while(TRUE) {
    time_t cur_time = time(NULL);
    struct tm *tmp = localtime(&cur_time);
    size_t ttlen = strftime(kbar_time_state.text->str,
                            kbar_time_state.text->allocated_len,
                            TIME_FMT, tmp);
    if(ttlen > 0) {
      kbar_print_bar_state();
      return G_SOURCE_CONTINUE;
    }
    gsize time_len = kbar_time_state.text->allocated_len;
    time_len = 2 * (time_len + (time_len == 0 ? 1 : 0));
    g_string_set_size(kbar_time_state.text, time_len);
  }
}
