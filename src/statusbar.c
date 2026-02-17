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

#include <unistd.h>
#include <gio/gio.h>
#include <gio/gunixoutputstream.h>
#include "statusbar.h"

struct KBarStatusBarEntry {
  KBarWidget *widget;
  gulong handler_id;
};

struct _KBarStatusBar {
  GObject parent_instance;
  gboolean print_started;
  GArray *bar_entries;
  JsonBuilder *builder;
  JsonGenerator *generator;
  GOutputStream *stdout_stream;
  guint idle_handler;
};

G_DEFINE_FINAL_TYPE(KBarStatusBar, kbar_statusbar, G_TYPE_OBJECT)

static void kbar_statusbar_dispose(GObject *object) {
  KBarStatusBar *self = KBAR_STATUSBAR(object);
  // Release bar entries
  for(gsize i = 0; i < self->bar_entries->len; i++) {
    struct KBarStatusBarEntry *entry = &g_array_index(self->bar_entries, struct KBarStatusBarEntry, i);
    g_clear_signal_handler(&entry->handler_id, G_OBJECT(entry->widget));
    g_clear_object(&entry->widget);
  }
  g_array_remove_range(self->bar_entries, 0, self->bar_entries->len);
  // Remove idle handlers
  if(self->idle_handler != 0) {
    g_source_remove(self->idle_handler);
    self->idle_handler = 0;
  }
  // Reset JSON writers
  json_builder_reset(self->builder);
  // Process parent class
  G_OBJECT_CLASS(kbar_statusbar_parent_class)->dispose(object);
}

static void kbar_statusbar_finalize(GObject *object) {
  KBarStatusBar *self = KBAR_STATUSBAR(object);
  g_array_free(self->bar_entries, TRUE);
  self->bar_entries = NULL;
  // Free JSON writers
  g_clear_object(&self->builder);
  g_clear_object(&self->generator);
  g_output_stream_close(self->stdout_stream, NULL, NULL);
  g_clear_object(&self->stdout_stream);
  // Free parent class
  G_OBJECT_CLASS(kbar_statusbar_parent_class)->finalize(object);
}

static void kbar_statusbar_class_init(KBarStatusBarClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = kbar_statusbar_finalize;
  object_class->dispose = kbar_statusbar_dispose;
}

static void kbar_statusbar_init(KBarStatusBar *self) {
  self->print_started = FALSE;
  self->bar_entries = g_array_new(FALSE, FALSE, sizeof(struct KBarStatusBarEntry));
  self->idle_handler = 0;
  self->builder = json_builder_new_immutable();
  self->generator = json_generator_new();
  json_generator_set_pretty(self->generator, FALSE);
  json_generator_set_indent(self->generator, 0);
  self->stdout_stream = g_unix_output_stream_new(STDOUT_FILENO, FALSE);
}

KBarStatusBar *kbar_statusbar_new (void) {
  return g_object_new(KBAR_TYPE_STATUSBAR, NULL);
}

static gboolean kbar_statusbar_idle_output_state(gpointer user_data) {
  KBarStatusBar *bar = KBAR_STATUSBAR(user_data);
  if(bar->print_started) {
    kbar_statusbar_output_state(bar);
  }
  bar->idle_handler = 0;
  g_object_unref(bar);
  return G_SOURCE_REMOVE;
}

static void kbar_status_bar_handle_change_notification([[maybe_unused]] GObject *self, [[maybe_unused]] GParamSpec *pspec, gpointer user_data) {
  KBarStatusBar *bar = KBAR_STATUSBAR(user_data);
  if(bar->idle_handler == 0) {
    bar->idle_handler = g_idle_add(kbar_statusbar_idle_output_state, g_object_ref(bar));
  }
}

void kbar_statusbar_take_widget (KBarStatusBar *self, KBarWidget *widget) {
  g_return_if_fail(KBAR_IS_STATUSBAR(self));
  g_return_if_fail(KBAR_IS_WIDGET(widget));
  struct KBarStatusBarEntry new_entry;
  new_entry.widget = widget;
  new_entry.handler_id = g_signal_connect_after(new_entry.widget, "notify", G_CALLBACK(kbar_status_bar_handle_change_notification), self);
  g_array_append_val(self->bar_entries, new_entry);
}

void kbar_statusbar_end_print(KBarStatusBar *self) {
  g_return_if_fail(KBAR_IS_STATUSBAR(self));
  g_return_if_fail(self->print_started);
  g_output_stream_printf(self->stdout_stream, NULL, NULL, NULL, "[]]\n");
  g_output_stream_flush(self->stdout_stream, NULL, NULL);
  for(gsize i = 0; i < self->bar_entries->len; i++) {
    kbar_widget_stop(g_array_index(self->bar_entries, struct KBarStatusBarEntry, i).widget, NULL);
  }
  self->print_started = FALSE;
}

void kbar_statusbar_start_print(KBarStatusBar *self, gint stop_signal, gint cont_signal) {
  g_return_if_fail(KBAR_IS_STATUSBAR(self));
  g_return_if_fail(!self->print_started);
  json_builder_reset(self->builder);
  json_builder_begin_object(self->builder);
  json_builder_set_member_name(self->builder, "version");
  json_builder_add_int_value(self->builder, 1);
  json_builder_set_member_name(self->builder, "stop_signal");
  json_builder_add_int_value(self->builder, stop_signal);
  json_builder_set_member_name(self->builder, "cont_signal");
  json_builder_add_int_value(self->builder, cont_signal);
  json_builder_end_object(self->builder);
  JsonNode *node = json_builder_get_root(self->builder);
  if(!node) {
    g_printerr("Error generating initial JSON object.\n");
    return;
  }
  json_generator_set_root(self->generator, node);
  json_generator_to_stream(self->generator, self->stdout_stream, NULL, NULL);
  json_node_unref(node);
  node = NULL;
  g_output_stream_printf(self->stdout_stream, NULL, NULL, NULL, "\n[");
  g_output_stream_flush(self->stdout_stream, NULL, NULL);
  // Start all widgets
  for(gsize i = 0; i < self->bar_entries->len; i++) {
    kbar_widget_start(g_array_index(self->bar_entries, struct KBarStatusBarEntry, i).widget, NULL);
  }
  self->print_started = TRUE;
}

void kbar_statusbar_output_state (KBarStatusBar *self) {
  g_return_if_fail(KBAR_IS_STATUSBAR(self));
  g_return_if_fail(self->print_started);
  json_builder_reset(self->builder);
  json_builder_begin_array(self->builder);
  for(gsize i = 0; i < self->bar_entries->len; i++) {
    kbar_widget_build_json(g_array_index(self->bar_entries, struct KBarStatusBarEntry, i).widget, self->builder);
  }
  json_builder_end_array(self->builder);
  JsonNode *node = json_builder_get_root(self->builder);
  if(!node) {
    g_printerr("Error generating status JSON object.\n");
    return;
  }
  json_generator_set_root(self->generator, node);
  json_generator_to_stream(self->generator, self->stdout_stream, NULL, NULL);
  json_node_unref(node);
  node = NULL;
  g_output_stream_printf(self->stdout_stream, NULL, NULL, NULL, ",\n");
  g_output_stream_flush(self->stdout_stream, NULL, NULL);
}
