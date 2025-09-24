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

#include "widget.h"

typedef struct {
  GString *text;
  gboolean urgent;
} KBarWidgetPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(KBarWidget, kbar_widget, G_TYPE_OBJECT)

enum {
  PROP_TEXT = 1,
  PROP_URGENT,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {0};

static void kbar_widget_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
  KBarWidgetPrivate *priv = kbar_widget_get_instance_private(KBAR_WIDGET(object));
  switch(prop_id) {
  case PROP_TEXT:
    g_string_assign(priv->text, g_value_get_string(value));
    break;
  case PROP_URGENT:
    priv->urgent = g_value_get_boolean(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void kbar_widget_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
  KBarWidgetPrivate *priv = kbar_widget_get_instance_private(KBAR_WIDGET(object));
  switch(prop_id) {
  case PROP_TEXT:
    g_value_set_string(value, priv->text->str);
    break;
  case PROP_URGENT:
    g_value_set_boolean(value, priv->urgent);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void kbar_widget_finalize(GObject *object) {
  KBarWidgetPrivate *priv = kbar_widget_get_instance_private(KBAR_WIDGET(object));
  g_string_free(priv->text, TRUE);
  priv->text = NULL;
  G_OBJECT_CLASS(kbar_widget_parent_class)->finalize(object);
}

static JsonBuilder *kbar_widget_default_build_json(KBarWidget *self, JsonBuilder *builder) {
  KBarWidgetPrivate *priv = kbar_widget_get_instance_private(self);
  builder = json_builder_begin_object(builder);
  builder = json_builder_set_member_name(builder, "urgent");
  builder = json_builder_add_boolean_value(builder, priv->urgent);
  builder = json_builder_set_member_name(builder, "full_text");
  builder = json_builder_add_string_value(builder, priv->text->str);
  builder = json_builder_end_object(builder);
  return builder;
}

static void kbar_widget_class_init (KBarWidgetClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->set_property = kbar_widget_set_property;
  object_class->get_property = kbar_widget_get_property;
  object_class->finalize = kbar_widget_finalize;
  klass->build_json = kbar_widget_default_build_json;
  obj_properties[PROP_TEXT] = g_param_spec_string("full-text", "text", "Text to display for this block", "", G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB);
  obj_properties[PROP_URGENT] = g_param_spec_boolean("urgent", NULL, "This block should be displayed as urgent", FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB);
  g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void kbar_widget_init(KBarWidget *self) {
  KBarWidgetPrivate *priv = kbar_widget_get_instance_private(self);
  priv->text = g_string_new(NULL);
  priv->urgent = FALSE;
}

KBarWidget *kbar_widget_new (void) {
  return g_object_new(KBAR_TYPE_WIDGET, NULL);
}

JsonBuilder *kbar_widget_build_json(KBarWidget *self, JsonBuilder *builder) {
  g_return_val_if_fail(KBAR_IS_WIDGET(self), NULL);
  g_return_val_if_fail(JSON_IS_BUILDER(builder), NULL);
  KBarWidgetClass *klass = KBAR_WIDGET_GET_CLASS(self);
  g_return_val_if_fail(klass->build_json != NULL, NULL);
  return klass->build_json(self, builder);
}
