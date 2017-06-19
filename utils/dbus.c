#include "dbus.h"

GDBusConnection *kbar_system_bus;

void kbar_dbus_init() {
  kbar_system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
}

void kbar_dbus_free() {
  g_dbus_connection_close_sync(kbar_system_bus, NULL, NULL);
  g_object_unref(kbar_system_bus);
}
