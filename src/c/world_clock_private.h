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
  TextLayer *ampm_layer;
  TextLayer *relative_info_layer;
  Layer *map_layer;
  GDrawCommandImage *world_map_image;
  GRect world_map_draw_rect;
  // Dot animation state
  GPoint current_dot_position;
  GPoint target_dot_position;
  int32_t dot_animation_progress; // 0 to ANIMATION_NORMALIZED_MAX
  bool dot_animation_active;
} WorldClockData;
