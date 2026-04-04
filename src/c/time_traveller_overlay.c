#include "time_traveller_overlay.h"
#include <string.h>

void time_traveller_overlay_init(WorldClockOverlay *overlay) {
  memset(overlay, 0, sizeof(WorldClockOverlay));
  overlay->valid = false;
}

void time_traveller_overlay_deinit(WorldClockOverlay *overlay) {
  // Nothing to free
}

void time_traveller_overlay_update(WorldClockOverlay *overlay,
                                   uint16_t current_width,
                                   uint16_t current_height) {
  overlay->map_width = current_width;
  overlay->map_height = current_height;
  overlay->expected_rows = current_height > time_traveller_OVERLAY_MAX_ROWS ? time_traveller_OVERLAY_MAX_ROWS : current_height;
  overlay->received_rows = overlay->expected_rows;
  overlay->valid = true;
  
  time_t now = time(NULL);
  SolarPosition solar = time_traveller_solar_position(now);
  
  for (uint16_t row = 0; row < overlay->expected_rows; ++row) {
    int32_t latitude = (TRIG_MAX_ANGLE / 4) - ((row * 2 + 1) * (TRIG_MAX_ANGLE / 2)) / (2 * current_height);
    
    time_traveller_daylight_interval(latitude, solar, current_width,
      &overlay->daylight_start[row], &overlay->daylight_end[row]);
      
    overlay->row_received[row] = true;
  }
}

bool time_traveller_overlay_query_row(const WorldClockOverlay *overlay,
                                   uint16_t row, uint8_t *start_out,
                                   uint8_t *end_out) {
  if (!overlay->valid || row >= overlay->expected_rows) {
    return false;
  }
  if (!overlay->row_received[row]) {
    return false;
  }
  *start_out = overlay->daylight_start[row];
  *end_out = overlay->daylight_end[row];
  return true;
}

bool time_traveller_overlay_is_city_night(const WorldClockOverlay *overlay,
                                       uint16_t map_width, int16_t city_row,
                                       int16_t city_col) {
  if (!overlay->valid || overlay->expected_rows == 0) {
    return false;
  }

  if (city_row < 0 || city_row >= (int16_t)overlay->expected_rows) {
    return false;
  }

  uint8_t day_start = overlay->daylight_start[city_row];
  uint8_t day_end = overlay->daylight_end[city_row];

  if (day_start == time_traveller_OVERLAY_FULL_NIGHT &&
      day_end == time_traveller_OVERLAY_FULL_NIGHT) {
    return true;
  }

  if (day_start == 0 && day_end == time_traveller_OVERLAY_FULL_DAY_SENTINEL) {
    return false;
  }

  if (day_start <= day_end) {
    return (city_col < day_start || city_col > day_end);
  } else {
    return (city_col > day_end && city_col < day_start);
  }
}
