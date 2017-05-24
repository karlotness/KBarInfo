#include <glib.h>
#include <glib-unix.h>

#include "widgets/time.h"

gboolean interrupt_handler(void *data);

GMainLoop *main_loop;

int main(int argc, char *argv[]) {
  main_loop = g_main_loop_new(NULL, FALSE);
  gboolean time_status = kbar_time_init(main_loop);
  // Configure interrupt signal
  g_unix_signal_add(SIGINT, &interrupt_handler, NULL);
  g_main_loop_run(main_loop);
  kbar_time_free();
}

gboolean interrupt_handler(void *data) {
  g_main_loop_quit(main_loop);
  return G_SOURCE_REMOVE;
}
