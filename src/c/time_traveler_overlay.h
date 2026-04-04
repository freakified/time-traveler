#pragma once

#include <pebble.h>
#include "time_traveler_solar_math.h"

#define time_traveler_OVERLAY_MAX_ROWS 168
#define time_traveler_OVERLAY_FULL_NIGHT 255
#define time_traveler_OVERLAY_FULL_DAY_SENTINEL 254

typedef struct {
  uint32_t version;
  uint16_t map_width;
  uint16_t map_height;
  uint16_t expected_rows;
  uint16_t received_rows;
  bool valid;
  uint32_t cache_timestamp;
  uint8_t daylight_start[time_traveler_OVERLAY_MAX_ROWS];
  uint8_t daylight_end[time_traveler_OVERLAY_MAX_ROWS];
  bool row_received[time_traveler_OVERLAY_MAX_ROWS];
} WorldClockOverlay;

void time_traveler_overlay_init(WorldClockOverlay *overlay);

void time_traveler_overlay_deinit(WorldClockOverlay *overlay);

void time_traveler_overlay_update(WorldClockOverlay *overlay,
                                   uint16_t current_width,
                                   uint16_t current_height);

bool time_traveler_overlay_query_row(const WorldClockOverlay *overlay,
                                   uint16_t row, uint8_t *start_out,
                                   uint8_t *end_out);

bool time_traveler_overlay_is_city_night(const WorldClockOverlay *overlay,
                                       uint16_t map_width, int16_t city_row,
                                       int16_t city_col);
