#include <stdio.h>

#include "state.h"
#include "../widgets/widget.h"
#include "../widgets/time.h"

#define KBAR_NUM_WIDGETS 1
const kbar_widget_state *states[1] = {&kbar_time_state};

gboolean kbar_initialized = FALSE;

void kbar_start_print() {
  puts("[");
}

void kbar_end_print() {
  puts("[]]");
}

void kbar_print_bar_state() {
  if(!kbar_initialized) {
    return;
  }
  puts("[");
  for(int i = 0; i < KBAR_NUM_WIDGETS; i++) {
    const kbar_widget_state *state = states[i];
    char* urgent = state->urgent ? "true" : "false";
    printf("{\"urgent\": \"%s\", \"full_text\": \"%s\"}",
           urgent, state->text->str);
    if(i < KBAR_NUM_WIDGETS - 1) {
      puts(",");
    }
  }
  puts("],");
  fflush(stdout);
}
