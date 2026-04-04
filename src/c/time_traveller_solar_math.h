#pragma once
#include <pebble.h>

int32_t fixed_acos(int32_t x);

// Computes (tan(lat) * tan(decl)) * TRIG_MAX_RATIO
// lat_angle and decl_angle should be in TRIG_MAX_ANGLE units (0-65536)
int32_t fixed_tanProduct(int32_t lat_angle, int32_t decl_angle);

typedef struct {
  int32_t declination;       // in TRIG_MAX_ANGLE units
  int32_t subsolar_longitude; // in TRIG_MAX_ANGLE units
} SolarPosition;

SolarPosition time_traveller_solar_position(time_t current_time);
void time_traveller_daylight_interval(int32_t latitude, SolarPosition solar, uint16_t map_width, uint8_t *start_out, uint8_t *end_out);
