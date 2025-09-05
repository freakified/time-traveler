#pragma once

#include <pebble.h>

typedef enum {
  WORLD_CLOCK_ICON_GENERIC_WEATHER,
  WORLD_CLOCK_ICON_HEAVY_RAIN,
  WORLD_CLOCK_ICON_LIGHT_RAIN,
  WORLD_CLOCK_ICON_HEAVY_SNOW,
  WORLD_CLOCK_ICON_LIGHT_SNOW,
  WORLD_CLOCK_ICON_PARTLY_CLOUDY,
  WORLD_CLOCK_ICON_SUNNY_DAY,
} WorldClockIcon;

GDrawCommandImage *world_clock_resources_get_icon(WorldClockIcon icon);
