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

#ifndef KBAR_WIDGET_TIME_H
#define KBAR_WIDGET_TIME_H

#include <glib.h>
#include <glib-object.h>
#include "widget.h"

G_BEGIN_DECLS

#define KBAR_TYPE_WIDGET_TIME (kbar_widget_time_get_type())
G_DECLARE_FINAL_TYPE(KBarWidgetTime, kbar_widget_time, KBAR, WIDGET_TIME, KBarWidget)

KBarWidgetTime *kbar_widget_time_new (void);

G_END_DECLS

#endif
