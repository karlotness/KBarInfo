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
 * along with KBarInfo. If not, see <https://www.gnu.org/licenses/>.
 */

#include <signal.h>
#include <glib.h>
#include <glib-unix.h>
#include "statusbar.h"
#include "widget.h"
#include "time.h"
#include "power.h"
#include "volume.h"
#include "network.h"

struct KBarMainState {
  GMainLoop *main_loop;
  KBarStatusBar *status_bar;
};

static gboolean kbar_idle_loop_start(void *data) {
  KBarStatusBar *status_bar = data;
  kbar_statusbar_start_print(status_bar, SIGUSR1, SIGUSR2);
  kbar_statusbar_output_state(status_bar);
  return G_SOURCE_REMOVE;
}

static gboolean kbar_interrupt_handler(void *data) {
  struct KBarMainState *main_state = data;
  kbar_statusbar_end_print(main_state->status_bar);
  if(main_state->main_loop) {
    g_main_loop_quit(main_state->main_loop);
  }
  return G_SOURCE_CONTINUE;
}

static gboolean kbar_signal_pause(void *data) {
  KBarStatusBar *status_bar = data;
  kbar_statusbar_pause(status_bar);
  return G_SOURCE_CONTINUE;
}

static gboolean kbar_signal_resume(void *data) {
  KBarStatusBar *status_bar = data;
  kbar_statusbar_resume(status_bar);
  return G_SOURCE_CONTINUE;
}

int main(void) {
  struct KBarMainState main_state = {0};
  main_state.main_loop = g_main_loop_new(NULL, FALSE);
  // Construct widgets
  main_state.status_bar = kbar_statusbar_new();
  // Configure signal handlers
  guint sigint_id = g_unix_signal_add(SIGINT, kbar_interrupt_handler, &main_state);
  guint sigusr1_id = g_unix_signal_add(SIGUSR1, kbar_signal_pause, main_state.status_bar);
  guint sigusr2_id = g_unix_signal_add(SIGUSR2, kbar_signal_resume, main_state.status_bar);
  // Add widgets
  kbar_statusbar_take_widget(main_state.status_bar, KBAR_WIDGET(kbar_widget_network_new()));
  kbar_statusbar_take_widget(main_state.status_bar, KBAR_WIDGET(kbar_widget_volume_new()));
  kbar_statusbar_take_widget(main_state.status_bar, KBAR_WIDGET(kbar_widget_power_new()));
  kbar_statusbar_take_widget(main_state.status_bar, KBAR_WIDGET(kbar_widget_time_new()));
  // Run loop
  g_idle_add(kbar_idle_loop_start, main_state.status_bar);
  g_main_loop_run(main_state.main_loop);
  // Destroy widgets and return
  g_source_remove(sigusr2_id);
  g_source_remove(sigusr1_id);
  g_source_remove(sigint_id);
  g_clear_object(&main_state.status_bar);
  g_main_loop_unref(main_state.main_loop);
  main_state.main_loop = NULL;
  return 0;
}
