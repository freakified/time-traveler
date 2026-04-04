#pragma once

#include <pebble.h>

// Color definitions for easy customization
#define COLOR_MAP_FOREGROUND PBL_IF_COLOR_ELSE(GColorWhite, GColorWhite)
#define COLOR_MAP_BACKGROUND                                                   \
  PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorLightGray)
#define COLOR_MAP_NIGHT_FOREGROUND                                             \
  PBL_IF_COLOR_ELSE(GColorLightGray, GColorLightGray)
#define COLOR_MAP_NIGHT_BACKGROUND                                             \
  PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack)
#define COLOR_MAP_NIGHT_OVERLAY PBL_IF_COLOR_ELSE(GColorBlack, GColorLightGray)
#define COLOR_CROSSHAIR PBL_IF_COLOR_ELSE(GColorBlack, GColorWhite)
#define COLOR_DOT_FILL GColorWhite
#define COLOR_DOT_OUTLINE GColorBlack
#define COLOR_TEXT_DEFAULT GColorBlack
#define COLOR_RULER GColorBlack
#define COLOR_STATUSBAR_TEXT GColorBlack
#define COLOR_APP_BACKGROUND PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite)

// Night Palette Definitions
#define COLOR_MAP_FOREGROUND_NIGHT                                             \
  PBL_IF_COLOR_ELSE(GColorLightGray, GColorLightGray)
#define COLOR_MAP_BACKGROUND_NIGHT                                             \
  PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack)
#define COLOR_TEXT_DEFAULT_NIGHT PBL_IF_COLOR_ELSE(GColorWhite, GColorWhite)
#define COLOR_CROSSHAIR_NIGHT PBL_IF_COLOR_ELSE(GColorWhite, GColorWhite)
#define COLOR_DOT_FILL_NIGHT PBL_IF_COLOR_ELSE(GColorWhite, GColorWhite)
#define COLOR_DOT_OUTLINE_NIGHT PBL_IF_COLOR_ELSE(GColorBlack, GColorBlack)
#define COLOR_STATUSBAR_TEXT_NIGHT GColorWhite
#define COLOR_RULER_NIGHT GColorWhite
#define COLOR_APP_BACKGROUND_NIGHT                                             \
  PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack)

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
    char text[25]; // "TODAY, +3 HRS" or "YESTERDAY, -5 HRS"
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
void time_traveller_main_window_view_model_announce_changed(
    WorldClockMainWindowViewModel *model);

typedef struct {
  char *city;
  int16_t offset_minutes; // current offset from UTC in minutes (from JS)
  int8_t day_label;       // -1 = yesterday, 0 = today, 1 = tomorrow (from JS)
  bool is_night;          // whether city is currently in nighttime (from JS)
} WorldClockDataPoint;

typedef struct {
  int16_t hour;
  int16_t minute;
  int16_t offset;
} WorldClockDataViewNumbers;

void time_traveller_view_model_set_time(WorldClockMainWindowViewModel *model,
                                        int16_t hour, int16_t minute);
void time_traveller_view_model_set_relative_info(
    WorldClockMainWindowViewModel *model, int16_t offset_hours,
    WorldClockDataPoint *data_point);

WorldClockDataViewNumbers
time_traveller_data_point_view_model_numbers(WorldClockDataPoint *data_point);

GDrawCommandImage *
time_traveller_data_point_create_icon(WorldClockDataPoint *data_point);

void time_traveller_view_model_fill_strings_and_pagination(
    WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *data_point);

void time_traveller_view_model_fill_numbers(
    WorldClockMainWindowViewModel *model, WorldClockDataViewNumbers numbers,
    WorldClockDataPoint *data_point);

void time_traveller_view_model_fill_all(WorldClockMainWindowViewModel *model,
                                        WorldClockDataPoint *data_point);

void time_traveller_view_model_fill_colors(WorldClockMainWindowViewModel *model,
                                           GColor color);

void time_traveller_view_model_fill_night_mode(
    WorldClockMainWindowViewModel *model, bool is_night);

void time_traveller_view_model_deinit(WorldClockMainWindowViewModel *model);

GColor time_traveller_data_point_color(WorldClockDataPoint *data_point,
                                       bool is_night);

int time_traveller_num_data_points(void);

WorldClockDataPoint *time_traveller_data_point_at(int idx);
WorldClockDataPoint *time_traveller_data_point_delta(WorldClockDataPoint *dp,
                                                     int delta);
int time_traveller_index_of_data_point(WorldClockDataPoint *dp);

// City coordinates for map display
typedef struct {
  float longitude;
  float latitude;
} CityCoordinates;

CityCoordinates *time_traveller_get_city_coordinates(int city_index);

extern WorldClockDataPoint s_data_points[];

// Dynamic city data from JS
void time_traveller_data_apply_js_blob(const uint8_t *blob, uint16_t length);
