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
#include "../utils/state.h"
#include "../utils/dbus.h"
#include "../utils/debug.h"
#include "power.h"

#define BATTERY_DBUS_NAME "org.freedesktop.UPower"
#define BATTERY_INTERFACE "org.freedesktop.UPower.Device"
#define BATTERY_PATH "/org/freedesktop/UPower/devices/battery_BAT0"
#define CRITICAL_PERCENTAGE 15

kbar_widget_state kbar_power_state;
gboolean kbar_power_error;
gulong kbar_power_sig;
GDBusProxy *batt_obj;

static void kbar_power_update();
static void kbar_power_signal(GDBusProxy *proxy, GVariant *changed,
                              GStrv *invalid, gpointer data);

gboolean kbar_power_init() {
  kbar_power_error = FALSE;
  GError *err = NULL;
  kbar_widget_state_init(&kbar_power_state);
  batt_obj = g_dbus_proxy_new_sync(kbar_system_bus,
                                   G_DBUS_PROXY_FLAGS_NONE,
                                   NULL,
                                   BATTERY_DBUS_NAME,
                                   BATTERY_PATH,
                                   BATTERY_INTERFACE,
                                   NULL,
                                   &err);
  if(err != NULL) {
    kbar_err_printf("DBus power error: %s\n", err->message);
    kbar_power_error = TRUE;
    goto exit;
  }
  kbar_power_sig = g_signal_connect_after(batt_obj,
                                          "g-properties-changed",
                                          G_CALLBACK(&kbar_power_signal),
                                          NULL);
  if(kbar_power_sig == 0) {
    kbar_err_printf("Power: Error attaching signal\n");
    kbar_power_error = TRUE;
    goto exit;
  }
 exit:
  kbar_power_update();
  if(err != NULL) {
    g_error_free(err);
  }
  return !kbar_power_error;
}

void kbar_power_free() {
  g_signal_handler_disconnect(batt_obj, kbar_power_sig);
  g_object_unref(batt_obj);
  kbar_widget_state_release(&kbar_power_state);
}

static void kbar_power_update() {
  if(kbar_power_error) {
    kbar_power_state.urgent = TRUE;
    g_string_assign(kbar_power_state.text, "P: err");
    kbar_print_bar_state();
    return;
  }
  GVariant *percent = g_dbus_proxy_get_cached_property(batt_obj,
                                                       "Percentage");
  GVariant *state = g_dbus_proxy_get_cached_property(batt_obj,
                                                     "State");
  gint pct = g_variant_get_double(percent);
  guint32 state_val = g_variant_get_uint32(state);
  g_variant_unref(percent);
  g_variant_unref(state);
  // Build the status string
  gchar *state_str;
  switch(state_val) {
  case 1:
    state_str = "C ";
    break;
  case 4:
    state_str = "F ";
    break;
  default:
    state_str = " ";
  }
  kbar_power_state.urgent = pct <= CRITICAL_PERCENTAGE;
  g_string_printf(kbar_power_state.text, "P:%s%i%%", state_str, pct);
  kbar_print_bar_state();
}

static void kbar_power_signal(GDBusProxy *proxy, GVariant *changed,
                              GStrv *invalid, gpointer data) {
  kbar_power_update();
}
