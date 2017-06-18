#include <glib.h>
#include <glib-unix.h>
#include <stdio.h>

#include "utils/state.h"
#include "utils/dbus.h"
#include "widgets/time.h"
#include "widgets/power.h"

gboolean interrupt_handler(void *data);
GMainLoop *main_loop;

int main(int argc, char *argv[]) {
  kbar_dbus_init();
  main_loop = g_main_loop_new(NULL, FALSE);
  gboolean time_status = kbar_time_init();
  gboolean power_status = kbar_power_init();
  // Configure interrupt signal
  g_unix_signal_add(SIGINT, &interrupt_handler, NULL);
  kbar_start_print();
  kbar_initialized = TRUE;
  kbar_print_bar_state();
  g_main_loop_run(main_loop);
  kbar_end_print();
  kbar_time_free();
  kbar_power_free();
  kbar_dbus_free();
}

gboolean interrupt_handler(void *data) {
  g_main_loop_quit(main_loop);
  return G_SOURCE_REMOVE;
}
