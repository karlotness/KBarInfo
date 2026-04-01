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

#include <string.h>
#include <NetworkManager.h>
#include "network.h"

struct _KBarWidgetNetwork {
  KBarWidget parent_instance;
  NMClient *client;
  NMState state;
  GString *ssid, *vpn;
  gboolean state_error, network_error;
  guint state_sig, conn_add_sig, conn_remove_sig;
};

G_DEFINE_FINAL_TYPE(KBarWidgetNetwork, kbar_widget_network, KBAR_TYPE_WIDGET)

static void kbar_network_update(KBarWidgetNetwork *widget);
static gboolean kbar_widget_network_start(KBarWidget *self, [[maybe_unused]] GError **error);
static gboolean kbar_widget_network_stop(KBarWidget *self, [[maybe_unused]] GError **error);
static void kbar_network_nm_state_cb(GDBusProxy *proxy, GVariant *changed, GStrv *invalid, gpointer userdata);
static void kbar_network_nm_conn_cb(NMClient *cb_client, NMActiveConnection *active_connection, gpointer userdata);

static void kbar_widget_network_dispose(GObject *object) {
  kbar_widget_network_stop(KBAR_WIDGET(object), NULL);
  G_OBJECT_CLASS(kbar_widget_network_parent_class)->dispose(object);
}

static void kbar_widget_network_finalize(GObject *object) {
  KBarWidgetNetwork *self = KBAR_WIDGET_NETWORK(object);
  g_string_free(self->ssid, TRUE);
  g_string_free(self->vpn, TRUE);
  self->ssid = NULL;
  self->vpn = NULL;
  G_OBJECT_CLASS(kbar_widget_network_parent_class)->finalize(object);
}

static void kbar_widget_network_class_init (KBarWidgetNetworkClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->dispose = kbar_widget_network_dispose;
  object_class->finalize = kbar_widget_network_finalize;
  KBarWidgetClass *widget_class = KBAR_WIDGET_CLASS(klass);
  widget_class->start = kbar_widget_network_start;
  widget_class->stop = kbar_widget_network_stop;
}

static void kbar_widget_network_init(KBarWidgetNetwork *self) {
  self->client = NULL;
  self->state = NM_STATE_UNKNOWN;
  self->ssid = g_string_new(NULL);
  self->vpn = g_string_new(NULL);
  self->state_error = FALSE;
  self->network_error = FALSE;
  self->state_sig = 0;
  self->conn_add_sig = 0;
  self->conn_remove_sig = 0;
}

KBarWidgetNetwork *kbar_widget_network_new(void) {
  return g_object_new(KBAR_TYPE_WIDGET_NETWORK, NULL);
}

static gboolean kbar_widget_network_start(KBarWidget *self, [[maybe_unused]] GError **error) {
  KBarWidgetNetwork *widget = KBAR_WIDGET_NETWORK(self);
  widget->client = nm_client_new(NULL, NULL);
  widget->state = NM_STATE_UNKNOWN;
  widget->ssid = g_string_set_size(widget->ssid, 0);
  widget->vpn = g_string_set_size(widget->vpn, 0);
  widget->state_error = FALSE;
  widget->network_error = FALSE;
  if(!widget->client) {
    return FALSE;
  }
  // Connect signals
  widget->state_sig = g_signal_connect_after(widget->client, "notify::state", G_CALLBACK(kbar_network_nm_state_cb), self);
  widget->conn_add_sig = g_signal_connect_after(widget->client, "active-connection-added", G_CALLBACK(kbar_network_nm_conn_cb), self);
  widget->conn_remove_sig = g_signal_connect_after(widget->client, "active-connection-removed", G_CALLBACK(kbar_network_nm_conn_cb), self);
  // Perform initial updates
  kbar_network_nm_state_cb(NULL, NULL, NULL, self);
  kbar_network_nm_conn_cb(NULL, NULL, self);
  return TRUE;
}

