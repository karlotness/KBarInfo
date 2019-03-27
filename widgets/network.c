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
#include "../utils/dbus.h"
#include "../utils/debug.h"
#include "../utils/state.h"
#include "network.h"
#include <NetworkManager.h>

kbar_widget_state kbar_network_state;
static NMClient *client;
static gboolean kbar_state_error, kbar_network_error;
static guint kbar_network_state_sig, kbar_network_conn_add_sig, kbar_network_conn_remove_sig;
static NMState state;
static GString *ssid, *vpn;

static void kbar_network_nm_state_sig(GDBusProxy *proxy,
                                      GVariant *changed,
                                      GStrv *invalid,
                                      gpointer data);
static void kbar_network_nm_conn_sig(NMClient *cb_client,
                                     NMActiveConnection *active_connection,
                                     gpointer data);
static void kbar_network_update();

gboolean kbar_network_init() {
  kbar_state_error = FALSE;
  kbar_network_error = FALSE;
  ssid = g_string_new(NULL);
  vpn = g_string_new(NULL);
  client = nm_client_new(NULL, NULL);
  kbar_widget_state_init(&kbar_network_state);
  if(!client) {
    return FALSE;
  }
  state = NM_STATE_UNKNOWN;
  // Connect signals
  kbar_network_state_sig = g_signal_connect(client,
                                            "notify::state",
                                            G_CALLBACK(kbar_network_nm_state_sig),
                                            NULL);
  kbar_network_conn_add_sig = g_signal_connect(client,
                                               "active-connection-added",
                                               G_CALLBACK(kbar_network_nm_conn_sig),
                                               NULL);
  kbar_network_conn_remove_sig = g_signal_connect(client,
                                                  "active-connection-removed",
                                                  G_CALLBACK(kbar_network_nm_conn_sig),
                                                  NULL);
  // Perform initial state update
  kbar_network_nm_state_sig(NULL, NULL, NULL, NULL);
  kbar_network_nm_conn_sig(NULL, NULL, NULL);
  kbar_network_update();
  return TRUE;
}

void kbar_network_free() {
  g_signal_handler_disconnect(client, kbar_network_state_sig);
  g_signal_handler_disconnect(client, kbar_network_conn_add_sig);
  g_signal_handler_disconnect(client, kbar_network_conn_remove_sig);
  g_object_unref(client);
  g_string_free(ssid, TRUE);
  g_string_free(vpn, TRUE);
  kbar_widget_state_release(&kbar_network_state);
}

static void kbar_network_nm_state_sig(__attribute__((unused)) GDBusProxy *proxy,
                                      __attribute__((unused)) GVariant *changed,
                                      __attribute__((unused)) GStrv *invalid,
                                      __attribute__((unused)) gpointer data) {
  state = nm_client_get_state(client);
  kbar_state_error = FALSE;
  kbar_network_update();
}

static void kbar_network_nm_conn_sig(__attribute__((unused)) NMClient *cb_client,
                                     __attribute__((unused)) NMActiveConnection *active_connection,
                                     __attribute__((unused)) gpointer data) {
  const GPtrArray *connections = nm_client_get_active_connections(client);
  if(connections == NULL) {
    goto error;
  }
  vpn = g_string_set_size(vpn, 0);
  for(guint i = 0; i < connections->len; i++) {
    NMActiveConnection *conn = g_ptr_array_index(connections, i);
    if(!conn) {
      goto error;
    }
    const char *type, *name;
    type = nm_active_connection_get_connection_type(conn);
    name = nm_active_connection_get_id(conn);
    if(!type || !name) {
      goto error;
    }
    if(strncmp(type, "802-11-wireless", 15) == 0) {
      g_string_assign(ssid, name);
    }
    else if(strncmp(type, "wireguard", 9) == 0 || strncmp(type, "vpn", 3) == 0) {
      if(vpn->len > 0){
        g_string_append(vpn, ", ");
      }
      g_string_append(vpn, name);
    }
    else {
      continue;
    }
  }
  kbar_network_error = FALSE;
  kbar_network_update();
  return;
 error:
  kbar_network_error = TRUE;
  kbar_network_update();
  return;
}

static void kbar_network_update() {
  kbar_network_state.urgent = kbar_network_error || kbar_state_error;
  if(kbar_network_error) {
    g_string_assign(kbar_network_state.text, "N: err");
    kbar_print_bar_state();
    return;
  }
  gchar *c_str;
  if(ssid->len == 0) {
    g_string_assign(ssid, "--");
  }
  if(state != NM_STATE_CONNECTED_GLOBAL && state != NM_STATE_CONNECTED_SITE && state != NM_STATE_CONNECTED_LOCAL) {
    c_str = "~ ";
  }
  else {
    c_str = " ";
  }
  if(g_strcmp0(vpn->str, "") != 0 && vpn->len > 0) {
    // VPN is connected
    g_string_printf(kbar_network_state.text, "N:%s%s (%s)", c_str, ssid->str, vpn->str);
  }
  else {
    g_string_printf(kbar_network_state.text, "N:%s%s", c_str, ssid->str);
  }
  kbar_print_bar_state();
}
