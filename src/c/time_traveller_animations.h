#pragma once

#include <pebble.h>
#include "time_traveller_data.h"

struct WorldClockData;

Animation *time_traveller_create_view_model_animation_numbers(WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *next_data_point);

Animation *time_traveller_create_view_model_animation_bgcolor(WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *next_data_point);

Animation *time_traveller_create_view_model_animation_icon(WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *next_data_point, uint32_t duration);
