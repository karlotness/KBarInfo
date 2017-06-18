#pragma once
#ifndef WIDGETS_VOLUME_H
#define WIDGETS_VOLUME_H

#include "widget.h"

extern kbar_widget_state kbar_volume_state;

gboolean kbar_volume_init();
void kbar_volume_free();

#endif
