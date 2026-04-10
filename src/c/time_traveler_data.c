#include "time_traveler_data.h"
#include "metrics.h"
#include <pebble.h>

static WorldClockDataPoint s_cities[MAX_JS_CITIES];
static int s_num_cities;

static int8_t s_date_format = 0;

void time_traveler_main_window_view_model_announce_changed(
    WorldClockMainWindowViewModel *model) {
  if (model->announce_changed) {
    model->announce_changed((struct WorldClockMainWindowViewModel *)model);
  }
}

void time_traveler_view_model_set_time(WorldClockMainWindowViewModel *model,
                                       int16_t hour, int16_t minute) {
  model->time.hour = hour;
  model->time.minute = minute;

  if (clock_is_24h_style()) {
    snprintf(model->time.text, sizeof(model->time.text), "%02d:%02d",
             model->time.hour, model->time.minute);
    model->meridiem.text[0] = '\0';
  } else {
    int display_hour = hour % 12;
    if (display_hour == 0)
      display_hour = 12;
    snprintf(model->time.text, sizeof(model->time.text), "%d:%02d",
             display_hour, model->time.minute);
    snprintf(model->meridiem.text, sizeof(model->meridiem.text),
             hour < 12 ? "AM" : "PM");
  }
}

void time_traveler_view_model_set_relative_info(
    WorldClockMainWindowViewModel *model, int16_t relative_offset_minutes,
    WorldClockDataPoint *data_point) {
  if (time_traveler_data_is_user_location(data_point)) {
    static const char *s_days[]   = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
    static const char *s_months[] = {"JAN","FEB","MAR","APR","MAY","JUN",
                                     "JUL","AUG","SEP","OCT","NOV","DEC"};
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (s_date_format == 1) {
      snprintf(model->relative_info.text, sizeof(model->relative_info.text),
               "%s, %02d %s", s_days[t->tm_wday], t->tm_mday, s_months[t->tm_mon]);
    } else if (s_date_format == 2) {
      snprintf(model->relative_info.text, sizeof(model->relative_info.text),
               "%04d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    } else {
      snprintf(model->relative_info.text, sizeof(model->relative_info.text),
               "%s, %s %02d", s_days[t->tm_wday], s_months[t->tm_mon], t->tm_mday);
    }
    model->current_offset = 0;
    return;
  }

  int16_t abs_minutes = (relative_offset_minutes < 0) ? -relative_offset_minutes
                                                      : relative_offset_minutes;
  int16_t hours = abs_minutes / 60;
  int16_t mins = abs_minutes % 60;
  int sign = (relative_offset_minutes >= 0) ? 1 : -1;

  char day_str[16];
  if (data_point->day_label == -1) {
    strcpy(day_str, "YESTERDAY");
  } else if (data_point->day_label == 1) {
    strcpy(day_str, "TOMORROW");
  } else {
    strcpy(day_str, "TODAY");
  }

  char offset_str[16];
  if (mins == 0) {
    snprintf(offset_str, sizeof(offset_str), "%c%d HRS", sign > 0 ? '+' : '-',
             hours);
  } else {
    snprintf(offset_str, sizeof(offset_str), "%c%d:%02d HRS",
             sign > 0 ? '+' : '-', hours, mins);
  }

  snprintf(model->relative_info.text, sizeof(model->relative_info.text),
           "%s, %s", day_str, offset_str);

  model->current_offset = relative_offset_minutes;
}

static WorldClockDataPoint s_user_location_dp = {
#if defined(PBL_PLATFORM_APLITE) || defined(PBL_PLATFORM_BASALT) ||            \
    defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_FLINT)
    .city = "CURR. LOCATION",
#else
    .city = "CURRENT LOCATION",
#endif
    .latitude = 0,
    .longitude = 0,
    .offset_minutes = 0,
    .day_label = 0,
    .is_night = false};

static float s_user_lat = 0;
static float s_user_lon = 0;
static bool s_has_user_location = false;
static int s_user_matched_city_index = -1;

void time_traveler_data_set_date_format(int8_t format) {
  s_date_format = format;
}

