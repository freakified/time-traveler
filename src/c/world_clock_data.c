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
    snprintf(model->time.text, sizeof(model->time.text), "%02d:%02d", model->time.hour, model->time.minute);
    model->time.ampm[0] = '\0';
  } else {
    int display_hour = hour % 12;
    if (display_hour == 0) display_hour = 12;
    const char *ampm = (hour < 12) ? "AM" : "PM";
    snprintf(model->time.text, sizeof(model->time.text), "%d:%02d", display_hour, model->time.minute);
    strcpy(model->time.ampm, ampm);
  }
}

void world_clock_view_model_set_relative_info(WorldClockMainWindowViewModel *model, int16_t relative_offset_minutes, WorldClockDataPoint *data_point) {
  int16_t abs_minutes = (relative_offset_minutes < 0) ? -relative_offset_minutes : relative_offset_minutes;
  int16_t hours = abs_minutes / 60;
  int16_t mins = abs_minutes % 60;
  int sign = (relative_offset_minutes >= 0) ? 1 : -1;

  char day_str[10];
  if (data_point->day_label == -1) {
    strcpy(day_str, "YESTERDAY");
  } else if (data_point->day_label == 1) {
    strcpy(day_str, "TOMORROW");
  } else {
    strcpy(day_str, "TODAY");
  }

  char offset_str[12];
  if (mins == 0) {
    snprintf(offset_str, sizeof(offset_str), "%c%d HRS", sign > 0 ? '+' : '-', hours);
  } else {
    snprintf(offset_str, sizeof(offset_str), "%c%d:%02d HRS", sign > 0 ? '+' : '-', hours, mins);
  }

  snprintf(model->relative_info.text, sizeof(model->relative_info.text), "%s, %s", day_str, offset_str);

  model->current_offset = relative_offset_minutes / 60;
}

