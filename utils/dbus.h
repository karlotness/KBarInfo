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
#pragma once
#ifndef UTILS_DBUS_H
#define UTILS_DBUS_H

#include <gio/gio.h>

#define DBUS_PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

extern GDBusConnection *kbar_system_bus;

void kbar_dbus_init();
void kbar_dbus_free();

#endif
