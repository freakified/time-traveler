#include <pebble.h>
#include "world_clock_resources.h"

GDrawCommandImage *world_clock_resources_get_icon(WorldClockIcon icon) {
  switch (icon) {
    case WORLD_CLOCK_ICON_HEAVY_RAIN:
      return gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_HEAVY_RAIN);

    case WORLD_CLOCK_ICON_LIGHT_RAIN:
      return gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_LIGHT_RAIN);

    case WORLD_CLOCK_ICON_HEAVY_SNOW:
      return gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_HEAVY_SNOW);

    case WORLD_CLOCK_ICON_LIGHT_SNOW:
      return gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_LIGHT_SNOW);

    case WORLD_CLOCK_ICON_PARTLY_CLOUDY:
      return gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_PARTLY_CLOUDY);

    case WORLD_CLOCK_ICON_SUNNY_DAY:
      return gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_SUNNY_DAY);

    default:
    case WORLD_CLOCK_ICON_GENERIC_WEATHER:
      return gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_GENERIC_WEATHER);
  }
}
