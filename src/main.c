/*
 * Copyright 2025 Karl Otness
 * SPDX-License-Identifier: GPL-3.0-or-later
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
#include "widget.h"
#include "time.h"

static GMainLoop *main_loop = NULL;
static guint idle_handler = 0;

static gboolean kbar_interrupt_handler([[maybe_unused]] void *data) {
  if(main_loop) {
    g_main_loop_quit(main_loop);
  }
  return G_SOURCE_CONTINUE;
}

static gboolean kbar_signal_ignore([[maybe_unused]] void *data) {
  return G_SOURCE_CONTINUE;
}

static gboolean kbar_output_bar_state(gpointer user_data) {
  [[maybe_unused]] KBarWidget **widgets = user_data;
  idle_handler = 0;
  return G_SOURCE_REMOVE;
}

static void kbar_handle_change_notification([[maybe_unused]] GObject *self, [[maybe_unused]] GParamSpec *pspec, gpointer user_data) {
  if(idle_handler == 0) {
    idle_handler = g_idle_add(kbar_output_bar_state, user_data);
  }
}

int main(void) {
  main_loop = g_main_loop_new(NULL, FALSE);
  guint sigint_id = g_unix_signal_add(SIGINT, kbar_interrupt_handler, NULL);
  guint sigusr1_id = g_unix_signal_add(SIGUSR1, kbar_signal_ignore, NULL);
  guint sigusr2_id = g_unix_signal_add(SIGUSR2, kbar_signal_ignore, NULL);
  // Construct widgets
  KBarWidget *widgets[2] = {0};
  const unsigned int num_widgets = sizeof(widgets) / sizeof(widgets[0]) - 1;
  widgets[0] = KBAR_WIDGET(kbar_widget_time_new());
  // Subscribe to all widget updates
  for(unsigned int i = 0; i < num_widgets; i++) {
    g_signal_connect_after(widgets[i], "notify", G_CALLBACK(kbar_handle_change_notification), widgets);
  }
  // Run loop
  idle_handler = g_idle_add(kbar_output_bar_state, widgets);
  g_main_loop_run(main_loop);
  // Destroy widgets and return
  if(idle_handler != 0) {
    g_source_remove(idle_handler);
    idle_handler = 0;
  }
  for(unsigned int i = 0; i < num_widgets; i++) {
    g_object_unref(G_OBJECT(widgets[i]));
    widgets[i] = NULL;
  }
  g_source_remove(sigusr2_id);
  g_source_remove(sigusr1_id);
  g_source_remove(sigint_id);
  g_main_loop_unref(main_loop);
  main_loop = NULL;
  return 0;
}
