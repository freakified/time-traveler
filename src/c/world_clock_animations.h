/*
 * Copyright (c) 2015 Pebble Technology
 */

#pragma once

#include <pebble.h>
#include "world_clock_data.h"

Animation *world_clock_create_view_model_animation_numbers(WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *next_data_point);

Animation *world_clock_create_view_model_animation_bgcolor(WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *next_data_point);

Animation *world_clock_create_view_model_animation_icon(WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *next_data_point, uint32_t duration);
