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
#include <stdio.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gio/gunixoutputstream.h>
#include <json-glib/json-glib.h>

#include "state.h"
#include "../widgets/time.h"
#include "../widgets/power.h"
#include "../widgets/volume.h"
#include "../widgets/network.h"

#define KBAR_NUM_WIDGETS 4
const kbar_widget_state *states[KBAR_NUM_WIDGETS] =
  {&kbar_network_state, &kbar_volume_state,
   &kbar_power_state, &kbar_time_state};

gboolean kbar_initialized = FALSE;
JsonBuilder *builder;
JsonGenerator *generator;
GOutputStream *stdout_stream;

gboolean kbar_json_init() {
  builder = json_builder_new_immutable();
  generator = json_generator_new();
  if(generator) {
    json_generator_set_pretty(generator, FALSE);
    json_generator_set_indent(generator, 0);
  }
  stdout_stream = g_unix_output_stream_new(fileno(stdout), FALSE);
  return builder && generator && stdout_stream;
}

void kbar_json_free() {
  if(builder) {
    json_builder_reset(builder);
    g_object_unref(builder);
  }
  if (generator) {
    g_object_unref(generator);
  }
  if (stdout_stream) {
    g_output_stream_close(stdout_stream, NULL, NULL);
    g_object_unref(stdout_stream);
  }
}

void kbar_start_print() {
  json_builder_reset(builder);
  json_builder_begin_object(builder);
  json_builder_set_member_name(builder, "version");
  json_builder_add_int_value(builder, 1);
  json_builder_set_member_name(builder, "stop_signal");
  json_builder_add_int_value(builder, SIGUSR1);
  json_builder_set_member_name(builder, "cont_signal");
  json_builder_add_int_value(builder, SIGUSR2);
  json_builder_end_object(builder);
  JsonNode *node = json_builder_get_root(builder);
  if(!node) {
    return;
  }
  json_generator_set_root(generator, node);
  json_generator_to_stream(generator, stdout_stream, NULL, NULL);
  json_node_unref(node);
  g_output_stream_printf(stdout_stream, NULL, NULL, NULL, "\n[");
  g_output_stream_flush(stdout_stream, NULL, NULL);
}

void kbar_end_print() {
  g_output_stream_printf(stdout_stream, NULL, NULL, NULL, "[]]\n");
  g_output_stream_flush(stdout_stream, NULL, NULL);
}

void kbar_print_bar_state() {
  if(!kbar_initialized) {
    return;
  }
  json_builder_reset(builder);
  json_builder_begin_array(builder);
  for(int i = 0; i < KBAR_NUM_WIDGETS; i++) {
    const kbar_widget_state *state = states[i];
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "urgent");
    json_builder_add_boolean_value(builder, state->urgent);
    json_builder_set_member_name(builder, "full_text");
    json_builder_add_string_value(builder, state->text->str);
    json_builder_end_object(builder);
  }
  json_builder_end_array(builder);
  JsonNode *node = json_builder_get_root(builder);
  if(!node) {
    return;
  }
  json_generator_set_root(generator, node);
  json_generator_to_stream(generator, stdout_stream, NULL, NULL);
  json_node_unref(node);
  g_output_stream_printf(stdout_stream, NULL, NULL, NULL, ",\n");
  g_output_stream_flush(stdout_stream, NULL, NULL);
}
