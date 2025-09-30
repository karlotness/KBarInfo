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
  gdouble vol_pct;
};

G_DEFINE_FINAL_TYPE(KBarWidgetVolume, kbar_widget_volume, KBAR_TYPE_WIDGET)

static void kbar_volume_update(KBarWidgetVolume *widget);
static void kbar_volume_connection_cb(pa_context *c, void *userdata);
static void kbar_volume_event_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata);
static void kbar_volume_server_info_cb(pa_context *c, const pa_server_info *i, void *userdata);
static void kbar_volume_sink_info_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata);
static void kbar_volume_generic_success_cb(pa_context *c, int success, void *userdata);

static void kbar_widget_volume_finalize(GObject *object) {
  KBarWidgetVolume *self = KBAR_WIDGET_VOLUME(object);
  pa_context_disconnect(self->pa_ctx);
  pa_glib_mainloop_free(self->pa_main);
  self->pa_main = NULL;
  pa_context_unref(self->pa_ctx);
  self->pa_ctx = NULL;
  G_OBJECT_CLASS(kbar_widget_volume_parent_class)->finalize(object);
}

static void kbar_widget_volume_class_init (KBarWidgetVolumeClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = kbar_widget_volume_finalize;
}

static void kbar_widget_volume_init(KBarWidgetVolume *self) {
  self->error = FALSE;
  self->mute = FALSE;
  self->vol_pct = 0;
  self->pa_main = pa_glib_mainloop_new(NULL);
  self->pa_ctx = pa_context_new(pa_glib_mainloop_get_api(self->pa_main), "kbarinfo");
  pa_context_set_state_callback(self->pa_ctx, &kbar_volume_connection_cb, self);
  pa_context_set_subscribe_callback(self->pa_ctx, &kbar_volume_event_cb, self);
  pa_context_connect(self->pa_ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);
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
    kbar_volume_event_cb(c, PA_SUBSCRIPTION_EVENT_SINK | PA_SUBSCRIPTION_EVENT_CHANGE, 0, userdata);
  }
  else if(state != PA_CONTEXT_TERMINATED) {
    widget->error = TRUE;
    kbar_volume_update(widget);
  }
}

static void kbar_volume_event_cb(pa_context *c, pa_subscription_event_type_t t, [[maybe_unused]] uint32_t idx, void *userdata) {
  const pa_subscription_event_type_t event_source = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
  const pa_subscription_event_type_t event_type = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
  if(event_source != PA_SUBSCRIPTION_EVENT_SINK && event_source != PA_SUBSCRIPTION_EVENT_SERVER) {
    // Neither sink nor server event. Not interested
    return;
  }
  if(event_type != PA_SUBSCRIPTION_EVENT_CHANGE) {
    // Not a change event. Not interested
    return;
  }
  pa_operation *op = pa_context_get_server_info(c, &kbar_volume_server_info_cb, userdata);
  pa_operation_unref(op);
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
  return ((gdouble) vol  * 100) / PA_VOLUME_NORM;
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
  widget->vol_pct = kbar_volume_to_percent(pa_cvolume_avg(&(i->volume)));
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
  gboolean urgent = widget->error;
  gboolean dynamic_text = FALSE;
  gchar *text = "";
  if(widget->error) {
    text = "V: err";
    dynamic_text = FALSE;
  }
  else if(widget->mute) {
    text = "V: M";
    dynamic_text = FALSE;
  }
  else {
    text = g_strdup_printf("V: %0.0f%%", widget->vol_pct);
    dynamic_text = TRUE;
  }
  g_object_set(widget, "full-text", text, "urgent", urgent, NULL);
  if(dynamic_text) {
    g_free(text);
  }
}
