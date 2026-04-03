#pragma once

#include <pebble.h>

// Color definitions for easy customization
#define COLOR_MAP_FOREGROUND PBL_IF_COLOR_ELSE(GColorWhite, GColorLightGray)
#define COLOR_MAP_BACKGROUND PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorBlack)
#define COLOR_MAP_NIGHT_FOREGROUND                                             \
  PBL_IF_COLOR_ELSE(GColorLightGray, GColorLightGray)
#define COLOR_MAP_NIGHT_BACKGROUND                                             \
  PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack)
#define COLOR_MAP_NIGHT_OVERLAY PBL_IF_COLOR_ELSE(GColorBlack, GColorBlack)
#define COLOR_MAP_TWILIGHT GColorWhite
#define COLOR_CROSSHAIR PBL_IF_COLOR_ELSE(GColorBlack, GColorWhite)
#define COLOR_DOT_FILL GColorWhite
#define COLOR_DOT_OUTLINE GColorBlack
#define COLOR_TEXT_DEFAULT GColorBlack
#define COLOR_RULER GColorBlack
#define COLOR_STATUSBAR_TEXT GColorBlack
#define COLOR_APP_BACKGROUND PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorBlack)

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
    char text[12]; // "HH:MM" or "H:MM" format (time numbers only)
    char ampm[4];  // "AM", "PM", or empty for 24h format
  } time;
  struct {
    int16_t idx;
    int16_t num;
    char text[8];
  } pagination;
  struct {
    char text[25]; // "TODAY, +3 HRS DST" or "YESTERDAY, -5 HRS DST"
  } relative_info;
  int16_t current_offset; // current offset for animation
} WorldClockMainWindowViewModel;

//! calls model's .announce_changed or does nothing if NULL
void world_clock_main_window_view_model_announce_changed(
    WorldClockMainWindowViewModel *model);

typedef struct {
  char *city;
  int16_t offset_hours; // offset from local time in hours
  bool is_dst;          // true if currently in DST
} WorldClockDataPoint;

typedef struct {
  int16_t hour;
  int16_t minute;
  int16_t offset;
} WorldClockDataViewNumbers;

void world_clock_view_model_set_time(WorldClockMainWindowViewModel *model,
                                     int16_t hour, int16_t minute);
void world_clock_view_model_set_relative_info(
    WorldClockMainWindowViewModel *model, int16_t offset_hours,
    WorldClockDataPoint *data_point);

WorldClockDataViewNumbers
world_clock_data_point_view_model_numbers(WorldClockDataPoint *data_point);

GDrawCommandImage *
world_clock_data_point_create_icon(WorldClockDataPoint *data_point);

void world_clock_view_model_fill_strings_and_pagination(
    WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *data_point);

void world_clock_view_model_fill_numbers(WorldClockMainWindowViewModel *model,
                                         WorldClockDataViewNumbers numbers,
                                         WorldClockDataPoint *data_point);

void world_clock_view_model_fill_all(WorldClockMainWindowViewModel *model,
                                     WorldClockDataPoint *data_point);

void world_clock_view_model_fill_colors(WorldClockMainWindowViewModel *model,
                                        GColor color);

void world_clock_view_model_deinit(WorldClockMainWindowViewModel *model);

GColor world_clock_data_point_color(WorldClockDataPoint *data_point);

int world_clock_num_data_points(void);

WorldClockDataPoint *world_clock_data_point_at(int idx);
WorldClockDataPoint *world_clock_data_point_delta(WorldClockDataPoint *dp,
                                                  int delta);

// City coordinates for map display
typedef struct {
  float longitude;
  float latitude;
} CityCoordinates;

CityCoordinates *world_clock_get_city_coordinates(int city_index);

extern WorldClockDataPoint s_data_points[];
