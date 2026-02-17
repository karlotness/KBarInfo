/*
 * Copyright 2026 Karl Otness
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

#include "power.h"

static const char *const kbar_battery_dbus_name = "org.freedesktop.UPower";
static const char *const kbar_battery_interface = "org.freedesktop.UPower.Device";
static const char *const kbar_battery_path = "/org/freedesktop/UPower/devices/DisplayDevice";
static const double kbar_battery_critical_pct = 15.0;

struct _KBarWidgetPower {
  KBarWidget parent_instance;
  GDBusProxy *batt_obj;
  gulong property_signal;
};

G_DEFINE_FINAL_TYPE(KBarWidgetPower, kbar_widget_power, KBAR_TYPE_WIDGET)

static gboolean kbar_widget_power_start(KBarWidget *self, [[maybe_unused]] GError **error);
static gboolean kbar_widget_power_stop(KBarWidget *self, [[maybe_unused]] GError **error);
static void kbar_power_update(KBarWidgetPower *self);

static void kbar_widget_power_dispose(GObject *object) {
  kbar_widget_power_stop(KBAR_WIDGET(object), NULL);
  G_OBJECT_CLASS(kbar_widget_power_parent_class)->dispose(object);
}

static void kbar_widget_power_class_init (KBarWidgetPowerClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->dispose = kbar_widget_power_dispose;
  KBarWidgetClass *widget_class = KBAR_WIDGET_CLASS(klass);
  widget_class->start = kbar_widget_power_start;
  widget_class->stop = kbar_widget_power_stop;
}

static void kbar_widget_power_init(KBarWidgetPower *self) {
  self->batt_obj = NULL;
  self->property_signal = 0;
}

static void kbar_power_signal([[maybe_unused]] GDBusProxy *proxy, [[maybe_unused]] GVariant *changed, [[maybe_unused]] GStrv *invalid, gpointer data) {
  kbar_power_update(KBAR_WIDGET_POWER(data));
}

static void kbar_widget_power_proxy_cb([[maybe_unused]] GObject *source_object, GAsyncResult *res, gpointer data) {
  KBarWidgetPower *widget = KBAR_WIDGET_POWER(data);
  GDBusProxy *proxy = g_dbus_proxy_new_for_bus_finish(res, NULL);
  if(widget->batt_obj) {
    g_object_unref(proxy);
    return;
  }
  widget->batt_obj = proxy;
  // Connect signals
  widget->property_signal = g_signal_connect_after(widget->batt_obj, "g-properties-changed", G_CALLBACK(kbar_power_signal), NULL);
  kbar_power_update(widget);
}

static gboolean kbar_widget_power_start(KBarWidget *self, [[maybe_unused]] GError **error) {
  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, NULL, kbar_battery_dbus_name, kbar_battery_path, kbar_battery_interface, NULL, kbar_widget_power_proxy_cb, self);
  return TRUE;
}

static gboolean kbar_widget_power_stop(KBarWidget *self, [[maybe_unused]] GError **error) {
  KBarWidgetPower *widget = KBAR_WIDGET_POWER(self);
  if(widget->batt_obj && widget->property_signal != 0u) {
    g_signal_handler_disconnect(widget->batt_obj, widget->property_signal);
    widget->property_signal = 0;
  }
  if(widget->batt_obj) {
    g_object_unref(widget->batt_obj);
    widget->batt_obj = NULL;
  }
  return TRUE;
}

KBarWidgetPower *kbar_widget_power_new(void) {
  return g_object_new(KBAR_TYPE_WIDGET_POWER, NULL);
}

static void kbar_power_update(KBarWidgetPower *self) {
  // Get current status values
  GVariant *percent = g_dbus_proxy_get_cached_property(self->batt_obj, "Percentage");
  GVariant *state = g_dbus_proxy_get_cached_property(self->batt_obj, "State");
  double pct = g_variant_get_double(percent);
  guint32 state_val = g_variant_get_uint32(state);
  g_variant_unref(percent);
  g_variant_unref(state);
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
  gboolean urgent = pct <= kbar_battery_critical_pct;
  gchar *text = g_strdup_printf("P:%s%.0f%%", state_str, pct);
  g_object_set(self, "full-text", text, "urgent", urgent, NULL);
  g_free(text);
}