void time_traveler_data_set_user_location(float lat, float lon) {
  s_user_lat = lat;
  s_user_lon = lon;
  s_has_user_location = true;
}

bool time_traveler_data_has_user_location(void) { return s_has_user_location; }

void time_traveler_data_get_user_location(float *lat, float *lon) {
  if (lat) *lat = s_user_lat;
  if (lon) *lon = s_user_lon;
}

bool time_traveler_data_is_user_location(WorldClockDataPoint *dp) {
  if (dp == &s_user_location_dp) return true;
  // If the user is matched to a pinned city, that city counts as user location
  if (s_user_matched_city_index >= 0 && s_user_matched_city_index < s_num_cities) {
    return (dp == &s_cities[s_user_matched_city_index]);
  }
  return false;
}

void time_traveler_data_set_user_matched_city(int index) {
  s_user_matched_city_index = index;
}

int time_traveler_data_get_user_matched_city(void) {
  return s_user_matched_city_index;
}

void time_traveler_data_clear_user_matched_city(void) {
  s_user_matched_city_index = -1;
}

int time_traveler_data_find_user_location_index(void) {
  for (int i = 0; i < time_traveler_num_data_points(); i++) {
    if (time_traveler_data_is_user_location(time_traveler_data_point_at(i))) {
      return i;
    }
  }
  return -1;
}

WorldClockDataViewNumbers
time_traveler_data_point_view_model_numbers(WorldClockDataPoint *data_point) {
  if (!data_point) {
    return (WorldClockDataViewNumbers){0};
  }
  time_t now = time(NULL);
  struct tm *current_local = localtime(&now);

  if (data_point == &s_user_location_dp) {
    return (WorldClockDataViewNumbers){
        .hour = current_local->tm_hour,
        .offset = 0,
        .minute = current_local->tm_min,
    };
  }

  time_t gmt_seconds = now - current_local->tm_gmtoff;
  int16_t local_offset_minutes = current_local->tm_gmtoff / 60;

  time_t city_seconds = gmt_seconds + (data_point->offset_minutes * 60);
  struct tm *city_time = localtime(&city_seconds);

  int16_t relative_offset_minutes =
      data_point->offset_minutes - local_offset_minutes;

  return (WorldClockDataViewNumbers){
      .hour = city_time->tm_hour,
      .offset = relative_offset_minutes,
      .minute = city_time->tm_min,
  };
}

int time_traveler_index_of_data_point(WorldClockDataPoint *dp);

void time_traveler_view_model_fill_strings_and_pagination(
    WorldClockMainWindowViewModel *view_model,
    WorldClockDataPoint *data_point) {
  view_model->city = data_point->city;

  view_model->pagination.idx =
      (int16_t)(1 + time_traveler_index_of_data_point(data_point));
  view_model->pagination.num = (int16_t)time_traveler_num_data_points();
  snprintf(view_model->pagination.text, sizeof(view_model->pagination.text),
           "%d/%d", view_model->pagination.idx, view_model->pagination.num);
  time_traveler_main_window_view_model_announce_changed(view_model);
}

void time_traveler_view_model_fill_numbers(WorldClockMainWindowViewModel *model,
                                           WorldClockDataViewNumbers numbers,
                                           WorldClockDataPoint *data_point) {
  time_traveler_view_model_set_time(model, numbers.hour, numbers.minute);
  time_traveler_view_model_set_relative_info(model, numbers.offset, data_point);
}

void time_traveler_view_model_fill_colors(WorldClockMainWindowViewModel *model,
                                          GColor color) {
  model->bg_color.top = color;
  model->bg_color.bottom = color;
  time_traveler_main_window_view_model_announce_changed(model);
}

GColor time_traveler_data_point_color(WorldClockDataPoint *data_point,
                                      bool is_night) {
  return is_night ? COLOR_APP_BACKGROUND_NIGHT : COLOR_APP_BACKGROUND;
}

