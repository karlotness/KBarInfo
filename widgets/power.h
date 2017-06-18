#pragma once
#ifndef WIDGETS_POWER_H
#define WIDGETS_POWER_H

#include "widget.h"

extern kbar_widget_state kbar_power_state;

gboolean kbar_power_init();
void kbar_power_free();

#endif
