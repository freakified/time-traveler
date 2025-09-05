#include <pebble.h>
#include "world_clock_data.h"

void world_clock_main_window_view_model_announce_changed(WorldClockMainWindowViewModel *model) {
  if (model->announce_changed) {
    model->announce_changed((struct WorldClockMainWindowViewModel *)model);
  }
}

void world_clock_view_model_set_time(WorldClockMainWindowViewModel *model, int16_t hour, int16_t minute) {
  model->time.hour = hour;
  model->time.minute = minute;
  snprintf(model->time.text, sizeof(model->time.text), "%02d:%02d", model->time.hour, model->time.minute);
}

void world_clock_view_model_set_relative_info(WorldClockMainWindowViewModel *model, int16_t offset_hours) {
  // Get current local time to determine if it's today, yesterday, or tomorrow
  time_t now = time(NULL);
  struct tm *local_time = localtime(&now);

  // Calculate GMT time by subtracting local timezone offset
  time_t gmt_seconds = now - local_time->tm_gmtoff;

  // Calculate city time by adding offset to GMT
  time_t city_time_seconds = gmt_seconds + (offset_hours * 3600);

  // Calculate day difference using seconds since epoch
  int local_day = (int)(now / 86400);
  int city_day = (int)(city_time_seconds / 86400);
  int day_diff = city_day - local_day;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "day_diff: %d", day_diff);

  char day_str[10];
  if (day_diff == 0) {
    strcpy(day_str, "TODAY");
  } else if (day_diff == 1) {
    strcpy(day_str, "TOMORROW");
  } else if (day_diff == -1) {
    strcpy(day_str, "YESTERDAY");
  } else {
    strcpy(day_str, "TODAY"); // Fallback
  }

  if (offset_hours >= 0) {
    snprintf(model->relative_info.text, sizeof(model->relative_info.text), "%s, +%d HRS", day_str, offset_hours);
  } else {
    snprintf(model->relative_info.text, sizeof(model->relative_info.text), "%s, %d HRS", day_str, offset_hours);
  }
}

WorldClockDataViewNumbers world_clock_data_point_view_model_numbers(WorldClockDataPoint *data_point) {
  // Get current local time
  time_t now = time(NULL);
  struct tm *current_local = localtime(&now);

  // Calculate GMT time by subtracting local timezone offset
  time_t gmt_seconds = now - current_local->tm_gmtoff;

  // Calculate city time by adding offset to GMT
  time_t city_seconds = gmt_seconds + (data_point->offset_hours * 3600);

  // Use localtime to get the correct hour/minute for the city time
  struct tm *city_time = localtime(&city_seconds);

  return (WorldClockDataViewNumbers){
      .hour = city_time->tm_hour,
      .offset = data_point->offset_hours,
      .minute = city_time->tm_min,
  };
}

int world_clock_index_of_data_point(WorldClockDataPoint *dp);

void world_clock_view_model_fill_strings_and_pagination(WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *data_point) {
  view_model->city = data_point->city;

  view_model->pagination.idx = (int16_t)(1 + world_clock_index_of_data_point(data_point));
  view_model->pagination.num = (int16_t)world_clock_num_data_points();
  snprintf(view_model->pagination.text, sizeof(view_model->pagination.text), "%d/%d", view_model->pagination.idx, view_model->pagination.num);
  world_clock_main_window_view_model_announce_changed(view_model);
}

void world_clock_view_model_fill_numbers(WorldClockMainWindowViewModel *model, WorldClockDataViewNumbers numbers) {
  world_clock_view_model_set_time(model, numbers.hour, numbers.minute);
  world_clock_view_model_set_relative_info(model, numbers.offset);
}

void world_clock_view_model_fill_colors(WorldClockMainWindowViewModel *model, GColor color) {
  model->bg_color.top = color;
  model->bg_color.bottom = color;
  world_clock_main_window_view_model_announce_changed(model);
}

GColor world_clock_data_point_color(WorldClockDataPoint *data_point) {
  // Use offset to determine color - larger offsets get different colors
  return data_point->offset_hours > 10 ? GColorOrange : GColorPictonBlue;
}

void world_clock_view_model_fill_all(WorldClockMainWindowViewModel *model, WorldClockDataPoint *data_point) {
  WorldClockMainWindowViewModelFunc annouce_changed = model->announce_changed;
  memset(model, 0, sizeof(*model));
  model->announce_changed = annouce_changed;
  world_clock_view_model_fill_strings_and_pagination(model, data_point);
  world_clock_view_model_fill_colors(model, world_clock_data_point_color(data_point));
  world_clock_view_model_fill_numbers(model, world_clock_data_point_view_model_numbers(data_point));

  world_clock_main_window_view_model_announce_changed(model);
}

void world_clock_view_model_deinit(WorldClockMainWindowViewModel *model) {
  // No cleanup needed for world clock
}

static WorldClockDataPoint s_data_points[] = {
    {
        .city = "SAN FRANCISCO",
        .offset_hours = -8, // PST: UTC-8
    },
    {
        .city = "NEW YORK",
        .offset_hours = -5, // EST: UTC-5
    },
    {
        .city = "LONDON",
        .offset_hours = 0, // GMT: UTC+0
    },
    {
        .city = "TOKYO",
        .offset_hours = 9, // JST: UTC+9
    },
    {
        .city = "SYDNEY",
        .offset_hours = 10, // AEST: UTC+10
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
