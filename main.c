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
#include <glib.h>
#include <glib-unix.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/state.h"
#include "utils/dbus.h"
#include "widgets/time.h"
#include "widgets/power.h"
#include "widgets/volume.h"
#include "widgets/network.h"

gboolean interrupt_handler(void *data);
gboolean signal_ignore(void *data);
GMainLoop *main_loop;

int main(int argc, char *argv[]) {
  kbar_dbus_init();
  main_loop = g_main_loop_new(NULL, FALSE);
  gboolean time_status = kbar_time_init();
  gboolean power_status = kbar_power_init();
  gboolean volume_status = kbar_volume_init();
  gboolean network_status = kbar_network_init();
  if(!time_status || !power_status || !volume_status ||
     !network_status) {
    exit(3);
  }
  // Configure interrupt signal
  g_unix_signal_add(SIGINT, &interrupt_handler, NULL);
  g_unix_signal_add(SIGUSR1, &signal_ignore, NULL);
  g_unix_signal_add(SIGUSR2, &signal_ignore, NULL);
  kbar_start_print();
  kbar_initialized = TRUE;
  kbar_print_bar_state();
  g_main_loop_run(main_loop);
  kbar_end_print();
  kbar_time_free();
  kbar_power_free();
  kbar_volume_free();
  kbar_network_free();
  kbar_dbus_free();
}

gboolean interrupt_handler(void *data) {
  g_main_loop_quit(main_loop);
  return G_SOURCE_REMOVE;
}

gboolean signal_ignore(void *data) {

}
