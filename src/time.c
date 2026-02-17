/*
 * Copyright 2025 Karl Otness
 * SPDX-License-Identifier: GPL-3.0-or-later
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
 * along with KBarInfo. If not, see <https://www.gnu.org/licenses/>.
 */

#include "time.h"

#define KBAR_TIME_FMT "%a %b %d - %I:%M %p %Z"
#define KBAR_TIME_MIN_SLEEP 5
#define KBAR_TIME_MAX_SLEEP 60

struct _KBarWidgetTime {
  KBarWidget parent_instance;
  guint time_timer;
};

static gboolean kbar_time_tick(void *data) {
  KBarWidgetTime *self = KBAR_WIDGET_TIME(data);
  GDateTime *g_time = g_date_time_new_now_local();
  if(!g_time) {
    g_printerr("Error getting current time\n");
    return G_SOURCE_CONTINUE;
  }
  // Compute how long to sleep before next wakeup
  gint sec = g_date_time_get_second(g_time);
  gint wait_secs = 60 - sec;
  if(wait_secs < KBAR_TIME_MIN_SLEEP) {
    wait_secs = KBAR_TIME_MIN_SLEEP;
  }
  else if(wait_secs > KBAR_TIME_MAX_SLEEP) {
    wait_secs = KBAR_TIME_MAX_SLEEP;
  }
  // Re-add timer call with new timeout
  self->time_timer = g_timeout_add_seconds((guint) wait_secs, &kbar_time_tick, data);
  // Produce new time string
  gchar *time_str = g_date_time_format(g_time, KBAR_TIME_FMT);
  if(!time_str) {
    g_printerr("Error producing time string\n");
    g_date_time_unref(g_time);
    return G_SOURCE_REMOVE;
  }
  // Set new value
  g_object_set(self, "full-text", time_str, NULL);
  // Free allocated values
  g_free(time_str);
  g_date_time_unref(g_time);
  return G_SOURCE_REMOVE;
}

static gboolean kbar_widget_time_start(KBarWidget *self, [[maybe_unused]] GError **error) {
  KBarWidgetTime *widget = KBAR_WIDGET_TIME(self);
  g_return_val_if_fail(widget->time_timer == 0, TRUE);
  widget->time_timer = 0;
  kbar_time_tick(widget);
  return TRUE;
}

static gboolean kbar_widget_time_stop(KBarWidget *self, [[maybe_unused]] GError **error) {
  KBarWidgetTime *widget = KBAR_WIDGET_TIME(self);
  gboolean ok = TRUE;
  if(widget->time_timer != 0) {
    ok &= g_source_remove(widget->time_timer);
    widget->time_timer = 0;
  }
  if(!ok) {
    // Report error
    return FALSE;
  }
  else {
    return TRUE;
  }
}

G_DEFINE_FINAL_TYPE(KBarWidgetTime, kbar_widget_time, KBAR_TYPE_WIDGET)

static void kbar_widget_time_dispose(GObject *object) {
  kbar_widget_time_stop(KBAR_WIDGET(object), NULL);
  G_OBJECT_CLASS(kbar_widget_time_parent_class)->dispose(object);
}

static void kbar_widget_time_class_init (KBarWidgetTimeClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->dispose = kbar_widget_time_dispose;
  KBarWidgetClass *widget_class = KBAR_WIDGET_CLASS(klass);
  widget_class->start = kbar_widget_time_start;
  widget_class->stop = kbar_widget_time_stop;
}

static void kbar_widget_time_init(KBarWidgetTime *self) {
  self->time_timer = 0;
}

KBarWidgetTime *kbar_widget_time_new(void) {
  return g_object_new(KBAR_TYPE_WIDGET_TIME, NULL);
}