static gboolean kbar_widget_network_stop(KBarWidget *self, [[maybe_unused]] GError **error) {
  KBarWidgetNetwork *widget = KBAR_WIDGET_NETWORK(self);
  if(widget->state_sig != 0) {
    g_signal_handler_disconnect(widget->client, widget->state_sig);
    widget->state_sig = 0;
  }
  if(widget->conn_add_sig != 0) {
    g_signal_handler_disconnect(widget->client, widget->conn_add_sig);
    widget->conn_add_sig = 0;
  }
  if(widget->conn_remove_sig != 0) {
    g_signal_handler_disconnect(widget->client, widget->conn_remove_sig);
    widget->conn_remove_sig = 0;
  }
  if(widget->client) {
    g_object_unref(widget->client);
    widget->client = NULL;
  }
  return TRUE;
}

static void kbar_network_nm_state_cb([[maybe_unused]] GDBusProxy *proxy, [[maybe_unused]] GVariant *changed, [[maybe_unused]] GStrv *invalid, gpointer userdata) {
  KBarWidgetNetwork *widget = KBAR_WIDGET_NETWORK(userdata);
  widget->state = nm_client_get_state(widget->client);
  widget->state_error = FALSE;
  kbar_network_update(widget);
}

static void kbar_network_nm_conn_cb([[maybe_unused]] NMClient *cb_client, [[maybe_unused]] NMActiveConnection *active_connection, gpointer userdata) {
  KBarWidgetNetwork *widget = KBAR_WIDGET_NETWORK(userdata);
  const GPtrArray *connections = nm_client_get_active_connections(widget->client);
  if(!connections) {
    goto error;
  }
  widget->vpn = g_string_set_size(widget->vpn, 0);
  widget->ssid = g_string_set_size(widget->ssid, 0);
  for(guint i = 0; i < connections->len; i++) {
    NMActiveConnection *conn = g_ptr_array_index(connections, i);
    if(!conn) {
      goto error;
    }
    const char *type = nm_active_connection_get_connection_type(conn);
    const char *name = nm_active_connection_get_id(conn);
    if(!type || !name) {
      goto error;
    }
    if(strncmp(type, "802-11-wireless", 15) == 0 || strncmp(type, "802-3-ethernet", 14) == 0) {
      if(widget->ssid->len > 0) {
        widget->ssid = g_string_append(widget->ssid, ", ");
      }
      widget->ssid = g_string_append(widget->ssid, name);
    }
    else if(strncmp(type, "wireguard", 9) == 0 || strncmp(type, "vpn", 3) == 0) {
      if(widget->vpn->len > 0){
        widget->vpn = g_string_append(widget->vpn, ", ");
      }
      widget->vpn = g_string_append(widget->vpn, name);
    }
  }
  widget->network_error = FALSE;
  kbar_network_update(widget);
  return;
 error:
  widget->network_error = TRUE;
  kbar_network_update(widget);
}

static void kbar_network_update(KBarWidgetNetwork *widget) {
  const gboolean error = widget->state_error || widget->network_error;
  gchar *dynamic_text = NULL;
  if(!error) {
    if(widget->ssid->len == 0) {
      widget->ssid = g_string_assign(widget->ssid, "--");
    }
    const gchar *c_str = "";
    if(widget->state != NM_STATE_CONNECTED_GLOBAL && widget->state != NM_STATE_CONNECTED_SITE && widget->state != NM_STATE_CONNECTED_LOCAL) {
      c_str = "~ ";
    }
    else {
      c_str = " ";
    }
    if(widget->vpn->len > 0) {
      // VPN is connected
      dynamic_text = g_strdup_printf("N:%s%s (%s)", c_str, widget->ssid->str, widget->vpn->str);
    }
    else {
      dynamic_text = g_strdup_printf("N:%s%s", c_str, widget->ssid->str);
    }
  }
  g_object_set(widget, "full-text", (dynamic_text == NULL ? "N: err" : dynamic_text), "urgent", error, NULL);
  g_free(dynamic_text);
}
