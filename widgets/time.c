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
