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
#pragma once
#ifndef WIDGETS_WIDGET_H
#define WIDGETS_WIDGET_H

#include <glib.h>

typedef struct kbar_widget_state {
  GString *text;
  gboolean urgent;
} kbar_widget_state;

void kbar_widget_state_init(kbar_widget_state *state);
void kbar_widget_state_release(kbar_widget_state *state);

#endif
