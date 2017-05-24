#pragma once
#ifndef WIDGETS_TIME_H
#define WIDGETS_TIME_H

#include "widget.h"

extern kbar_widget_state kbar_time_state;

gboolean kbar_time_init(GMainLoop *main_loop);
void kbar_time_free();

#endif
