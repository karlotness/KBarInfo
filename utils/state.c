#include <stdio.h>

#include "state.h"
#include "../widgets/time.h"
#include "../widgets/power.h"
#include "../widgets/volume.h"
#include "../widgets/network.h"

#define KBAR_NUM_WIDGETS 4
const kbar_widget_state *states[KBAR_NUM_WIDGETS] =
  {&kbar_network_state, &kbar_volume_state,
   &kbar_power_state, &kbar_time_state};

static void kbar_json_escape(GString *str);
gboolean kbar_initialized = FALSE;

void kbar_start_print() {
  printf("{\"version\": 1 }\n[");
}

void kbar_end_print() {
  printf("[]]\n");
}

void kbar_print_bar_state() {
  if(!kbar_initialized) {
    return;
  }
  printf("[");
  for(int i = 0; i < KBAR_NUM_WIDGETS; i++) {
    const kbar_widget_state *state = states[i];
    char* urgent = state->urgent ? "true" : "false";
    kbar_json_escape(state->text);
    printf("{\"urgent\": %s, \"full_text\": \"%s\"}",
           urgent, state->text->str);
    if(i < KBAR_NUM_WIDGETS - 1) {
      printf(", ");
    }
  }
  printf("],\n");
  fflush(stdout);
}

static void kbar_json_escape(GString *str) {
  if(str->len <= 0) {
    return;
  }
  gchar *c = str->str;
  while(*c != 0) {
    if(*c == '"' && (c == str->str || *(c - 1) != '\\')) {
      g_string_insert(str, c - str->str, "\\");
      c++;
    }
    c++;
  }
}