void time_traveler_view_model_fill_all(WorldClockMainWindowViewModel *model,
                                       WorldClockDataPoint *data_point) {
  WorldClockMainWindowViewModelFunc annouce_changed = model->announce_changed;
  memset(model, 0, sizeof(*model));
  model->announce_changed = annouce_changed;
  time_traveler_view_model_fill_strings_and_pagination(model, data_point);
  time_traveler_view_model_fill_colors(
      model, time_traveler_data_point_color(data_point, false));
  time_traveler_view_model_fill_night_mode(model, false);
  time_traveler_view_model_fill_numbers(
      model, time_traveler_data_point_view_model_numbers(data_point),
      data_point);

  time_traveler_main_window_view_model_announce_changed(model);
}

void time_traveler_view_model_fill_loading(
    WorldClockMainWindowViewModel *model) {
  WorldClockMainWindowViewModelFunc annouce_changed = model->announce_changed;
  memset(model, 0, sizeof(*model));
  model->announce_changed = annouce_changed;

  model->city = "LOADING...";
  model->pagination.idx = 0;
  model->pagination.num = (int16_t)time_traveler_num_data_points();
  snprintf(model->pagination.text, sizeof(model->pagination.text), "-/%d",
           model->pagination.num);

  time_t now = time(NULL);
  struct tm *current_local = localtime(&now);
  time_traveler_view_model_set_time(model, current_local->tm_hour,
                                    current_local->tm_min);
  strcpy(model->relative_info.text, "PLEASE WAIT");

  time_traveler_view_model_fill_colors(model, COLOR_APP_BACKGROUND);
  time_traveler_view_model_fill_night_mode(model, false);

  time_traveler_main_window_view_model_announce_changed(model);
}

void time_traveler_view_model_fill_night_mode(
    WorldClockMainWindowViewModel *model, bool is_night) {
  model->is_night = is_night;
  if (is_night) {
    model->text_color = COLOR_TEXT_DEFAULT_NIGHT;
    model->crosshair_color = COLOR_CROSSHAIR_NIGHT;
    model->dot_fill_color = COLOR_DOT_FILL_NIGHT;
    model->dot_outline_color = COLOR_DOT_OUTLINE_NIGHT;
    model->statusbar_text_color = COLOR_STATUSBAR_TEXT_NIGHT;
    model->ruler_color = COLOR_RULER_NIGHT;
    model->bg_color.top = COLOR_APP_BACKGROUND_NIGHT;
    model->bg_color.bottom = COLOR_APP_BACKGROUND_NIGHT;
  } else {
    model->text_color = COLOR_TEXT_DEFAULT;
    model->crosshair_color = COLOR_CROSSHAIR;
    model->dot_fill_color = COLOR_DOT_FILL;
    model->dot_outline_color = COLOR_DOT_OUTLINE;
    model->statusbar_text_color = COLOR_STATUSBAR_TEXT;
    model->ruler_color = COLOR_RULER;
    model->bg_color.top = COLOR_APP_BACKGROUND;
    model->bg_color.bottom = COLOR_APP_BACKGROUND;
  }
  time_traveler_main_window_view_model_announce_changed(model);
}

void time_traveler_view_model_deinit(WorldClockMainWindowViewModel *model) {}

// ============================================================
// Dynamic city list — populated entirely from JS blob
// (s_cities and s_num_cities are forward-declared at the top)
// ============================================================

