#pragma once

#include <pebble.h>

struct WorldClockMainWindowViewModel;

typedef void (*WorldClockMainWindowViewModelFunc)(
    struct WorldClockMainWindowViewModel *model);

typedef struct {
  WorldClockMainWindowViewModelFunc announce_changed;
  struct {
    GColor top;
    GColor bottom;
    int32_t to_bottom_normalized;
  } bg_color;
  char *city;
  struct {
    int16_t hour;
    int16_t minute;
    char text[16]; // Increased for safety
  } time;
  struct {
    char text[8]; // Increased for safety
  } meridiem;
  struct {
    int16_t idx;
    int16_t num;
    char text[16]; // Increased for safety
  } pagination;
  struct {
    char text[36]; // Increased for safety (TODAY, +12:34 HRS = ~20 chars)
  } relative_info;
  int16_t current_offset; // current offset for animation
  bool is_night;
  GColor text_color;
  GColor crosshair_color;
  GColor dot_fill_color;
  GColor dot_outline_color;
  GColor statusbar_text_color;
  GColor ruler_color;
} WorldClockMainWindowViewModel;

//! calls model's .announce_changed or does nothing if NULL
void time_traveler_main_window_view_model_announce_changed(
    WorldClockMainWindowViewModel *model);

typedef struct {
  char city[20];         // City name (from JS, or "CURRENT LOCATION" for GPS city)
  float latitude;        // City latitude (from JS)
  float longitude;       // City longitude (from JS)
  int16_t offset_minutes; // current offset from UTC in minutes (from JS)
  int8_t day_label;       // -1 = yesterday, 0 = today, 1 = tomorrow (from JS)
  bool is_night;          // whether city is currently in nighttime (from JS)
} WorldClockDataPoint;

typedef struct {
  int16_t hour;
  int16_t minute;
  int16_t offset;
} WorldClockDataViewNumbers;

void time_traveler_view_model_set_time(WorldClockMainWindowViewModel *model,
                                        int16_t hour, int16_t minute);
void time_traveler_view_model_set_relative_info(
    WorldClockMainWindowViewModel *model, int16_t offset_hours,
    WorldClockDataPoint *data_point);

WorldClockDataViewNumbers
time_traveler_data_point_view_model_numbers(WorldClockDataPoint *data_point);

void time_traveler_view_model_fill_strings_and_pagination(
    WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *data_point);

void time_traveler_view_model_fill_numbers(
    WorldClockMainWindowViewModel *model, WorldClockDataViewNumbers numbers,
    WorldClockDataPoint *data_point);

void time_traveler_view_model_fill_all(WorldClockMainWindowViewModel *model,
                                        WorldClockDataPoint *data_point);

void time_traveler_view_model_fill_colors(WorldClockMainWindowViewModel *model,
                                           GColor color);

void time_traveler_view_model_fill_night_mode(
    WorldClockMainWindowViewModel *model, bool is_night);
void time_traveler_view_model_fill_loading(
    WorldClockMainWindowViewModel *model);

GColor time_traveler_data_point_color(WorldClockDataPoint *data_point,
                                       bool is_night);

int time_traveler_num_data_points(void);

void time_traveler_data_set_user_location(float lat, float lon);
void time_traveler_data_estimate_location_from_timezone(void);
bool time_traveler_data_has_user_location(void);
bool time_traveler_data_location_is_estimated(void);
void time_traveler_data_get_user_location(float *lat, float *lon);
bool time_traveler_data_is_user_location(WorldClockDataPoint *dp);

// Request user UTC offset via JS
// Using this instead of tm_gmtoff to attempt to fix offset bug
void time_traveler_data_set_user_utc_offset_minutes(int16_t minutes);
bool time_traveler_data_has_user_utc_offset_minutes(void);
int16_t time_traveler_data_get_user_utc_offset_minutes(void);

// Matched city: when user GPS is near a pinned city, merge them
void time_traveler_data_set_user_matched_city(int index);
int time_traveler_data_get_user_matched_city(void);
void time_traveler_data_clear_user_matched_city(void);
void time_traveler_data_set_date_format(int8_t format);

WorldClockDataPoint *time_traveler_data_point_at(int idx);
WorldClockDataPoint *time_traveler_data_point_delta(WorldClockDataPoint *dp,
                                                     int delta);
int time_traveler_index_of_data_point(WorldClockDataPoint *dp);

// City coordinates for map display
typedef struct {
  float longitude;
  float latitude;
} CityCoordinates;

CityCoordinates *time_traveler_get_city_coordinates(int city_index);

// City blob format constants
#define MAX_JS_CITIES 50
#define CITY_BLOB_BYTES_PER_CITY 24

// Returns the index of the user location entry, or -1 if not present
int time_traveler_data_find_user_location_index(void);

// Apply binary blob from JS: CITY_BLOB_BYTES_PER_CITY bytes per city
void time_traveler_data_apply_js_blob(const uint8_t *blob, uint16_t length);
