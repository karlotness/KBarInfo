#pragma once
#ifndef UTILS_DBUS_H
#define UTILS_DBUS_H

#include <gio/gio.h>

#define DBUS_PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

extern GDBusConnection *kbar_system_bus;

void kbar_dbus_init();
void kbar_dbus_free();

#endif
