#include "time_traveller_ui.h"
#include <string.h>

#define MARGIN 8

uint8_t time_traveller_ui_palette_size(GBitmapFormat format) {
  switch (format) {
  case GBitmapFormat1BitPalette:
    return 2;
  case GBitmapFormat2BitPalette:
    return 4;
  case GBitmapFormat4BitPalette:
    return 16;
  default:
    return 0;
  }
}

uint8_t time_traveller_ui_luminance_steps(GColor color) {
  const uint16_t total = color.r + color.g + color.b;
  return (uint8_t)((total + 1) / 3);
}

uint8_t time_traveller_ui_blend_channel(uint8_t background, uint8_t foreground,
                                     uint8_t luminance) {
  return (uint8_t)((background * (3 - luminance) + foreground * luminance + 1) /
                   3);
}

GColor time_traveller_ui_palette_color_for_luminance(uint8_t luminance,
                                                  GColor background,
                                                  GColor foreground) {
  if (luminance == 0) {
    return background;
  }
  if (luminance >= 3) {
    return foreground;
  }

  return (GColor){
      .a = 3,
      .r = time_traveller_ui_blend_channel(background.r, foreground.r, luminance),
      .g = time_traveller_ui_blend_channel(background.g, foreground.g, luminance),
      .b = time_traveller_ui_blend_channel(background.b, foreground.b, luminance),
  };
}

void time_traveller_ui_recolor(GBitmap *bitmap) {
  if (!bitmap) {
    return;
  }

  const GColor background = COLOR_MAP_BACKGROUND;
  const GColor foreground = COLOR_MAP_FOREGROUND;

  GColor *palette = gbitmap_get_palette(bitmap);
  if (!palette) {
    return;
  }

  const uint8_t palette_size =
      time_traveller_ui_palette_size(gbitmap_get_format(bitmap));
  for (uint8_t i = 0; i < palette_size; ++i) {
    const uint8_t luminance = time_traveller_ui_luminance_steps(palette[i]);
    GColor color = time_traveller_ui_palette_color_for_luminance(
        luminance, background, foreground);
    color.a = palette[i].a;
    palette[i] = color;
  }
}

void time_traveller_ui_recolor_night(GBitmap *bitmap) {
  const GColor background = COLOR_MAP_NIGHT_BACKGROUND;
  const GColor foreground = COLOR_MAP_NIGHT_FOREGROUND;
  if (!bitmap) {
    return;
  }

  GColor *palette = gbitmap_get_palette(bitmap);
  if (!palette) {
    return;
  }

  const uint8_t palette_size =
      time_traveller_ui_palette_size(gbitmap_get_format(bitmap));
  for (uint8_t i = 0; i < palette_size; ++i) {
    const uint8_t luminance = time_traveller_ui_luminance_steps(palette[i]);
    GColor color = time_traveller_ui_palette_color_for_luminance(
        luminance, background, foreground);
    color.a = palette[i].a;
    palette[i] = color;
  }
}

int16_t time_traveller_ui_clamp_x(int16_t x, int16_t width) {
  if (x < 0) {
    return 0;
  }
  if (x >= width) {
    return width - 1;
  }
  return x;
}

void time_traveller_ui_draw_segment(GContext *ctx, int16_t y, int16_t start_x,
                                 int16_t end_x, GColor color) {
  if (start_x > end_x) {
    return;
  }

  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, GRect(start_x, y, end_x - start_x + 1, 1), 0,
                     GCornerNone);
}

void time_traveller_ui_draw_bitmap_segment(GContext *ctx, const GBitmap *bitmap,
                                        const GRect map_rect, int16_t row,
                                        int16_t start_x, int16_t end_x) {
  if (!ctx || !bitmap || start_x > end_x) {
    return;
  }

  const GRect segment_rect = GRect(start_x, row, end_x - start_x + 1, 1);
  GBitmap *segment = gbitmap_create_as_sub_bitmap(bitmap, segment_rect);
  if (!segment) {
    return;
  }

  graphics_draw_bitmap_in_rect(ctx, segment,
                               GRect(map_rect.origin.x + start_x,
                                     map_rect.origin.y + row,
                                     segment_rect.size.w, segment_rect.size.h));
  gbitmap_destroy(segment);
}

GRect init_text_layer(Layer *parent_layer, TextLayer **text_layer, int16_t y,
                      int16_t h, int16_t additional_right_margin,
                      char *font_key) {
  const int16_t font_compensator =
      strcmp(font_key, FONT_KEY_LECO_38_BOLD_NUMBERS) == 0 ? 3 : 1;

  const int16_t current_margin = PBL_IF_ROUND_ELSE(4, MARGIN);

  const GRect frame =
      GRect(current_margin - font_compensator, y,
            layer_get_bounds(parent_layer).size.w - 2 * current_margin +
                2 * font_compensator - additional_right_margin,
            h);

  *text_layer = text_layer_create(frame);
  text_layer_set_background_color(*text_layer, GColorClear);
  text_layer_set_text_color(*text_layer, COLOR_TEXT_DEFAULT);
  text_layer_set_font(*text_layer, fonts_get_system_font(font_key));

  if (PBL_IF_ROUND_ELSE(true, false)) {
    text_layer_set_text_alignment(*text_layer, GTextAlignmentCenter);
  }

  layer_add_child(parent_layer, text_layer_get_layer(*text_layer));
  return frame;
}

void init_statusbar_text_layer(Layer *parent, TextLayer **layer) {
  const int16_t status_margin = PBL_IF_ROUND_ELSE(8, MARGIN);
  const GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  const GRect status_bounds =
      GRect(0, 0, layer_get_bounds(parent).size.w - 2 * status_margin,
            STATUS_BAR_LAYER_HEIGHT);
  const GSize text_size = graphics_text_layout_get_content_size(
      "00:00 PM", font, status_bounds, GTextOverflowModeTrailingEllipsis,
      GTextAlignmentCenter);
  const int16_t text_height =
      text_size.h > 0 ? text_size.h : STATUS_BAR_LAYER_HEIGHT;
  const int16_t status_y = ((STATUS_BAR_LAYER_HEIGHT - text_height) / 2) - 1;
  const GRect frame =
      GRect(status_margin, status_y, status_bounds.size.w, text_height);

  *layer = text_layer_create(frame);
  text_layer_set_background_color(*layer, GColorClear);
  text_layer_set_text_color(*layer, COLOR_STATUSBAR_TEXT);
  text_layer_set_font(*layer, font);
  layer_add_child(parent, text_layer_get_layer(*layer));

  text_layer_set_text_alignment(*layer, GTextAlignmentCenter);
}
