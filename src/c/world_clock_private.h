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
  GBitmap *world_map_base_image;
  GBitmap *world_map_night_image;
  GBitmap *world_map_image;
  GRect world_map_draw_rect;
  // Dot animation state
  GPoint current_dot_position;
  GPoint target_dot_position;
  int32_t dot_animation_progress; // 0 to ANIMATION_NORMALIZED_MAX
  bool dot_animation_active;
  uint16_t overlay_map_width;
  uint16_t overlay_map_height;
  uint32_t overlay_version;
  uint16_t overlay_expected_rows;
  uint16_t overlay_received_rows;
  bool overlay_valid;
  uint8_t overlay_daylight_start[104];
  uint8_t overlay_daylight_end[104];
  bool overlay_row_received[104];
} WorldClockData;

bool world_clock_is_city_index_night(WorldClockData *data, int city_index);
GPoint world_clock_lon_lat_to_screen(float longitude, float latitude,
                                     const GRect map_bounds);
GRect world_clock_calibrated_map_rect(const WorldClockData *data);
