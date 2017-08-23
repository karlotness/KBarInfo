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
#include <NetworkManager.h>
#include "../utils/debug.h"
#include "../utils/state.h"
#include "network.h"

kbar_widget_state kbar_network_state;
static NMClient *kbar_nm_client;

typedef enum kbar_net_state_value {
  CONNECTED, CONNECTING, DISCONNECTED
} kbar_net_state_value;

static void kbar_net_conn_update(NMClient *client,
                                 GParamSpec *pspec,
                                 gpointer user_data);
static void kbar_net_state_update(NMClient *client,
                                  GParamSpec *pspec,
                                  gpointer user_data);
static void kbar_net_write_state();
static gulong global_state_notify, global_primary_conn_notify;
static GString *network;
static kbar_net_state_value kbar_net_state;

gboolean kbar_network_init() {
  kbar_widget_state_init(&kbar_network_state);
  kbar_nm_client = nm_client_new(NULL, NULL);
  global_primary_conn_notify =
    g_signal_connect(kbar_nm_client, "notify::primary-connection",
                     G_CALLBACK(&kbar_net_conn_update), NULL);
  global_state_notify =
    g_signal_connect(kbar_nm_client, "notify::state",
                     G_CALLBACK(&kbar_net_state_update), NULL);
  network = g_string_new("");
  kbar_net_state = DISCONNECTED;
  kbar_net_conn_update(kbar_nm_client, NULL, NULL);
  kbar_net_state_update(kbar_nm_client, NULL, NULL);
  return TRUE;
}

void kbar_network_free() {
  g_signal_handler_disconnect(kbar_nm_client,
                              global_state_notify);
  g_signal_handler_disconnect(kbar_nm_client,
                              global_primary_conn_notify);
  g_object_unref(kbar_nm_client);
  kbar_widget_state_release(&kbar_network_state);
}

static void kbar_net_conn_update(NMClient *client,
                                 GParamSpec *pspec,
                                 gpointer user_data) {
  NMActiveConnection *conn = nm_client_get_primary_connection (client);
  if(conn) {
    const gchar* id = nm_active_connection_get_id(conn);
    g_string_assign(network, id);
  }
  else {
    g_string_assign(network, "");
  }
}

static void kbar_net_state_update(NMClient *client,
                                  GParamSpec *pspec,
                                  gpointer user_data) {
  // State updated
  NMState state = nm_client_get_state(client);
  switch(state) {
  case NM_STATE_CONNECTED_LOCAL:
  case NM_STATE_CONNECTED_SITE:
  case NM_STATE_CONNECTED_GLOBAL:
    kbar_net_state = CONNECTED;
    break;
  case NM_STATE_CONNECTING:
    kbar_net_state = CONNECTING;
    break;
  default:
    kbar_net_state = DISCONNECTED;
  }
  kbar_net_write_state();
}

static void kbar_net_write_state() {
  gchar *status_str = NULL;
  switch(kbar_net_state) {
  case CONNECTED:
    status_str = "";
    break;
  case CONNECTING:
    status_str = "~";
    break;
  default:
    status_str = "--";
  }
  if(kbar_net_state == DISCONNECTED) {
    g_string_assign(kbar_network_state.text, "N: --");
  }
  else {
    g_string_printf(kbar_network_state.text, "N: %s%s", status_str, network->str);
  }
  kbar_print_bar_state();
}
