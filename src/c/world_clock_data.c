/*
 * Copyright (c) 2015 Pebble Technology
 */

#include <pebble.h>
#include "world_clock_data.h"
#include "world_clock_resources.h"

void world_clock_main_window_view_model_announce_changed(WorldClockMainWindowViewModel *model) {
  if (model->announce_changed) {
    model->announce_changed((struct WorldClockMainWindowViewModel *)model);
  }
}

void world_clock_view_model_set_highlow(WorldClockMainWindowViewModel *model, int16_t high, int16_t low) {
  model->highlow.high = high;
  model->highlow.low = low;
  snprintf(model->highlow.text, sizeof(model->highlow.text), "HI %d°, LO %d°", model->highlow.high, model->highlow.low);
}

void world_clock_view_model_set_temperature(WorldClockMainWindowViewModel *model, int16_t value) {
  model->temperature.value = value;
  snprintf(model->temperature.text, sizeof(model->temperature.text), "%d°", model->temperature.value);
}

void world_clock_view_model_set_icon(WorldClockMainWindowViewModel *model, GDrawCommandImage *image) {
  free(model->icon.draw_command);
  model->icon.draw_command = image;
  world_clock_main_window_view_model_announce_changed(model);
}

WorldClockDataViewNumbers world_clock_data_point_view_model_numbers(WorldClockDataPoint *data_point) {
  return (WorldClockDataViewNumbers){
      .temperature = data_point->current,
      .high = data_point->high,
      .low = data_point->low,
  };
}

int world_clock_index_of_data_point(WorldClockDataPoint *dp);

void world_clock_view_model_fill_strings_and_pagination(WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *data_point) {
  view_model->city = data_point->city;
  view_model->description = data_point->description;

  view_model->pagination.idx = (int16_t)(1 + world_clock_index_of_data_point(data_point));
  view_model->pagination.num = (int16_t)world_clock_num_data_points();
  snprintf(view_model->pagination.text, sizeof(view_model->pagination.text), "%d/%d", view_model->pagination.idx, view_model->pagination.num);
  world_clock_main_window_view_model_announce_changed(view_model);
}


GDrawCommandImage *world_clock_data_point_create_icon(WorldClockDataPoint *data_point) {
  return world_clock_resources_get_icon(data_point->icon);
}


void world_clock_view_model_fill_numbers(WorldClockMainWindowViewModel *model, WorldClockDataViewNumbers numbers) {
  world_clock_view_model_set_temperature(model, numbers.temperature);
  world_clock_view_model_set_highlow(model, numbers.high, numbers.low);
}

void world_clock_view_model_fill_colors(WorldClockMainWindowViewModel *model, GColor color) {
  model->bg_color.top = color;
  model->bg_color.bottom = color;
  world_clock_main_window_view_model_announce_changed(model);
}

GColor world_clock_data_point_color(WorldClockDataPoint *data_point) {
  return data_point->current > 90 ? GColorOrange : GColorPictonBlue;
}

void world_clock_view_model_fill_all(WorldClockMainWindowViewModel *model, WorldClockDataPoint *data_point) {
  WorldClockMainWindowViewModelFunc annouce_changed = model->announce_changed;
  memset(model, 0, sizeof(*model));
  model->announce_changed = annouce_changed;
  world_clock_view_model_fill_strings_and_pagination(model, data_point);
  world_clock_view_model_set_icon(model, world_clock_data_point_create_icon(data_point));
  world_clock_view_model_fill_colors(model, world_clock_data_point_color(data_point));
  world_clock_view_model_fill_numbers(model, world_clock_data_point_view_model_numbers(data_point));

  world_clock_main_window_view_model_announce_changed(model);
}

void world_clock_view_model_deinit(WorldClockMainWindowViewModel *model) {
  world_clock_view_model_set_icon(model, NULL);
}

static WorldClockDataPoint s_data_points[] = {
    {
        .city = "PALO ALTO",
        .description = "Light Rain.",
        .icon = WORLD_CLOCK_ICON_LIGHT_RAIN,
        .current = 68,
        .high = 70,
        .low = 60,
    },
    {
        .city = "LOS ANGELES",
        .description = "Clear throughout the day.",
        .icon = WORLD_CLOCK_ICON_SUNNY_DAY,
        .current = 100,
        .high = 100,
        .low = 80,
    },
    {
        .city = "SAN FRANCISCO",
        .description = "Rain and Fog.",
        .icon = WORLD_CLOCK_ICON_HEAVY_SNOW,
        .current = 60,
        .high = 62,
        .low = 56,
    },
    {
        .city = "SAN DIEGO",
        .description = "Surfboard :)",
        .icon = WORLD_CLOCK_ICON_GENERIC_WEATHER,
        .current = 110,
        .high = 120,
        .low = 9,
    },
};

int world_clock_num_data_points(void) {
  return ARRAY_LENGTH(s_data_points);
}

WorldClockDataPoint *world_clock_data_point_at(int idx) {
  if (idx < 0 || idx > world_clock_num_data_points() - 1) {
    return NULL;
  }

  return &s_data_points[idx];
}

int world_clock_index_of_data_point(WorldClockDataPoint *dp) {
  for (int i = 0; i < world_clock_num_data_points(); i++) {
    if (dp == world_clock_data_point_at(i)) {
      return i;
    }
  }
  return -1;
}

WorldClockDataPoint *world_clock_data_point_delta(WorldClockDataPoint *dp, int delta) {
  int idx = world_clock_index_of_data_point(dp);
  if (idx < 0) {
    return NULL;
  }
  return world_clock_data_point_at(idx + delta);
}
