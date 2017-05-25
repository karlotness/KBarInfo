#pragma once
#ifndef UTILS_STATE_H
#define UTILS_STATE_H

#include <glib.h>

extern gboolean kbar_initialized;

void kbar_start_print();
void kbar_end_print();
void kbar_print_bar_state();

#endif
