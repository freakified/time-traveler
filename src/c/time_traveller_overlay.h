#pragma once

#include <pebble.h>

#define time_traveller_OVERLAY_MAX_ROWS 140
#define time_traveller_OVERLAY_FULL_NIGHT 255
#define time_traveller_OVERLAY_FULL_DAY_SENTINEL 254
#define time_traveller_OVERLAY_CACHE_MAX_AGE_SECONDS 7200
#define time_traveller_OVERLAY_TIMEOUT_MS 10000

typedef struct {
  uint32_t version;
  uint16_t map_width;
  uint16_t map_height;
  uint16_t expected_rows;
  uint16_t received_rows;
  bool valid;
  uint32_t cache_timestamp;
  uint8_t daylight_start[time_traveller_OVERLAY_MAX_ROWS];
  uint8_t daylight_end[time_traveller_OVERLAY_MAX_ROWS];
  bool row_received[time_traveller_OVERLAY_MAX_ROWS];
} WorldClockOverlay;

typedef void (*WorldClockOverlayCompleteCallback)(void *context);
typedef void (*WorldClockOverlayTimeoutCallback)(void *context);

void time_traveller_overlay_init(WorldClockOverlay *overlay,
                              WorldClockOverlayCompleteCallback on_complete,
                              WorldClockOverlayTimeoutCallback on_timeout,
                              void *context);

void time_traveller_overlay_deinit(WorldClockOverlay *overlay);

bool time_traveller_overlay_load_cache(WorldClockOverlay *overlay,
                                    uint16_t current_width,
                                    uint16_t current_height);

void time_traveller_overlay_reset(WorldClockOverlay *overlay, uint32_t version,
                               uint16_t map_width, uint16_t map_height,
                               uint16_t total_rows);

bool time_traveller_overlay_feed_chunk(WorldClockOverlay *overlay,
                                    uint16_t row_start, uint16_t row_count,
                                    const uint8_t *data, uint16_t data_len);

bool time_traveller_overlay_query_row(const WorldClockOverlay *overlay,
                                   uint16_t row, uint8_t *start_out,
                                   uint8_t *end_out);

void time_traveller_overlay_start_timeout(WorldClockOverlay *overlay);

void time_traveller_overlay_cancel_timeout(WorldClockOverlay *overlay);

void time_traveller_overlay_save_cache(const WorldClockOverlay *overlay);

bool time_traveller_overlay_is_city_night(const WorldClockOverlay *overlay,
                                       uint16_t map_width, int16_t city_row,
                                       int16_t city_col);
