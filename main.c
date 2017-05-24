#include <stdio.h>
#include <glib.h>

int main(int argc, char *argv[]) {
  GMainLoop *main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(main_loop);
}