WorldClockDataViewNumbers world_clock_data_point_view_model_numbers(WorldClockDataPoint *data_point) {
  time_t now = time(NULL);
  struct tm *current_local = localtime(&now);

  time_t gmt_seconds = now - current_local->tm_gmtoff;
  int16_t local_offset_minutes = current_local->tm_gmtoff / 60;

  time_t city_seconds = gmt_seconds + (data_point->offset_minutes * 60);
  struct tm *city_time = localtime(&city_seconds);

  int16_t relative_offset_minutes = data_point->offset_minutes - local_offset_minutes;

  return (WorldClockDataViewNumbers){
      .hour = city_time->tm_hour,
      .offset = relative_offset_minutes,
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

GColor world_clock_data_point_color(WorldClockDataPoint *data_point,
                                    bool is_night) {
  return is_night ? COLOR_APP_BACKGROUND_NIGHT : COLOR_APP_BACKGROUND;
}

void world_clock_view_model_fill_all(WorldClockMainWindowViewModel *model, WorldClockDataPoint *data_point) {
  WorldClockMainWindowViewModelFunc annouce_changed = model->announce_changed;
  memset(model, 0, sizeof(*model));
  model->announce_changed = annouce_changed;
  world_clock_view_model_fill_strings_and_pagination(model, data_point);
  world_clock_view_model_fill_colors(model, world_clock_data_point_color(data_point, false));
  world_clock_view_model_fill_night_mode(model, false);
  world_clock_view_model_fill_numbers(model, world_clock_data_point_view_model_numbers(data_point), data_point);

  world_clock_main_window_view_model_announce_changed(model);
}

void world_clock_view_model_fill_night_mode(WorldClockMainWindowViewModel *model, bool is_night) {
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
}

void world_clock_view_model_deinit(WorldClockMainWindowViewModel *model) {
}

// ============================================================
// 49 Casio cities: names only (offsets/day/night come from JS)
// ============================================================
WorldClockDataPoint s_data_points[] = {
    { .city = "PAGO PAGO" },
    { .city = "HONOLULU" },
    { .city = "ANCHORAGE" },
    { .city = "VANCOUVER" },
    { .city = "SAN FRANCISCO" },
    { .city = "EDMONTON" },
    { .city = "DENVER" },
    { .city = "MEXICO CITY" },
    { .city = "CHICAGO" },
    { .city = "NEW YORK" },
    { .city = "SANTIAGO" },
    { .city = "HALIFAX" },
    { .city = "ST. JOHNS" },
    { .city = "RIO DE JANEIRO" },
    { .city = "F. DE NORONHA" },
    { .city = "PRAIA" },
    { .city = "UTC" },
    { .city = "LISBON" },
    { .city = "LONDON" },
    { .city = "MADRID" },
    { .city = "PARIS" },
    { .city = "ROME" },
    { .city = "BERLIN" },
    { .city = "STOCKHOLM" },
    { .city = "ATHENS" },
    { .city = "CAIRO" },
    { .city = "JERUSALEM" },
    { .city = "MOSCOW" },
    { .city = "JEDDAH" },
    { .city = "TEHRAN" },
    { .city = "DUBAI" },
    { .city = "KABUL" },
    { .city = "KARACHI" },
    { .city = "DELHI" },
    { .city = "KATHMANDU" },
    { .city = "DHAKA" },
    { .city = "YANGON" },
    { .city = "BANGKOK" },
    { .city = "SINGAPORE" },
    { .city = "HONG KONG" },
    { .city = "BEIJING" },
    { .city = "TAIPEI" },
    { .city = "SEOUL" },
    { .city = "TOKYO" },
    { .city = "ADELAIDE" },
    { .city = "GUAM" },
    { .city = "SYDNEY" },
    { .city = "NOUMEA" },
    { .city = "WELLINGTON" },
};

// City coordinates for map display
static CityCoordinates s_city_coordinates[] = {
    {-170.70, -14.27}, // Pago Pago
    {-157.86,  21.31}, // Honolulu
    {-149.90,  61.22}, // Anchorage
    {-123.12,  49.28}, // Vancouver
    {-122.42,  37.77}, // San Francisco
    {-113.49,  53.54}, // Edmonton
    {-104.99,  39.74}, // Denver
    { -99.13,  19.43}, // Mexico City
    { -87.63,  41.88}, // Chicago
    { -74.01,  40.71}, // New York
    { -70.67, -33.45}, // Santiago
    { -63.57,  44.65}, // Halifax
    { -52.71,  47.56}, // St. Johns
    { -43.17, -22.91}, // Rio De Janeiro
    { -32.42,  -3.86}, // Fernando de Noronha
    { -23.51,  14.92}, // Praia
    {   0.00,   0.00}, // UTC
    {  -9.14,  38.72}, // Lisbon
    {  -0.13,  51.51}, // London
    {  -3.70,  40.42}, // Madrid
    {   2.35,  48.86}, // Paris
    {  12.50,  41.90}, // Rome
    {  13.41,  52.52}, // Berlin
    {  18.07,  59.33}, // Stockholm
    {  23.73,  37.98}, // Athens
    {  31.24,  30.04}, // Cairo
    {  35.23,  31.77}, // Jerusalem
    {  37.62,  55.76}, // Moscow
    {  39.19,  21.49}, // Jeddah
    {  51.39,  35.69}, // Tehran
    {  55.27,  25.20}, // Dubai
    {  69.17,  34.53}, // Kabul
    {  67.01,  24.86}, // Karachi
    {  77.21,  28.61}, // Delhi
    {  85.32,  27.72}, // Kathmandu
    {  90.41,  23.81}, // Dhaka
    {  96.20,  16.87}, // Yangon
    { 100.50,  13.76}, // Bangkok
    { 103.82,   1.35}, // Singapore
    { 114.17,  22.32}, // Hong Kong
    { 116.40,  39.90}, // Beijing
    { 121.57,  25.03}, // Taipei
    { 126.98,  37.57}, // Seoul
    { 139.69,  35.68}, // Tokyo
    { 138.60, -34.93}, // Adelaide
    { 144.79,  13.44}, // Guam
    { 151.21, -33.87}, // Sydney
    { 166.46, -22.28}, // Noumea
    { 174.78, -41.29}, // Wellington
};

// Apply binary blob from JS: 4 bytes per city
//   bytes 0-1: offset_minutes (int16, big-endian)
//   byte 2:    day_label (0=today, 1=tomorrow, 255=yesterday)
//   byte 3:    is_night (0 or 1)
void world_clock_data_apply_js_blob(const uint8_t *blob, uint16_t length) {
  int num_cities = world_clock_num_data_points();
  int received = length / 4;
  int count = (received < num_cities) ? received : num_cities;

  for (int i = 0; i < count; i++) {
    const uint8_t *p = &blob[i * 4];
    int16_t offset = (int16_t)((p[0] << 8) | p[1]);
    int8_t day_label = (p[2] == 255) ? -1 : (int8_t)p[2];
    bool is_night = (p[3] != 0);

    s_data_points[i].offset_minutes = offset;
    s_data_points[i].day_label = day_label;
    s_data_points[i].is_night = is_night;
  }
}

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
