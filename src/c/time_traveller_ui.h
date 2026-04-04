#pragma once

#include <pebble.h>
#include "time_traveller_data.h"

uint8_t time_traveller_ui_palette_size(GBitmapFormat format);

uint8_t time_traveller_ui_luminance_steps(GColor color);

uint8_t time_traveller_ui_blend_channel(uint8_t background, uint8_t foreground,
                                     uint8_t luminance);

GColor time_traveller_ui_palette_color_for_luminance(uint8_t luminance,
                                                  GColor background,
                                                  GColor foreground);

void time_traveller_ui_recolor(GBitmap *bitmap);

void time_traveller_ui_recolor_night(GBitmap *bitmap);

int16_t time_traveller_ui_clamp_x(int16_t x, int16_t width);

void time_traveller_ui_draw_segment(GContext *ctx, int16_t y, int16_t start_x,
                                 int16_t end_x, GColor color);

void time_traveller_ui_draw_bitmap_segment(GContext *ctx, const GBitmap *bitmap,
                                        const GRect map_rect, int16_t row,
                                        int16_t start_x, int16_t end_x);

GRect init_text_layer(Layer *parent_layer, TextLayer **text_layer, int16_t y,
                      int16_t h, int16_t additional_right_margin,
                      char *font_key);

void init_statusbar_text_layer(Layer *parent, TextLayer **layer);
