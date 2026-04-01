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

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include "volume.h"

struct _KBarWidgetVolume {
  KBarWidget parent_instance;
  pa_glib_mainloop *pa_main;
  pa_context *pa_ctx;
  gboolean error, mute;
  pa_volume_t vol_raw;
  uint32_t default_sink_idx;
  // Tracking previous values
  pa_volume_t prev_volume;
  gboolean prev_error, prev_mute, ever_set;
};

G_DEFINE_FINAL_TYPE(KBarWidgetVolume, kbar_widget_volume, KBAR_TYPE_WIDGET)

static void kbar_volume_update(KBarWidgetVolume *widget);
static void kbar_volume_connection_cb(pa_context *c, void *userdata);
static void kbar_volume_event_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata);
static void kbar_volume_server_info_cb(pa_context *c, const pa_server_info *i, void *userdata);
static void kbar_volume_sink_info_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata);
static void kbar_volume_generic_success_cb(pa_context *c, int success, void *userdata);
static gboolean kbar_widget_volume_start(KBarWidget *self, [[maybe_unused]] GError **error);
static gboolean kbar_widget_volume_stop(KBarWidget *self, [[maybe_unused]] GError **error);

static void kbar_widget_volume_dispose(GObject *object) {
  kbar_widget_volume_stop(KBAR_WIDGET(object), NULL);
  G_OBJECT_CLASS(kbar_widget_volume_parent_class)->dispose(object);
}

static void kbar_widget_volume_class_init (KBarWidgetVolumeClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->dispose = kbar_widget_volume_dispose;
  KBarWidgetClass *widget_class = KBAR_WIDGET_CLASS(klass);
  widget_class->start = kbar_widget_volume_start;
  widget_class->stop = kbar_widget_volume_stop;
}

static void kbar_widget_volume_init(KBarWidgetVolume *self) {
  self->error = FALSE;
  self->mute = FALSE;
  self->vol_raw = 0;
  self->default_sink_idx = PA_INVALID_INDEX;
  self->pa_main = NULL;
  self->pa_ctx = NULL;
  self->prev_volume = 0;
  self->prev_mute = FALSE;
  self->prev_error = FALSE;
  self->ever_set = FALSE;
}

KBarWidgetVolume *kbar_widget_volume_new(void) {
  return g_object_new(KBAR_TYPE_WIDGET_VOLUME, NULL);
}

static void kbar_volume_connection_cb(pa_context *c, void *userdata) {
  KBarWidgetVolume *widget = KBAR_WIDGET_VOLUME(userdata);
  pa_context_state_t state = pa_context_get_state(c);
  if(state == PA_CONTEXT_READY) {
    // Connection established. Subscribe to events
    pa_operation *op = pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SERVER, &kbar_volume_generic_success_cb, userdata);
    pa_operation_unref(op);
    // Get initial volume values (so we don't wait for change)
    kbar_volume_event_cb(c, PA_SUBSCRIPTION_EVENT_SERVER | PA_SUBSCRIPTION_EVENT_CHANGE, PA_INVALID_INDEX, userdata);
  }
  else if(state != PA_CONTEXT_TERMINATED) {
    widget->error = TRUE;
    kbar_volume_update(widget);
  }
}

static void kbar_volume_event_cb(pa_context *c, pa_subscription_event_type_t t, [[maybe_unused]] uint32_t idx, void *userdata) {
  const pa_subscription_event_type_t event_source = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
  const pa_subscription_event_type_t event_type = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
  KBarWidgetVolume *widget = KBAR_WIDGET_VOLUME(userdata);

  // Process events
  if(event_source == PA_SUBSCRIPTION_EVENT_SERVER && event_type == PA_SUBSCRIPTION_EVENT_CHANGE) {
    // The default sink may have changed. Query and check
    widget->default_sink_idx = PA_INVALID_INDEX;
    pa_operation *op = pa_context_get_server_info(c, &kbar_volume_server_info_cb, userdata);
    pa_operation_unref(op);
  }
  else if (event_source == PA_SUBSCRIPTION_EVENT_SINK &&
           event_type == PA_SUBSCRIPTION_EVENT_REMOVE &&
           idx == widget->default_sink_idx) {
    // The default sink was removed, reset and wait for server update
    widget->default_sink_idx = PA_INVALID_INDEX;
  }
  else if(event_source == PA_SUBSCRIPTION_EVENT_SINK &&
          event_type == PA_SUBSCRIPTION_EVENT_CHANGE &&
          widget->default_sink_idx != PA_INVALID_INDEX &&
          idx == widget->default_sink_idx) {
    // The default sink changed, query new volume level
    pa_operation *op = pa_context_get_sink_info_by_index(c, widget->default_sink_idx, &kbar_volume_sink_info_cb, userdata);
    pa_operation_unref(op);
  }
}