// Apply binary blob from JS: CITY_BLOB_BYTES_PER_CITY bytes per city
//   bytes 0-15:  city name, null-terminated
//   bytes 16-17: latitude  × 100 as int16, big-endian
//   bytes 18-19: longitude × 100 as int16, big-endian
//   bytes 20-21: offset_minutes as int16, big-endian
//   byte  22:    day_label (0=today, 1=tomorrow, 255=yesterday)
//   byte  23:    is_night (0 or 1)
void time_traveler_data_apply_js_blob(const uint8_t *blob, uint16_t length) {
  int received = length / CITY_BLOB_BYTES_PER_CITY;
  if (received > MAX_JS_CITIES) received = MAX_JS_CITIES;

  for (int i = 0; i < received; i++) {
    const uint8_t *p = &blob[i * CITY_BLOB_BYTES_PER_CITY];
    WorldClockDataPoint *dp = &s_cities[i];

    // Name: null-terminated, at most 15 chars from blob
    strncpy(dp->city, (const char *)p, 15);
    dp->city[15] = '\0';

    // Latitude and longitude (×100, big-endian int16)
    int16_t lat_x100 = (int16_t)((p[16] << 8) | p[17]);
    int16_t lon_x100 = (int16_t)((p[18] << 8) | p[19]);
    dp->latitude  = (float)lat_x100 / 100.0f;
    dp->longitude = (float)lon_x100 / 100.0f;

    // UTC offset
    dp->offset_minutes = (int16_t)((p[20] << 8) | p[21]);

    // Day label
    dp->day_label = (p[22] == 255) ? -1 : (int8_t)p[22];

    // Night flag
    dp->is_night = (p[23] != 0);
  }

  s_num_cities = received;
}

int time_traveler_num_data_points(void) {
  int count = s_num_cities;
  // Only add the synthetic "CURRENT LOCATION" entry when we have GPS
  // but are NOT matched to a pinned city
  if (s_has_user_location && s_user_matched_city_index < 0) {
    count++;
  }
  return count;
}

static int prv_get_user_location_insert_index(void) {
  if (!s_has_user_location || s_user_matched_city_index >= 0)
    return -1;

  // Find where to insert user location based on longitude
  int insert_idx = 0;
  for (int i = 0; i < s_num_cities; i++) {
    if (s_user_lon < s_cities[i].longitude) {
      break;
    }
    insert_idx++;
  }
  return insert_idx;
}

WorldClockDataPoint *time_traveler_data_point_at(int idx) {
  if (idx < 0 || idx >= time_traveler_num_data_points()) {
    return NULL;
  }

  // Only insert the synthetic entry when not matched to a pinned city
  if (s_has_user_location && s_user_matched_city_index < 0) {
    int user_idx = prv_get_user_location_insert_index();
    if (idx == user_idx) {
      return &s_user_location_dp;
    }
    if (idx > user_idx) {
      idx--;
    }
  }

  if (idx < 0 || idx >= s_num_cities) return NULL;
  return &s_cities[idx];
}

static CityCoordinates s_actual_user_coords;
static CityCoordinates s_city_coord_ret;

CityCoordinates *time_traveler_get_city_coordinates(int city_index) {
  if (city_index < 0 || city_index >= time_traveler_num_data_points()) {
    return NULL;
  }

  if (s_has_user_location && s_user_matched_city_index < 0) {
    // Synthetic user location entry is in the list
    int user_idx = prv_get_user_location_insert_index();
    if (city_index == user_idx) {
      s_actual_user_coords.latitude = s_user_lat;
      s_actual_user_coords.longitude = s_user_lon;
      return &s_actual_user_coords;
    }
    if (city_index > user_idx) {
      city_index--;
    }
  }

  if (city_index < 0 || city_index >= s_num_cities) return NULL;

  // If this is the matched city, return real GPS coords for map accuracy
  if (s_has_user_location && city_index == s_user_matched_city_index) {
    s_actual_user_coords.latitude = s_user_lat;
    s_actual_user_coords.longitude = s_user_lon;
    return &s_actual_user_coords;
  }

  s_city_coord_ret.latitude  = s_cities[city_index].latitude;
  s_city_coord_ret.longitude = s_cities[city_index].longitude;
  return &s_city_coord_ret;
}

int time_traveler_index_of_data_point(WorldClockDataPoint *dp) {
  for (int i = 0; i < time_traveler_num_data_points(); i++) {
    if (dp == time_traveler_data_point_at(i)) {
      return i;
    }
  }
  return -1;
}

WorldClockDataPoint *time_traveler_data_point_delta(WorldClockDataPoint *dp,
                                                    int delta) {
  int idx = time_traveler_index_of_data_point(dp);
  if (idx < 0) {
    return NULL;
  }
  int num = time_traveler_num_data_points();
  return time_traveler_data_point_at((idx + delta + num) % num);
}
