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

#ifndef KBAR_WIDGET_H
#define KBAR_WIDGET_H

#include <glib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

#define KBAR_TYPE_WIDGET (kbar_widget_get_type())
G_DECLARE_DERIVABLE_TYPE(KBarWidget, kbar_widget, KBAR, WIDGET, GObject)

struct _KBarWidgetClass {
  GObjectClass parent_class;
  JsonBuilder *(*build_json)(KBarWidget *self, JsonBuilder *builder);
  gboolean (*start)(KBarWidget *self, GError **error);
  gboolean (*stop)(KBarWidget *self, GError **error);
  gboolean (*pause)(KBarWidget *self, GError **error);
  gboolean (*resume)(KBarWidget *self, GError **error);
};

KBarWidget *kbar_widget_new (void);
JsonBuilder *kbar_widget_build_json(KBarWidget *self, JsonBuilder *builder);
gboolean kbar_widget_start(KBarWidget *self, GError **error);
gboolean kbar_widget_stop(KBarWidget *self, GError **error);
gboolean kbar_widget_pause(KBarWidget *self, GError **error);
gboolean kbar_widget_resume(KBarWidget *self, GError **error);

G_END_DECLS

#endif
