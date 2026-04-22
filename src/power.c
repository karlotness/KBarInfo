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
#include <upower.h>

static const double kbar_battery_critical_pct = 15.0;

struct _KBarWidgetPower {
  KBarWidget parent_instance;
  UpClient *client;
  UpDevice *display_device;
  gulong percentage_signal, state_signal;
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
  self->client = NULL;
  self->display_device = NULL;
  self->percentage_signal = 0;
  self->state_signal = 0;
}

static void kbar_power_signal([[maybe_unused]] GObject *self, [[maybe_unused]] GParamSpec *pspec, gpointer data) {
  kbar_power_update(KBAR_WIDGET_POWER(data));
}

static void kbar_widget_power_proxy_cb([[maybe_unused]] GObject *source_object, GAsyncResult *res, gpointer data) {
  KBarWidgetPower *widget = KBAR_WIDGET_POWER(data);
  UpClient *client = up_client_new_finish(res, NULL);
  if(!client) {
    return;
  }
  if(widget->client) {
    g_object_unref(client);
    return;
  }
  widget->client = client;
  widget->display_device = up_client_get_display_device(widget->client);
  // Connect signals
  widget->percentage_signal = g_signal_connect_after(widget->display_device, "notify::percentage", G_CALLBACK(kbar_power_signal), widget);
  widget->state_signal = g_signal_connect_after(widget->display_device, "notify::state", G_CALLBACK(kbar_power_signal), widget);
  kbar_power_update(widget);
}

static gboolean kbar_widget_power_start(KBarWidget *self, [[maybe_unused]] GError **error) {
  up_client_new_async(NULL, kbar_widget_power_proxy_cb, self);
  return TRUE;
}

static gboolean kbar_widget_power_stop(KBarWidget *self, [[maybe_unused]] GError **error) {
  KBarWidgetPower *widget = KBAR_WIDGET_POWER(self);
  g_clear_signal_handler(&widget->state_signal, G_OBJECT(widget->display_device));
  g_clear_signal_handler(&widget->percentage_signal, G_OBJECT(widget->display_device));
  g_clear_object(&widget->display_device);
  g_clear_object(&widget->client);
  return TRUE;
}

KBarWidgetPower *kbar_widget_power_new(void) {
  return g_object_new(KBAR_TYPE_WIDGET_POWER, NULL);
}

static void kbar_power_update(KBarWidgetPower *self) {
  // Get current status values
  double pct = 0;
  guint state_val = 0;
  g_object_get(self->display_device, "percentage", &pct, "state", &state_val, NULL);
  const gchar *state_str = "";
  switch(state_val) {
  case UP_DEVICE_STATE_CHARGING:
    state_str = "C ";
    break;
  case UP_DEVICE_STATE_FULLY_CHARGED:
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
