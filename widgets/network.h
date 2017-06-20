#pragma once
#ifndef WIDGETS_NETWORK_H
#define WIDGETS_NETWORK_H

#include "widget.h"

extern kbar_widget_state kbar_network_state;

gboolean kbar_network_init();
void kbar_network_free();

#endif
