/*
 * Copyright (c) 2015 Pebble Technology
 */

#pragma once

#include <pebble.h>

struct WorldClockMainWindowViewModel;

typedef void (*WorldClockMainWindowViewModelFunc)(struct WorldClockMainWindowViewModel* model);

typedef struct {
  WorldClockMainWindowViewModelFunc announce_changed;
  struct {
    GColor top;
    GColor bottom;
    int32_t to_bottom_normalized;
  } bg_color;
  char *city;
  struct {
    int16_t value;
    char text[8];
  } temperature;
  struct {
    GDrawCommandImage *draw_command;
    int32_t to_square_normalized;
  } icon;
  struct {
    int16_t idx;
    int16_t num;
    char text[8];
  } pagination;
  struct {
    int16_t high;
    int16_t low;
    char text[20];
  } highlow;
  char *description;
} WorldClockMainWindowViewModel;

//! calls model's .announce_changed or does nothing if NULL
void world_clock_main_window_view_model_announce_changed(WorldClockMainWindowViewModel *model);

typedef struct {
  char *city;
  char *description;
  int icon;
  int16_t current;
  int16_t high;
  int16_t low;
} WorldClockDataPoint;

typedef struct {
  int16_t temperature;
  int16_t low;
  int16_t high;
} WorldClockDataViewNumbers;


void world_clock_view_model_set_highlow(WorldClockMainWindowViewModel *model, int16_t high, int16_t low);

void world_clock_view_model_set_temperature(WorldClockMainWindowViewModel *model, int16_t value);
void world_clock_view_model_set_icon(WorldClockMainWindowViewModel *model, GDrawCommandImage *image);

WorldClockDataViewNumbers world_clock_data_point_view_model_numbers(WorldClockDataPoint *data_point);

GDrawCommandImage *world_clock_data_point_create_icon(WorldClockDataPoint *data_point);

void world_clock_view_model_fill_strings_and_pagination(WorldClockMainWindowViewModel *view_model, WorldClockDataPoint *data_point);

void world_clock_view_model_fill_numbers(WorldClockMainWindowViewModel *model, WorldClockDataViewNumbers numbers);

void world_clock_view_model_fill_all(WorldClockMainWindowViewModel *model, WorldClockDataPoint *data_point);

void world_clock_view_model_fill_colors(WorldClockMainWindowViewModel *model, GColor color);

void world_clock_view_model_deinit(WorldClockMainWindowViewModel *model);

GColor world_clock_data_point_color(WorldClockDataPoint *data_point);

int world_clock_num_data_points(void);

WorldClockDataPoint *world_clock_data_point_at(int idx);
WorldClockDataPoint *world_clock_data_point_delta(WorldClockDataPoint *dp, int delta);
