#pragma once

#include <pebble.h>
#include "time_traveller_data.h"
#include "time_traveller_overlay.h"

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
  GBitmap *world_map_base_image;
  GBitmap *world_map_night_image;
  GBitmap *world_map_image;
  GRect world_map_draw_rect;
  GPoint current_dot_position;
  GPoint target_dot_position;
  int32_t dot_animation_progress;
  bool dot_animation_active;
  WorldClockOverlay overlay;
} WorldClockData;

bool time_traveller_is_city_index_night(WorldClockData *data, int city_index);
GPoint time_traveller_lon_lat_to_screen(float longitude, float latitude,
                                     const GRect map_bounds);
GRect time_traveller_calibrated_map_rect(const WorldClockData *data);
