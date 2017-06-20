#include "../utils/dbus.h"
#include "../utils/debug.h"
#include "../utils/state.h"
#include "network.h"

#define NM_DBUS_NAME "org.freedesktop.NetworkManager"
#define NM_DBUS_INTERFACE NM_DBUS_NAME
#define NM_DBUS_CONN_INTERFACE "org.freedesktop.NetworkManager.Connection.Active"
#define NM_DBUS_PATH "/org/freedesktop/NetworkManager"

kbar_widget_state kbar_network_state;
gboolean kbar_network_error, vpn_active;
guint kbar_network_state_sig, nm_state, *active_conns_sig;
gsize active_conns_cap, active_conns_count;
GString *ssid, *vpn;
GDBusProxy *nm_obj, **active_conns;

static void kbar_network_nm_state_sig(GDBusProxy *proxy,
                                      GVariant *changed,
                                      GStrv *invalid,
                                      gpointer data);
static void kbar_network_nm_conn_sig(GDBusProxy *proxy,
                                     GVariant *changed,
                                     GStrv *invalid,
                                     gpointer data);
static void kbar_network_free_active_conns();
static void kbar_network_update();

gboolean kbar_network_init() {
  active_conns = NULL;
  active_conns_sig = NULL;
  active_conns_cap = 0;
  active_conns_count = 0;
  kbar_network_error = FALSE;
  GError *err = NULL;
  ssid = g_string_new(NULL);
  vpn = g_string_new(NULL);
  nm_state = 0;
  kbar_widget_state_init(&kbar_network_state);
  nm_obj = g_dbus_proxy_new_sync(kbar_system_bus,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,
                                 NM_DBUS_NAME,
                                 NM_DBUS_PATH,
                                 NM_DBUS_INTERFACE,
                                 NULL,
                                 &err);
  if(err != NULL) {
    kbar_err_printf("DBus network error: %s\n", err->message);
    kbar_network_error = TRUE;
    goto exit;
  }
  kbar_network_state_sig = g_signal_connect_after(nm_obj,
                                                  "g-properties-changed",
                                                  G_CALLBACK(kbar_network_nm_state_sig),
                                                  NULL);
  if(kbar_network_state_sig == 0) {
    kbar_err_printf("Network: Error attaching signal\n");
    kbar_network_error = TRUE;
    goto exit;
  }
  kbar_network_nm_state_sig(NULL, NULL, NULL, NULL);
  kbar_network_nm_conn_sig(NULL, NULL, NULL, NULL);
 exit:
  if(err != NULL) {
    g_error_free(err);
    return FALSE;
  }
  return !kbar_network_error;
}

void kbar_network_free() {
  kbar_network_free_active_conns();
  g_signal_handler_disconnect(nm_obj, kbar_network_state_sig);
  g_object_unref(nm_obj);
  kbar_widget_state_release(&kbar_network_state);
  g_string_free(ssid, TRUE);
  g_string_free(vpn, TRUE);
}

static void kbar_network_nm_state_sig(GDBusProxy *proxy,
                                      GVariant *changed,
                                      GStrv *invalid,
                                      gpointer data) {
  GVariant *state = g_dbus_proxy_get_cached_property(nm_obj,
                                                     "State");
  nm_state = g_variant_get_uint32(state);
  g_variant_unref(state);
  kbar_network_free_active_conns();
  // Get new active connections.
  gsize len;
  GVariant *active_c_path = g_dbus_proxy_get_cached_property(nm_obj,
                                                            "ActiveConnections");
  const gchar **conns_path = g_variant_get_objv(active_c_path, &len);
  // Ensure we have the space for these objects
  if(len > active_conns_cap) {
    // Need to reallocate
    active_conns = g_renew(GDBusProxy*, active_conns, len);
    active_conns_sig = g_renew(guint, active_conns_sig, len);
  }
  // Connect to the relevant objects
  for(gsize i = 0; i < len; i++) {
    kbar_err_printf("linking\n");
    GError *err = NULL;
    active_conns[i] = g_dbus_proxy_new_sync(kbar_system_bus,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            NULL,
                                            NM_DBUS_NAME,
                                            conns_path[i],
                                            NM_DBUS_CONN_INTERFACE,
                                            NULL,
                                            &err);
    if(err != NULL) {
      // Error occurred
      g_error_free(err);
      continue;
    }
    active_conns_sig[i] = g_signal_connect_after(active_conns[i],
                                                 "g-properties-changed",
                                                 G_CALLBACK(&kbar_network_nm_conn_sig),
                                                 NULL);
    active_conns_count++;
  }
  kbar_network_nm_conn_sig(NULL, NULL, NULL, NULL);
  g_free(conns_path);
  g_variant_unref(active_c_path);
  kbar_network_update();
}

static void kbar_network_free_active_conns() {
  for(gsize i = 0; i < active_conns_count; i++) {
    if(active_conns_sig[i] > 0) {
      g_signal_handler_disconnect(active_conns[i],
                                  active_conns_sig[i]);
      active_conns_sig[i] = 0;
    }
    g_object_unref(active_conns[i]);
    active_conns[i] = NULL;
  }
  active_conns_count = 0;
  g_string_assign(ssid, "");
  g_string_assign(vpn, "");
  vpn_active = FALSE;
}

static void kbar_network_nm_conn_sig(GDBusProxy *proxy,
                                     GVariant *changed,
                                     GStrv *invalid,
                                     gpointer data) {
  // One of the current connections changed. Re-query all.
  g_string_assign(ssid, "");
  g_string_assign(vpn, "");
  vpn_active = FALSE;
  for(gsize i = 0; i < active_conns_count; i++) {
    GDBusProxy *obj = active_conns[i];
    g_object_ref(obj);
    if(obj == NULL) {
      continue;
    }
    GVariant *type_v = g_dbus_proxy_get_cached_property(obj, "Type");
    GVariant *id_v = g_dbus_proxy_get_cached_property(obj, "Id");
    gchar const *type = g_variant_get_string(type_v, NULL);
    gchar const *id = g_variant_get_string(id_v, NULL);
    if(g_strcmp0(type, "vpn") == 0) {
      // VPN connection. Report it.
      g_string_assign(vpn, id);
    }
    else if(g_strcmp0(type, "tun") == 0) {
      // Tunnel connection (ignore)
      vpn_active = TRUE;
    }
    else {
      // Probably the "primary" connection we want.
      g_string_assign(ssid, id);
    }
    g_variant_unref(type_v);
    g_variant_unref(id_v);
    g_object_unref(obj);
  }
  kbar_network_error = FALSE;
  kbar_network_update();
  kbar_err_printf("%s     %s\n", ssid->str, vpn->str);
}

static void kbar_network_update() {
  kbar_network_state.urgent = kbar_network_error;
  if(kbar_network_error) {
    g_string_assign(kbar_network_state.text, "N: err");
    kbar_print_bar_state();
    return;
  }
  gchar* c_str;
  if(nm_state <= 50 && nm_state >= 40) {
    c_str = "~";
  }
  else {
    c_str = " ";
  }
  if(g_strcmp0(vpn->str, "") != 0 && vpn_active) {
    // VPN is connected
    g_string_printf(kbar_network_state.text, "N:%s%s (%s)", c_str, ssid->str, vpn->str);
  }
  else {
    g_string_printf(kbar_network_state.text, "N:%s%s", c_str, ssid->str);
  }
  kbar_print_bar_state();
}
