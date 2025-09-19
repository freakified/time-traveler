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

  if (clock_is_24h_style()) {
    // 24-hour format
    snprintf(model->time.text, sizeof(model->time.text), "%02d:%02d", model->time.hour, model->time.minute);
    model->time.ampm[0] = '\0'; // Empty AM/PM for 24h format
  } else {
    // 12-hour format with separate AM/PM
    int display_hour = hour % 12;
    if (display_hour == 0) display_hour = 12; // 12 AM/PM instead of 0
    const char *ampm = (hour < 12) ? "AM" : "PM";
    snprintf(model->time.text, sizeof(model->time.text), "%d:%02d", display_hour, model->time.minute);
    strcpy(model->time.ampm, ampm);
  }
}

void world_clock_view_model_set_relative_info(WorldClockMainWindowViewModel *model, int16_t relative_offset_hours, WorldClockDataPoint *data_point) {
  // Get current local time to determine if it's today, yesterday, or tomorrow
  time_t now = time(NULL);
  struct tm *local_time = localtime(&now);

  // Calculate local timezone offset in hours
  int16_t local_offset_hours = local_time->tm_gmtoff / 3600;

  // Calculate GMT offset from relative offset
  int16_t gmt_offset_hours = relative_offset_hours + local_offset_hours;

  // Add DST adjustment
  if (data_point->is_dst) {
    gmt_offset_hours += 1;
  }

  // Calculate city's current time in UTC
  time_t city_time_seconds = now + (gmt_offset_hours * 3600);

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

  char dst_indicator[5] = "";
  if (data_point->is_dst) {
    strcpy(dst_indicator, " DST");
  }

  if (relative_offset_hours >= 0) {
    snprintf(model->relative_info.text, sizeof(model->relative_info.text), "%s, +%d HRS%s", day_str, relative_offset_hours, dst_indicator);
  } else {
    snprintf(model->relative_info.text, sizeof(model->relative_info.text), "%s, %d HRS%s", day_str, relative_offset_hours, dst_indicator);
  }

  model->current_offset = relative_offset_hours;
}

WorldClockDataViewNumbers world_clock_data_point_view_model_numbers(WorldClockDataPoint *data_point) {
  // Get current local time
  time_t now = time(NULL);
  struct tm *current_local = localtime(&now);

  // Calculate GMT time by subtracting local timezone offset
  time_t gmt_seconds = now - current_local->tm_gmtoff;

  // Calculate local timezone offset in hours
  int16_t local_offset_hours = current_local->tm_gmtoff / 3600;

  // Calculate GMT offset including DST
  int16_t gmt_offset_hours = data_point->offset_hours + (data_point->is_dst ? 1 : 0);

  // Calculate city time by adding offset to GMT (including DST)
  time_t city_seconds = gmt_seconds + (gmt_offset_hours * 3600);

  // Use localtime to get the correct hour/minute for the city time
  struct tm *city_time = localtime(&city_seconds);

  // Calculate offset relative to user's local time
  int16_t relative_offset = gmt_offset_hours - local_offset_hours;

  return (WorldClockDataViewNumbers){
      .hour = city_time->tm_hour,
      .offset = relative_offset,
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

void world_clock_view_model_fill_numbers(WorldClockMainWindowViewModel *model, WorldClockDataViewNumbers numbers, WorldClockDataPoint *data_point) {
  world_clock_view_model_set_time(model, numbers.hour, numbers.minute);
  world_clock_view_model_set_relative_info(model, numbers.offset, data_point);
}

void world_clock_view_model_fill_colors(WorldClockMainWindowViewModel *model, GColor color) {
  model->bg_color.top = color;
  model->bg_color.bottom = color;
  world_clock_main_window_view_model_announce_changed(model);
}

GColor world_clock_data_point_color(WorldClockDataPoint *data_point) {
  // Use offset to determine color - larger offsets get different colors
  return data_point->offset_hours < -10 ? GColorOrange : GColorVividCerulean;
}

void world_clock_view_model_fill_all(WorldClockMainWindowViewModel *model, WorldClockDataPoint *data_point) {
  WorldClockMainWindowViewModelFunc annouce_changed = model->announce_changed;
  memset(model, 0, sizeof(*model));
  model->announce_changed = annouce_changed;
  world_clock_view_model_fill_strings_and_pagination(model, data_point);
  world_clock_view_model_fill_colors(model, world_clock_data_point_color(data_point));
  world_clock_view_model_fill_numbers(model, world_clock_data_point_view_model_numbers(data_point), data_point);

  world_clock_main_window_view_model_announce_changed(model);
}

void world_clock_view_model_deinit(WorldClockMainWindowViewModel *model) {
  // No cleanup needed for world clock
}

WorldClockDataPoint s_data_points[] = {
    {
        .city = "SAN FRANCISCO",
        .offset_hours = -8, // PST: UTC-8
        .is_dst = false, // Currently PDT
    },
    {
        .city = "NEW YORK",
        .offset_hours = -5, // EST: UTC-5
        .is_dst = false, // Currently EDT
    },
    {
        .city = "LONDON",
        .offset_hours = 0, // GMT: UTC+0
        .is_dst = false, // Currently BST
    },
    {
        .city = "TOKYO",
        .offset_hours = 9, // JST: UTC+9
        .is_dst = false, // No DST
    },
    {
        .city = "SYDNEY",
        .offset_hours = 10, // AEST: UTC+10
        .is_dst = false, // Currently AEDT
    },
};

// City coordinates for map display
static CityCoordinates s_city_coordinates[] = {
    {-122.4194, 37.7749}, // San Francisco
    {-74.0060, 40.7128},  // New York
    {-0.1278, 51.5074},   // London
    {139.6917, 35.6895},  // Tokyo
    {151.2093, -33.8688}, // Sydney
};

// Get coordinates for a city by index
CityCoordinates *world_clock_get_city_coordinates(int city_index) {
    if (city_index < 0 || city_index >= world_clock_num_data_points()) {
        return NULL;
    }
    return &s_city_coordinates[city_index];
}

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