static void kbar_volume_server_info_cb(pa_context *c, const pa_server_info *i, void *userdata) {
  KBarWidgetVolume *widget = KBAR_WIDGET_VOLUME(userdata);
  if(!i) {
    widget->error = TRUE;
    kbar_volume_update(widget);
    return;
  }
  // Got default sink name. Query for volume
  pa_operation *op = pa_context_get_sink_info_by_name(c, i->default_sink_name, &kbar_volume_sink_info_cb, userdata);
  pa_operation_unref(op);
}

static gdouble kbar_volume_to_percent(pa_volume_t vol) {
  // Conversion logic from pavucontrol
  // https://github.com/pulseaudio/pavucontrol/blob/574139c10e70b63874bcb75fe4cdfd1f4644ad68/src/channelwidget.cc
  return ((gdouble) vol * 100) / PA_VOLUME_NORM;
}

static void kbar_volume_sink_info_cb([[maybe_unused]] pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
  // Got current volume state.
  KBarWidgetVolume *widget = KBAR_WIDGET_VOLUME(userdata);
  if(eol > 0) {
    return;
  }
  if(!i) {
    widget->error = TRUE;
    kbar_volume_update(widget);
    return;
  }
  widget->default_sink_idx = i->index;
  widget->vol_raw = pa_cvolume_avg(&(i->volume));
  widget->mute = i->mute;
  widget->error = FALSE;
  kbar_volume_update(widget);
}

static void kbar_volume_generic_success_cb([[maybe_unused]] pa_context *c, int success, void *userdata) {
  KBarWidgetVolume *widget = KBAR_WIDGET_VOLUME(userdata);
  if(!success) {
    widget->error = TRUE;
    kbar_volume_update(widget);
  }
}

static void kbar_volume_update(KBarWidgetVolume *widget) {
  // Check if meaningful updates have happened
  if(widget->ever_set &&
     (widget->mute == widget->prev_mute) &&
     (widget->error == widget->prev_error) &&
     (widget->vol_raw == widget->prev_volume)) {
    // Reported state has not changed
    return;
  }
  // Store current state values
  widget->prev_volume = widget->vol_raw;
  widget->prev_error = widget->error;
  widget->prev_mute = widget->mute;
  widget->ever_set = TRUE;
  // Build update string
  const gboolean urgent = widget->error;
  const gchar *text = "";
  gchar *dynamic_text = NULL;
  if(widget->error) {
    text = "V: err";
  }
  else if(widget->mute) {
    text = "V: M";
  }
  else {
    const gdouble vol_pct = kbar_volume_to_percent(widget->vol_raw);
    dynamic_text = g_strdup_printf("V: %0.0f%%", vol_pct);
  }
  g_object_set(widget, "full-text", (dynamic_text == NULL ? text : dynamic_text), "urgent", urgent, NULL);
  g_free(dynamic_text);
}

static gboolean kbar_widget_volume_start(KBarWidget *self, [[maybe_unused]] GError **error) {
  KBarWidgetVolume *widget = KBAR_WIDGET_VOLUME(self);
  widget->default_sink_idx = PA_INVALID_INDEX;
  widget->ever_set = FALSE;
  g_return_val_if_fail(widget->pa_main == NULL, TRUE);
  g_return_val_if_fail(widget->pa_ctx == NULL, TRUE);
  widget->pa_main = pa_glib_mainloop_new(NULL);
  widget->pa_ctx = pa_context_new(pa_glib_mainloop_get_api(widget->pa_main), "kbarinfo");
  pa_context_set_state_callback(widget->pa_ctx, &kbar_volume_connection_cb, self);
  pa_context_set_subscribe_callback(widget->pa_ctx, &kbar_volume_event_cb, self);
  pa_context_connect(widget->pa_ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);
  return TRUE;
}

static gboolean kbar_widget_volume_stop(KBarWidget *self, [[maybe_unused]] GError **error) {
  KBarWidgetVolume *widget = KBAR_WIDGET_VOLUME(self);
  if(widget->pa_ctx) {
    pa_context_disconnect(widget->pa_ctx);
  }
  if(widget->pa_main) {
    pa_glib_mainloop_free(widget->pa_main);
  }
  if(widget->pa_ctx) {
    pa_context_unref(widget->pa_ctx);
  }
  widget->pa_main = NULL;
  widget->pa_ctx = NULL;
  return TRUE;
}
