#pragma once

#include "time_traveller_private.h"
#include <pebble.h>

void time_traveller_main_window_push(WorldClockData *data);
Window *time_traveller_main_window_get(void);
void time_traveller_main_window_start_dot_animation(WorldClockData *data, int new_city_index);
void time_traveller_main_window_update_night_mode(WorldClockData *data);
void time_traveller_main_window_force_view_model_change(struct WorldClockMainWindowViewModel *arg);
void time_traveller_main_window_update_status_bar_time(void);
