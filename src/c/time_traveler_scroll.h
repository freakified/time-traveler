#pragma once

#include "time_traveler_private.h"
#include <pebble.h>

typedef enum {
  ScrollDirectionDown,
  ScrollDirectionUp,
} ScrollDirection;

void time_traveler_scroll_ask(WorldClockData *data, ScrollDirection direction);
void time_traveler_scroll_up_click_handler(ClickRecognizerRef recognizer, void *context);
void time_traveler_scroll_down_click_handler(ClickRecognizerRef recognizer, void *context);
void time_traveler_scroll_up_long_click_handler(ClickRecognizerRef recognizer, void *context);
void time_traveler_scroll_down_long_click_handler(ClickRecognizerRef recognizer, void *context);
void time_traveler_scroll_select_click_handler(ClickRecognizerRef recognizer, void *context);
