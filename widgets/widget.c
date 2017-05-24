#include "widget.h"

void kbar_widget_state_init(kbar_widget_state *state) {
  state->text = g_string_new(NULL);
  state->urgent = FALSE;
}

void kbar_widget_state_release(kbar_widget_state *state) {
  g_string_free(state->text, TRUE);
}
