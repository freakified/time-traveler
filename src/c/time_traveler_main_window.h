#pragma once

#include "time_traveler_private.h"
#include <pebble.h>

void time_traveler_main_window_push(WorldClockData *data);
Window *time_traveler_main_window_get(void);
void time_traveler_main_window_start_dot_animation(WorldClockData *data, int new_city_index);
void time_traveler_main_window_update_night_mode(WorldClockData *data);
void time_traveler_main_window_update_gps_arrow(WorldClockData *data, WorldClockDataPoint *dp);
void time_traveler_main_window_force_view_model_change(struct WorldClockMainWindowViewModel *arg);
void time_traveler_main_window_update_status_bar_time(void);
