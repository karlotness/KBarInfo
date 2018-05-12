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
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include "volume.h"
#include "../utils/state.h"

kbar_widget_state kbar_volume_state;

pa_glib_mainloop *pa_main;
gboolean kbar_volume_error, kbar_volume_mute;
gdouble kbar_volume_percent;
pa_context *pa_ctx;

static void kbar_volume_connection_cb(pa_context *c, void *userdata);
static void kbar_volume_event_cb(pa_context *c,
                                 pa_subscription_event_type_t t,
                                 uint32_t idx, void *userdata);
static void kbar_volume_server_info_cb(pa_context *c,
                                       const pa_server_info *i,
                                       void *userdata);
static void kbar_volume_sink_info_cb(pa_context *c,
                                     const pa_sink_info *i,
                                     int eol, void *userdata);
static gdouble kbar_volume_to_percent(pa_volume_t vol);
static void kbar_volume_update();

gboolean kbar_volume_init() {
  kbar_volume_error = FALSE;
  kbar_widget_state_init(&kbar_volume_state);
  pa_main = pa_glib_mainloop_new(NULL);
  pa_ctx = pa_context_new(pa_glib_mainloop_get_api(pa_main), "kbarinfo");
  pa_context_set_state_callback(pa_ctx, &kbar_volume_connection_cb,
                                NULL);
  pa_context_set_subscribe_callback(pa_ctx, &kbar_volume_event_cb, NULL);
  pa_context_connect(pa_ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);
  return TRUE;
}

void kbar_volume_free() {
  pa_context_disconnect(pa_ctx);
  pa_glib_mainloop_free(pa_main);
  pa_context_unref(pa_ctx);
  kbar_widget_state_release(&kbar_volume_state);
}

static void kbar_volume_connection_cb(pa_context *c, void *userdata) {
  pa_context_state_t state = pa_context_get_state(c);
  if(state == PA_CONTEXT_READY) {
    // Connection established. Subscribe to events
    kbar_volume_error = FALSE;
    pa_operation *op = pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK,
                                            NULL, NULL);
    pa_operation_unref(op);
    // Get initial volume values (so we don't wait for change)
    kbar_volume_event_cb(c,
                         PA_SUBSCRIPTION_EVENT_SINK |
                         PA_SUBSCRIPTION_EVENT_CHANGE,
                         0, NULL);
  }
  else if(state != PA_CONTEXT_TERMINATED) {
    kbar_volume_error = TRUE;
    kbar_volume_update();
  }
}

static void kbar_volume_event_cb(pa_context *c,
                                 pa_subscription_event_type_t t,
                                 uint32_t idx, void *userdata) {
  // A Sink event was fired. Get default sink name.
  pa_subscription_event_type_t ev_mask = PA_SUBSCRIPTION_EVENT_SINK |
    PA_SUBSCRIPTION_EVENT_CHANGE;
  if((t & ev_mask) != ev_mask) {
    // We are not interested in this event.
    return;
  }
  pa_operation *op =
    pa_context_get_server_info(c, &kbar_volume_server_info_cb, NULL);
  pa_operation_unref(op);
}

static void kbar_volume_server_info_cb(pa_context *c,
                                       const pa_server_info *i,
                                       void *userdata) {
  // Got default sink name. Query for volume
  const char* default_sink = i->default_sink_name;
  pa_operation *op =
    pa_context_get_sink_info_by_name(c, default_sink,
                                     &kbar_volume_sink_info_cb, NULL);
  pa_operation_unref(op);
}

static void kbar_volume_sink_info_cb(pa_context *c,
                                     const pa_sink_info *i,
                                     int eol, void *userdata) {
  // Got current volume state.
  if(eol > 0) {
    return;
  }
  kbar_volume_percent =
    kbar_volume_to_percent(pa_cvolume_avg(&(i->volume)));
  kbar_volume_mute = i->mute;
  kbar_volume_error = FALSE;
  kbar_volume_update();
}

static gdouble kbar_volume_to_percent(pa_volume_t vol) {
  // Conversion logic from pavucontrol
  // https://github.com/pulseaudio/pavucontrol/blob/574139c10e70b63874bcb75fe4cdfd1f4644ad68/src/channelwidget.cc
  return ((gdouble) vol  * 100) / PA_VOLUME_NORM;
}

static void kbar_volume_update() {
  kbar_volume_state.urgent = kbar_volume_error;
  if(kbar_volume_error) {
    g_string_assign(kbar_volume_state.text, "V: err");
  }
  else if (kbar_volume_mute) {
    g_string_assign(kbar_volume_state.text, "V: M");
  }
  else {
    g_string_printf(kbar_volume_state.text, "V: %0.0f%%",
                    kbar_volume_percent);
  }
  kbar_print_bar_state();
}
