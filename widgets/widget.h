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
