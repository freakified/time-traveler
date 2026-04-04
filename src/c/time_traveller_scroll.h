#pragma once

#include "time_traveller_private.h"
#include <pebble.h>

typedef enum {
  ScrollDirectionDown,
  ScrollDirectionUp,
} ScrollDirection;

void time_traveller_scroll_ask(WorldClockData *data, ScrollDirection direction);
void time_traveller_scroll_up_click_handler(ClickRecognizerRef recognizer, void *context);
void time_traveller_scroll_down_click_handler(ClickRecognizerRef recognizer, void *context);
