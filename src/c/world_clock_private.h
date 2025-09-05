#pragma once

#include <pebble.h>
#include "world_clock_data.h"

typedef struct {
  WorldClockDataPoint *data_point;
  WorldClockMainWindowViewModel view_model;
  Animation *previous_animation;
  TextLayer *fake_statusbar;
  TextLayer *pagination_layer;
  TextLayer *city_layer;
  Layer *horizontal_ruler_layer;
  TextLayer *time_layer;
  TextLayer *relative_info_layer;
} WorldClockData;
