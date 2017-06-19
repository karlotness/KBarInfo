#include "dbus.h"

GDBusConnection *kbar_system_bus;
GDBusConnection *kbar_session_bus;

void kbar_dbus_init() {
  kbar_system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
  kbar_session_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
}

void kbar_dbus_free() {
  g_dbus_connection_close_sync(kbar_system_bus, NULL, NULL);
  g_dbus_connection_close_sync(kbar_session_bus, NULL, NULL);
  g_object_unref(kbar_system_bus);
  g_object_unref(kbar_session_bus);
}
