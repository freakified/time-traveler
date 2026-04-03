#include "message_keys.auto.h"
#include "world_clock_animations.h"
#include "world_clock_data.h"
#include "world_clock_private.h"
#include <pebble.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for functions not in headers yet
int world_clock_index_of_data_point(WorldClockDataPoint *dp);
CityCoordinates *world_clock_get_city_coordinates(int city_index);
static GRect calibrated_map_rect(const WorldClockData *data);

#define DST_PERSIST_KEY 1
#define OVERLAY_PERSIST_KEY 2

static Window *s_main_window;

#define MARGIN 8
#define WORLD_MAP_BOTTOM_TRIM 10
#define WORLD_MAP_MAX_OVERLAY_ROWS 104

typedef struct {
  uint32_t version;
  uint16_t map_width;
  uint16_t map_height;
  uint16_t total_rows;
  uint8_t daylight_start[WORLD_MAP_MAX_OVERLAY_ROWS];
  uint8_t daylight_end[WORLD_MAP_MAX_OVERLAY_ROWS];
} OverlayBackup;
#define WORLD_MAP_OVERLAY_FULL_NIGHT 255
#define WORLD_MAP_OVERLAY_ROWS_BUFFER_SIZE 128
#define WORLD_MAP_APP_MESSAGE_INBOX_SIZE 768
#define WORLD_MAP_APP_MESSAGE_OUTBOX_SIZE 64

#if defined(PBL_PLATFORM_GABBRO)
#define WORLD_MAP_TOP 45
#else
#define WORLD_MAP_TOP STATUS_BAR_LAYER_HEIGHT
#endif

static uint8_t prv_world_map_palette_size(GBitmapFormat format) {
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

static uint8_t prv_world_map_luminance_steps(GColor color) {
  const uint16_t total = color.r + color.g + color.b;
  return (uint8_t)((total + 1) / 3);
}

static uint8_t prv_world_map_blend_channel(uint8_t background,
                                           uint8_t foreground,
                                           uint8_t luminance) {
  return (uint8_t)((background * (3 - luminance) + foreground * luminance + 1) /
                   3);
}

static GColor prv_world_map_palette_color_for_luminance(uint8_t luminance,
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
      .r = prv_world_map_blend_channel(background.r, foreground.r, luminance),
      .g = prv_world_map_blend_channel(background.g, foreground.g, luminance),
      .b = prv_world_map_blend_channel(background.b, foreground.b, luminance),
  };
}

static void prv_world_map_recolor(GBitmap *bitmap) {
  const GColor background = COLOR_MAP_BACKGROUND;
  const GColor foreground = COLOR_MAP_FOREGROUND;
  if (!bitmap) {
    return;
  }

  GColor *palette = gbitmap_get_palette(bitmap);
  if (!palette) {
    return;
  }

  const uint8_t palette_size =
      prv_world_map_palette_size(gbitmap_get_format(bitmap));
  for (uint8_t i = 0; i < palette_size; ++i) {
    const uint8_t luminance = prv_world_map_luminance_steps(palette[i]);
    GColor color = prv_world_map_palette_color_for_luminance(
        luminance, background, foreground);
    color.a = palette[i].a;
    palette[i] = color;
  }
}

static void prv_world_map_recolor_night(GBitmap *bitmap) {
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
      prv_world_map_palette_size(gbitmap_get_format(bitmap));
  for (uint8_t i = 0; i < palette_size; ++i) {
    const uint8_t luminance = prv_world_map_luminance_steps(palette[i]);
    GColor color = prv_world_map_palette_color_for_luminance(
        luminance, background, foreground);
    color.a = palette[i].a;
    palette[i] = color;
  }
}

static uint8_t prv_world_map_overlay_twilight_width(void) {
  return PBL_IF_COLOR_ELSE(0, 0);
}

static int16_t prv_world_map_clamp_x(int16_t x, int16_t width) {
  if (x < 0) {
    return 0;
  }
  if (x >= width) {
    return width - 1;
  }
  return x;
}

static bool prv_world_map_should_draw_twilight_pixel(int16_t x, int16_t y,
                                                     bool wrapped_segment) {
  if (PBL_IF_COLOR_ELSE(true, false)) {
    return true;
  }

  const int parity = (x + y + (wrapped_segment ? 1 : 0)) & 1;
  return parity == 0;
}

static void prv_world_map_draw_segment(GContext *ctx, int16_t y,
                                       int16_t start_x, int16_t end_x,
                                       GColor color) {
  if (start_x > end_x) {
    return;
  }

  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_line(ctx, GPoint(start_x, y), GPoint(end_x, y));
}

static void prv_world_map_draw_twilight_band(GContext *ctx, int16_t y,
                                             int16_t min_x, int16_t max_x,
                                             int16_t x, bool wrapped_segment) {
  if (x < min_x || x > max_x) {
    return;
  }

  if (PBL_IF_COLOR_ELSE(true, false)) {
    prv_world_map_draw_segment(ctx, y, x, x, COLOR_MAP_TWILIGHT);
    return;
  }

  if (prv_world_map_should_draw_twilight_pixel(x, y, wrapped_segment)) {
    prv_world_map_draw_segment(ctx, y, x, x, COLOR_MAP_TWILIGHT);
  }
}

static void prv_world_map_draw_twilight_edges(GContext *ctx, int16_t y,
                                              int16_t min_x, int16_t max_x,
                                              int16_t start_x, int16_t end_x,
                                              bool wrapped_segment) {
  const uint8_t twilight_width = prv_world_map_overlay_twilight_width();
  for (uint8_t offset = 0; offset < twilight_width; ++offset) {
    prv_world_map_draw_twilight_band(ctx, y, min_x, max_x, start_x + offset,
                                     wrapped_segment);
    prv_world_map_draw_twilight_band(ctx, y, min_x, max_x, end_x - offset,
                                     wrapped_segment);
  }
}

static void prv_world_map_draw_bitmap_segment(GContext *ctx,
                                              const GBitmap *bitmap,
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

static void prv_world_map_reset_overlay(WorldClockData *data, uint32_t version,
                                        uint16_t map_width, uint16_t map_height,
                                        uint16_t total_rows) {
  data->overlay_version = version;
  data->overlay_map_width = map_width;
  data->overlay_map_height = map_height;
  data->overlay_expected_rows = total_rows;
  data->overlay_received_rows = 0;
  data->overlay_valid = false;
  memset(data->overlay_row_received, 0, sizeof(data->overlay_row_received));
}

static void prv_save_overlay(WorldClockData *data) {
  if (!data || !data->overlay_valid) {
    return;
  }

  OverlayBackup backup = {
      .version = data->overlay_version,
      .map_width = data->overlay_map_width,
      .map_height = data->overlay_map_height,
      .total_rows = data->overlay_expected_rows,
  };
  memcpy(backup.daylight_start, data->overlay_daylight_start,
         sizeof(backup.daylight_start));
  memcpy(backup.daylight_end, data->overlay_daylight_end,
         sizeof(backup.daylight_end));

  persist_write_data(OVERLAY_PERSIST_KEY, &backup, sizeof(backup));
}

static void prv_load_overlay(WorldClockData *data) {
  if (!persist_exists(OVERLAY_PERSIST_KEY)) {
    return;
  }

  OverlayBackup backup;
  if (persist_read_data(OVERLAY_PERSIST_KEY, &backup, sizeof(backup)) !=
      sizeof(backup)) {
    return;
  }

  data->overlay_version = backup.version;
  data->overlay_map_width = backup.map_width;
  data->overlay_map_height = backup.map_height;
  data->overlay_expected_rows = backup.total_rows;
  data->overlay_received_rows = backup.total_rows;
  data->overlay_valid = true;

  memcpy(data->overlay_daylight_start, backup.daylight_start,
         sizeof(data->overlay_daylight_start));
  memcpy(data->overlay_daylight_end, backup.daylight_end,
         sizeof(data->overlay_daylight_end));
  memset(data->overlay_row_received, 1, sizeof(data->overlay_row_received));
}

////////////////////
// update procs for our three custom layers

static void bg_update_proc(Layer *layer, GContext *ctx) {
  WorldClockData *data = window_get_user_data(s_main_window);
  WorldClockMainWindowViewModel *model = &data->view_model;
  const GRect bounds = layer_get_bounds(layer);

  int16_t y = (model->bg_color.to_bottom_normalized * bounds.size.h) /
              ANIMATION_NORMALIZED_MAX;

  graphics_context_set_fill_color(
      ctx, PBL_IF_COLOR_ELSE(model->bg_color.bottom, GColorBlack));
  GRect rect_top = bounds;
  rect_top.size.h = y;
  graphics_fill_rect(ctx, rect_top, 0, GCornerNone);

  graphics_context_set_fill_color(
      ctx, PBL_IF_COLOR_ELSE(model->bg_color.top, GColorBlack));
  GRect rect_bottom = bounds;
  rect_bottom.origin.y += y;
  graphics_fill_rect(ctx, rect_bottom, 0, GCornerNone);
}

// Convert longitude/latitude to screen coordinates for the map
static GPoint lon_lat_to_screen(float longitude, float latitude,
                                const GRect map_bounds) {
  if (map_bounds.size.w <= 0 || map_bounds.size.h <= 0) {
    return GPointZero;
  }

  // World map is typically centered at 0° longitude
  // Longitude: -180 to 180 maps to 0 to map_width
  // Latitude: 90 to -90 maps to 0 to map_height
  int16_t x = map_bounds.origin.x +
              (int16_t)((longitude + 180.0) / 360.0 * map_bounds.size.w);
  int16_t y = map_bounds.origin.y +
              (int16_t)((90.0 - latitude) / 180.0 * map_bounds.size.h);

  // Clamp to bounds
  if (x < map_bounds.origin.x)
    x = map_bounds.origin.x;
  if (x >= map_bounds.origin.x + map_bounds.size.w)
    x = map_bounds.origin.x + map_bounds.size.w - 1;
  if (y < map_bounds.origin.y)
    y = map_bounds.origin.y;
  if (y >= map_bounds.origin.y + map_bounds.size.h)
    y = map_bounds.origin.y + map_bounds.size.h - 1;

  return GPoint(x, y);
}

static GRect calibrated_map_rect(const WorldClockData *data) {
  GRect rect = data->world_map_draw_rect;
  if (rect.size.h > WORLD_MAP_BOTTOM_TRIM) {
    rect.size.h -= WORLD_MAP_BOTTOM_TRIM;
  }
  return rect;
}

// Interpolate between two points based on animation progress
static GPoint interpolate_points(GPoint start, GPoint end, int32_t progress,
                                 int32_t max_progress) {
  if (max_progress == 0)
    return end;

  int32_t dx = end.x - start.x;
  int32_t dy = end.y - start.y;

  int16_t x = start.x + (int16_t)((dx * progress) / max_progress);
  int16_t y = start.y + (int16_t)((dy * progress) / max_progress);

  return GPoint(x, y);
}

// Animation update handler for dot movement
static void dot_animation_update(Animation *animation,
                                 const AnimationProgress progress) {
  WorldClockData *data = window_get_user_data(s_main_window);
  data->dot_animation_progress = progress;

  // Mark map layer dirty to trigger redraw
  layer_mark_dirty(data->map_layer);
}

// Animation stopped handler
static void dot_animation_stopped(Animation *animation, bool finished,
                                  void *context) {
  WorldClockData *data = window_get_user_data(s_main_window);
  data->dot_animation_active = false;
  data->dot_animation_progress = ANIMATION_NORMALIZED_MAX;

  // Update current position to target position
  data->current_dot_position = data->target_dot_position;

  // Mark map layer dirty for final update
  layer_mark_dirty(data->map_layer);
}

// Start dot animation to new city position
static void start_dot_animation(WorldClockData *data, int new_city_index) {
  // Get target position
  CityCoordinates *coords = world_clock_get_city_coordinates(new_city_index);
  if (!coords)
    return;

  GPoint target_pos = lon_lat_to_screen(coords->longitude, coords->latitude,
                                        calibrated_map_rect(data));

  // Set animation state
  data->target_dot_position = target_pos;
  data->dot_animation_active = true;
  data->dot_animation_progress = 0;

  // Create and start animation
  static const AnimationImplementation dot_anim_impl = {
      .update = dot_animation_update};

  Animation *dot_animation = animation_create();
  animation_set_duration(dot_animation, 300); // 300ms animation (2x faster)
  animation_set_curve(dot_animation, AnimationCurveEaseInOut);
  animation_set_implementation(dot_animation, &dot_anim_impl);
  animation_set_handlers(dot_animation,
                         (AnimationHandlers){.stopped = dot_animation_stopped},
                         NULL);

  animation_schedule(dot_animation);
}

static void prv_world_map_draw_overlay(WorldClockData *data, GContext *ctx,
                                       const GRect map_rect) {
  if (!data->overlay_valid || data->overlay_expected_rows == 0 ||
      data->overlay_map_width != map_rect.size.w ||
      data->overlay_map_height != map_rect.size.h) {
    return;
  }

  const int16_t width = map_rect.size.w;
  const int16_t row_count = data->overlay_expected_rows < map_rect.size.h
                                ? data->overlay_expected_rows
                                : map_rect.size.h;

  for (int16_t row = 0; row < row_count; ++row) {
    const int16_t y = map_rect.origin.y + row;
    const uint8_t day_start_value = data->overlay_daylight_start[row];
    const uint8_t day_end_value = data->overlay_daylight_end[row];

    int16_t day_start = prv_world_map_clamp_x(day_start_value, width);
    int16_t day_end = prv_world_map_clamp_x(day_end_value, width);

    if (day_start_value == WORLD_MAP_OVERLAY_FULL_NIGHT &&
        day_end_value == WORLD_MAP_OVERLAY_FULL_NIGHT) {
      if (PBL_IF_COLOR_ELSE(true, false)) {
        prv_world_map_draw_bitmap_segment(ctx, data->world_map_night_image,
                                          map_rect, row, 0, width - 1);
      } else {
        prv_world_map_draw_segment(ctx, y, map_rect.origin.x,
                                   map_rect.origin.x + width - 1,
                                   COLOR_MAP_NIGHT_OVERLAY);
      }
      continue;
    }

    if (day_start == 0 && day_end == width - 1) {
      continue;
    }

    if (PBL_IF_COLOR_ELSE(true, false)) {
      if (day_start <= day_end) {
        prv_world_map_draw_bitmap_segment(ctx, data->world_map_night_image,
                                          map_rect, row, 0, day_start - 1);
        prv_world_map_draw_bitmap_segment(ctx, data->world_map_night_image,
                                          map_rect, row, day_end + 1,
                                          width - 1);
      } else {
        prv_world_map_draw_bitmap_segment(ctx, data->world_map_night_image,
                                          map_rect, row, day_end + 1,
                                          day_start - 1);
      }
      continue;
    }

    if (day_start <= day_end) {
      prv_world_map_draw_segment(ctx, y, map_rect.origin.x,
                                 map_rect.origin.x + day_start - 1,
                                 COLOR_MAP_NIGHT_OVERLAY);
      prv_world_map_draw_segment(ctx, y, map_rect.origin.x + day_end + 1,
                                 map_rect.origin.x + width - 1,
                                 COLOR_MAP_NIGHT_OVERLAY);
      prv_world_map_draw_twilight_edges(
          ctx, y, map_rect.origin.x, map_rect.origin.x + width - 1,
          map_rect.origin.x + day_start, map_rect.origin.x + day_end, false);
    } else {
      prv_world_map_draw_segment(ctx, y, map_rect.origin.x + day_end + 1,
                                 map_rect.origin.x + day_start - 1,
                                 COLOR_MAP_NIGHT_OVERLAY);
      prv_world_map_draw_twilight_edges(
          ctx, y, map_rect.origin.x, map_rect.origin.x + width - 1,
          map_rect.origin.x + day_end + 1, map_rect.origin.x + day_start - 1,
          true);
    }
  }
}

static void map_update_proc(Layer *layer, GContext *ctx) {
  WorldClockData *data = window_get_user_data(s_main_window);
  const GRect bounds = layer_get_bounds(layer);
  const GRect map_rect = calibrated_map_rect(data);

  graphics_context_set_fill_color(ctx, COLOR_MAP_BACKGROUND);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the world map bitmap within its fitted rect.
  if (data->world_map_image) {
    graphics_draw_bitmap_in_rect(ctx, data->world_map_image,
                                 data->world_map_draw_rect);
  }

  // Mask the unused lower strip of the source asset without changing layout.
  if (data->world_map_draw_rect.size.h > WORLD_MAP_BOTTOM_TRIM) {
    graphics_context_set_fill_color(ctx, COLOR_MAP_BACKGROUND);
    graphics_fill_rect(
        ctx,
        GRect(data->world_map_draw_rect.origin.x,
              data->world_map_draw_rect.origin.y +
                  data->world_map_draw_rect.size.h - WORLD_MAP_BOTTOM_TRIM,
              data->world_map_draw_rect.size.w, WORLD_MAP_BOTTOM_TRIM),
        0, GCornerNone);
  }

  prv_world_map_draw_overlay(data, ctx, map_rect);

  // Calculate dot position (animated or static)
  GPoint dot_position;
  if (data->dot_animation_active) {
    // Use interpolated position during animation
    dot_position = interpolate_points(
        data->current_dot_position, data->target_dot_position,
        data->dot_animation_progress, ANIMATION_NORMALIZED_MAX);
  } else {
    // Use current city's position when not animating
    int current_city_index = world_clock_index_of_data_point(data->data_point);
    CityCoordinates *coords =
        world_clock_get_city_coordinates(current_city_index);
    if (coords) {
      dot_position =
          lon_lat_to_screen(coords->longitude, coords->latitude, map_rect);
      // Update current position for future animations
      data->current_dot_position = dot_position;
    } else {
      // Fallback position
      dot_position = GPoint(bounds.size.w / 2, bounds.size.h / 2);
    }
  }

  // Draw crosshair lines.
  graphics_context_set_stroke_color(ctx, COLOR_CROSSHAIR);

  // Horizontal line across the entire width
  graphics_draw_line(ctx, GPoint(0, dot_position.y),
                     GPoint(bounds.size.w, dot_position.y));

  // Vertical line across the entire height
  graphics_draw_line(ctx, GPoint(dot_position.x, 0),
                     GPoint(dot_position.x, bounds.size.h));

  // Draw single animated dot
  graphics_context_set_fill_color(ctx, COLOR_DOT_FILL);
  graphics_fill_circle(ctx, dot_position, 4);

  // Draw border for better visibility
  graphics_context_set_stroke_color(ctx, COLOR_DOT_OUTLINE);
  graphics_draw_circle(ctx, dot_position, 4);
}

static void horizontal_ruler_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  // y relative to layer's bounds to support clipping after some vertical
  // scrolling
  const int16_t yy = 11;

  graphics_context_set_stroke_color(ctx, COLOR_RULER);
  graphics_draw_line(ctx, GPoint(0, yy), GPoint(bounds.size.w, yy));
}

static bool prv_world_map_parse_uint8(const char **cursor, uint8_t *value_out) {
  if (!cursor || !*cursor || !value_out) {
    return false;
  }

  const char *current = *cursor;
  if (*current < '0' || *current > '9') {
    return false;
  }

  uint16_t value = 0;
  while (*current >= '0' && *current <= '9') {
    value = (uint16_t)(value * 10 + (uint16_t)(*current - '0'));
    if (value > 255) {
      return false;
    }
    current += 1;
  }

  *value_out = (uint8_t)value;
  *cursor = current;
  return true;
}

static uint16_t prv_world_map_parse_overlay_rows(WorldClockData *data,
                                                 uint16_t row_start,
                                                 uint16_t row_count,
                                                 const char *rows_text) {
  if (!rows_text) {
    return 0;
  }

  const char *cursor = rows_text;
  uint16_t parsed_rows = 0;

  while (parsed_rows < row_count && *cursor != '\0') {
    uint8_t daylight_start = 0;
    uint8_t daylight_end = 0;
    if (!prv_world_map_parse_uint8(&cursor, &daylight_start) ||
        *cursor != ',') {
      return parsed_rows;
    }
    cursor += 1;

    if (!prv_world_map_parse_uint8(&cursor, &daylight_end)) {
      return parsed_rows;
    }

    const uint16_t row_index = row_start + parsed_rows;
    if (row_index >= WORLD_MAP_MAX_OVERLAY_ROWS) {
      return parsed_rows;
    }

    data->overlay_daylight_start[row_index] = daylight_start;
    data->overlay_daylight_end[row_index] = daylight_end;
    if (!data->overlay_row_received[row_index]) {
      data->overlay_row_received[row_index] = true;
      data->overlay_received_rows += 1;
    }

    parsed_rows += 1;
    if (*cursor == ';') {
      cursor += 1;
    } else if (*cursor != '\0') {
      return parsed_rows;
    }
  }

  return parsed_rows;
}

static void prv_request_overlay_update(void) {
  if (!s_main_window) {
    return;
  }

  WorldClockData *data = window_get_user_data(s_main_window);
  if (!data || !data->map_layer) {
    return;
  }

  const GRect map_rect = calibrated_map_rect(data);
  if (map_rect.size.w <= 0 || map_rect.size.h <= 0) {
    return;
  }

  DictionaryIterator *iter = NULL;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK || !iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "overlay request begin failed: %d", result);
    return;
  }

  dict_write_uint8(iter, MESSAGE_KEY_request_overlay, 1);
  dict_write_uint16(iter, MESSAGE_KEY_overlay_map_width, map_rect.size.w);
  dict_write_uint16(iter, MESSAGE_KEY_overlay_map_height, map_rect.size.h);
  dict_write_end(iter);

  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "overlay request send failed: %d", result);
  }
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  WorldClockData *data = window_get_user_data(s_main_window);
  if (!data) {
    return;
  }

  Tuple *width_tuple = dict_find(iter, MESSAGE_KEY_overlay_map_width);
  Tuple *height_tuple = dict_find(iter, MESSAGE_KEY_overlay_map_height);
  Tuple *version_tuple = dict_find(iter, MESSAGE_KEY_overlay_version);
  Tuple *total_rows_tuple = dict_find(iter, MESSAGE_KEY_overlay_total_rows);
  Tuple *row_start_tuple = dict_find(iter, MESSAGE_KEY_overlay_row_start);
  Tuple *row_count_tuple = dict_find(iter, MESSAGE_KEY_overlay_row_count);
  Tuple *rows_tuple = dict_find(iter, MESSAGE_KEY_overlay_rows);
  char rows_buffer[WORLD_MAP_OVERLAY_ROWS_BUFFER_SIZE];

  if (!width_tuple || !height_tuple || !version_tuple || !total_rows_tuple ||
      !row_start_tuple || !row_count_tuple || !rows_tuple) {
    return;
  }

  if (rows_tuple->type != TUPLE_CSTRING) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "overlay rows tuple type invalid: %d",
            rows_tuple->type);
    return;
  }

  const uint16_t rows_copy_length = rows_tuple->length < sizeof(rows_buffer) - 1
                                        ? rows_tuple->length
                                        : sizeof(rows_buffer) - 1;
  strncpy(rows_buffer, rows_tuple->value->cstring, rows_copy_length);
  rows_buffer[rows_copy_length] = '\0';

  const uint16_t map_width = (uint16_t)width_tuple->value->int32;
  const uint16_t map_height = (uint16_t)height_tuple->value->int32;
  const uint32_t version = (uint32_t)version_tuple->value->int32;
  const uint16_t total_rows = (uint16_t)total_rows_tuple->value->int32;
  const uint16_t row_start = (uint16_t)row_start_tuple->value->int32;
  const uint16_t row_count = (uint16_t)row_count_tuple->value->int32;

  if (total_rows == 0 || total_rows > WORLD_MAP_MAX_OVERLAY_ROWS ||
      row_count == 0 || row_start + row_count > total_rows) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "overlay payload bounds invalid");
    return;
  }

  if (data->overlay_version != version ||
      data->overlay_map_width != map_width ||
      data->overlay_map_height != map_height ||
      data->overlay_expected_rows != total_rows) {
    prv_world_map_reset_overlay(data, version, map_width, map_height,
                                total_rows);
  }

  const uint16_t parsed_rows =
      prv_world_map_parse_overlay_rows(data, row_start, row_count, rows_buffer);
  if (parsed_rows != row_count) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "overlay chunk parse mismatch: %u/%u",
            (unsigned int)parsed_rows, (unsigned int)row_count);
    return;
  }

  if (data->overlay_received_rows >= data->overlay_expected_rows) {
    data->overlay_valid = true;
    prv_save_overlay(data);
    if (data->map_layer) {
      layer_mark_dirty(data->map_layer);
    }
  }
}

static void prv_inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "inbox dropped: %d", reason);
}

static void prv_outbox_failed(DictionaryIterator *iter, AppMessageResult reason,
                              void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "outbox failed: %d", reason);
}

static void prv_outbox_sent(DictionaryIterator *iter, void *context) {}

////////////////////
// App boilerplate

//! helper to construct the various text layers as they appear in this app
static GRect init_text_layer(Layer *parent_layer, TextLayer **text_layer,
                             int16_t y, int16_t h,
                             int16_t additional_right_margin, char *font_key) {
  // why "-1" (and then "+2")? because for this font we need to compensate for
  // weird white-spacing
  const int16_t font_compensator =
      strcmp(font_key, FONT_KEY_LECO_38_BOLD_NUMBERS) == 0 ? 3 : 1;

  // Use smaller margins for round watches
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

  // Center align text horizontally on round watches
  if (PBL_IF_ROUND_ELSE(true, false)) {
    text_layer_set_text_alignment(*text_layer, GTextAlignmentCenter);
  }

  layer_add_child(parent_layer, text_layer_get_layer(*text_layer));
  return frame;
}

void init_statusbar_text_layer(Layer *parent, TextLayer **layer) {
  // For status bar, always use full width regardless of screen shape
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

  // Status bar is always center-aligned
  text_layer_set_text_alignment(*layer, GTextAlignmentCenter);
}

//! sets the new data model
static void set_data_point(WorldClockData *data, WorldClockDataPoint *dp) {
  data->data_point = dp;
  world_clock_view_model_fill_all(&data->view_model, dp);
}

static void load_dst_settings() {
  bool dst_settings[5] = {true, true, true, false, true}; // defaults
  if (persist_exists(DST_PERSIST_KEY)) {
    persist_read_data(DST_PERSIST_KEY, dst_settings, sizeof(dst_settings));
  }
  for (int i = 0; i < 5; i++) {
    s_data_points[i].is_dst = dst_settings[i];
  }
}

static void save_dst_settings() {
  bool dst_settings[5];
  for (int i = 0; i < 5; i++) {
    dst_settings[i] = s_data_points[i].is_dst;
  }
  persist_write_data(DST_PERSIST_KEY, dst_settings, sizeof(dst_settings));
}

static void update_status_bar_time() {
  WorldClockData *data = window_get_user_data(s_main_window);

  // Get current local time
  time_t now = time(NULL);
  struct tm *local_time = localtime(&now);

  // Format time for status bar based on user preference
  static char time_str[] = "00:00";
  if (clock_is_24h_style()) {
    // 24-hour format
    strftime(time_str, sizeof(time_str), "%H:%M", local_time);
  } else {
    // 12-hour format with AM/PM
    strftime(time_str, sizeof(time_str), "%I:%M %p", local_time);
    // Remove leading zero from hour
    if (time_str[0] == '0') {
      memmove(time_str, time_str + 1, strlen(time_str));
    }
  }

  text_layer_set_text(data->fake_statusbar, time_str);
}

static void prv_handle_minute_tick(struct tm *tick_time,
                                   TimeUnits units_changed) {
  WorldClockData *data = window_get_user_data(s_main_window);

  // Update status bar time
  update_status_bar_time();

  // Update world clock times
  WorldClockDataViewNumbers numbers =
      world_clock_data_point_view_model_numbers(data->data_point);
  world_clock_view_model_fill_numbers(&data->view_model, numbers,
                                      data->data_point);
  prv_request_overlay_update();
}

static void update_ampm_position(WorldClockData *data) {
  if (clock_is_24h_style()) {
    layer_set_hidden(text_layer_get_layer(data->ampm_layer), true);
  } else {
    layer_set_hidden(text_layer_get_layer(data->ampm_layer), false);

    // Get the current bounds of the time layer
    GRect time_bounds = layer_get_frame(text_layer_get_layer(data->time_layer));

    // Get the content size of the time text to determine actual width
    GSize time_content_size = text_layer_get_content_size(data->time_layer);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "content size width: %d", time_content_size.w);

    // Position AM/PM layer right after the time text
    int16_t ampm_x = time_bounds.origin.x + time_content_size.w + 0;
    int16_t ampm_y = time_bounds.origin.y + 12; // align with baseline

    // Update AM/PM layer position
    GRect ampm_frame = layer_get_frame(text_layer_get_layer(data->ampm_layer));
    layer_set_frame(
        text_layer_get_layer(data->ampm_layer),
        GRect(ampm_frame.size.w, ampm_frame.size.h, ampm_x, ampm_y));
  }
}

static void view_model_changed(struct WorldClockMainWindowViewModel *arg) {
  WorldClockMainWindowViewModel *model = (WorldClockMainWindowViewModel *)arg;

  WorldClockData *data = window_get_user_data(s_main_window);

  text_layer_set_text(data->city_layer, model->city);
  text_layer_set_text(data->time_layer, model->time.text);
  text_layer_set_text(data->ampm_layer, model->time.ampm);
  text_layer_set_text(data->relative_info_layer, model->relative_info.text);
  text_layer_set_text(data->pagination_layer, model->pagination.text);

  // Update AM/PM position based on time text width
  update_ampm_position(data);

  // Force redraw by temporarily clearing and resetting the text
  text_layer_set_text(data->relative_info_layer, "");
  text_layer_set_text(data->relative_info_layer, model->relative_info.text);

  // make sure to redraw (if no string pointer changed none of the layers would
  // be dirty)
  layer_mark_dirty(window_get_root_layer(s_main_window));
  // Also mark the relative_info layer dirty to ensure update
  layer_mark_dirty(text_layer_get_layer(data->relative_info_layer));
}

static void main_window_load(Window *window) {
  WorldClockData *data = window_get_user_data(window);
  data->view_model.announce_changed = view_model_changed;

  Layer *window_layer = window_get_root_layer(window);
  const GRect bounds = layer_get_bounds(window_layer);
  layer_set_update_proc(window_layer, bg_update_proc);

  // Calculate layout based on screen shape
  const int16_t screen_width = bounds.size.w;

  // For round watches, center content vertically and use smaller margins
  const int16_t base_margin =
      PBL_IF_ROUND_ELSE(4, MARGIN); // Smaller margins for round
  const int16_t map_margin = PBL_IF_ROUND_ELSE(10, -10);
  const int16_t map_width = screen_width - 2 * map_margin;

  data->world_map_base_image =
      gbitmap_create_with_resource(RESOURCE_ID_WORLD_MAP);
  data->world_map_night_image =
      gbitmap_create_with_resource(RESOURCE_ID_WORLD_MAP);
  data->world_map_image = gbitmap_create_with_resource(RESOURCE_ID_WORLD_MAP);
  prv_world_map_recolor(data->world_map_base_image);
  prv_world_map_recolor_night(data->world_map_night_image);
  prv_world_map_recolor(data->world_map_image);

  int16_t map_height = 72;
  data->world_map_draw_rect = GRectZero;
  if (data->world_map_base_image) {
    const GRect image_bounds = gbitmap_get_bounds(data->world_map_base_image);
    map_height = image_bounds.size.h;
    if (map_height > WORLD_MAP_BOTTOM_TRIM) {
      map_height -= WORLD_MAP_BOTTOM_TRIM;
    }
    data->world_map_draw_rect = GRect((map_width - image_bounds.size.w) / 2, 0,
                                      image_bounds.size.w, image_bounds.size.h);
  }

  // Create map layer - full width for rectangular watches, margins for round
  data->map_layer =
      layer_create(GRect(map_margin, WORLD_MAP_TOP, map_width, map_height));
  layer_set_update_proc(data->map_layer, map_update_proc);
  layer_add_child(window_layer, data->map_layer);

  if (!data->world_map_image) {
    data->world_map_draw_rect = layer_get_bounds(data->map_layer);
  }

  // Keep the content stack aligned beneath the map after per-platform top
  // adjustments.
  const int16_t map_bottom = WORLD_MAP_TOP + map_height;
  const int16_t city_y = PBL_IF_ROUND_ELSE(map_bottom + 4, map_bottom + 5);
  const int16_t ruler_y = PBL_IF_ROUND_ELSE(map_bottom + 13, map_bottom + 22);
  const int16_t time_y = PBL_IF_ROUND_ELSE(map_bottom + 22, map_bottom + 31);
  const int16_t relative_info_y =
      PBL_IF_ROUND_ELSE(map_bottom + 64, map_bottom + 73);

  data->horizontal_ruler_layer = layer_create(
      GRect(base_margin, ruler_y, screen_width - 2 * base_margin, 20));
  layer_set_update_proc(data->horizontal_ruler_layer,
                        horizontal_ruler_update_proc);
  layer_add_child(window_layer, data->horizontal_ruler_layer);

  init_text_layer(window_layer, &data->city_layer, city_y, 30, 0,
                  FONT_KEY_GOTHIC_18_BOLD);
  const int16_t time_top = time_y;
  init_text_layer(window_layer, &data->time_layer, time_top, 40, 0,
                  FONT_KEY_LECO_38_BOLD_NUMBERS);

  // Create AM/PM layer with smaller font
  const int16_t ampm_y = time_top + 8; // Position below the time numbers
  GRect ampm_frame = GRect(base_margin, ampm_y, 40,
                           20); // Initial position, will be updated dynamically
  data->ampm_layer = text_layer_create(ampm_frame);
  text_layer_set_background_color(data->ampm_layer, GColorClear);
  text_layer_set_text_color(data->ampm_layer, COLOR_TEXT_DEFAULT);
  text_layer_set_font(
      data->ampm_layer,
      fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM));
  text_layer_set_text_alignment(data->ampm_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(data->ampm_layer));

  init_text_layer(window_layer, &data->relative_info_layer, relative_info_y, 19,
                  0, FONT_KEY_GOTHIC_14);

  init_statusbar_text_layer(window_layer, &data->fake_statusbar);

  init_statusbar_text_layer(window_layer, &data->pagination_layer);
  text_layer_set_text_alignment(data->pagination_layer, GTextAlignmentRight);

  // Initialize dot animation state
  data->dot_animation_active = false;
  data->dot_animation_progress = ANIMATION_NORMALIZED_MAX;

  // Set initial dot position to current city
  int current_city_index = world_clock_index_of_data_point(data->data_point);
  CityCoordinates *coords =
      world_clock_get_city_coordinates(current_city_index);
  if (coords) {
    data->current_dot_position = lon_lat_to_screen(
        coords->longitude, coords->latitude, calibrated_map_rect(data));
    data->target_dot_position = data->current_dot_position;
  }

  // propagate all view model content to the UI
  world_clock_main_window_view_model_announce_changed(&data->view_model);

  // Position AM/PM layer correctly on startup
  update_ampm_position(data);

  // Load cached overlay if available
  prv_load_overlay(data);

  prv_request_overlay_update();
}

static void main_window_unload(Window *window) {
  WorldClockData *data = window_get_user_data(window);
  data->view_model.announce_changed = NULL;
  world_clock_view_model_deinit(&data->view_model);

  layer_destroy(data->horizontal_ruler_layer);
  layer_destroy(data->map_layer);
  gbitmap_destroy(data->world_map_base_image);
  gbitmap_destroy(data->world_map_night_image);
  gbitmap_destroy(data->world_map_image);
  text_layer_destroy(data->city_layer);
  text_layer_destroy(data->time_layer);
  text_layer_destroy(data->ampm_layer);
  text_layer_destroy(data->relative_info_layer);
  text_layer_destroy(data->fake_statusbar);
  text_layer_destroy(data->pagination_layer);
}

static void after_scroll_swap_text(Animation *animation, bool finished,
                                   void *context) {
  WorldClockData *data = window_get_user_data(s_main_window);
  WorldClockDataPoint *data_point = context;

  world_clock_view_model_fill_strings_and_pagination(&data->view_model,
                                                     data_point);
  world_clock_view_model_fill_numbers(
      &data->view_model, world_clock_data_point_view_model_numbers(data_point),
      data_point);
}

static Animation *create_anim_scroll_out(Layer *layer, uint32_t duration,
                                         int16_t dy) {
  GPoint to_origin = GPoint(0, dy);
  Animation *result = (Animation *)property_animation_create_bounds_origin(
      layer, NULL, &to_origin);
  animation_set_duration(result, duration);
  animation_set_curve(result, AnimationCurveLinear);
  return result;
}

static Animation *create_anim_scroll_in(Layer *layer, uint32_t duration,
                                        int16_t dy) {
  GPoint from_origin = GPoint(0, dy);
  Animation *result = (Animation *)property_animation_create_bounds_origin(
      layer, &from_origin, &GPointZero);
  animation_set_duration(result, duration);
  animation_set_curve(result, AnimationCurveEaseOut);
  return result;
}

static const uint32_t BACKGROUND_SCROLL_DURATION = 100 * 2;
static const uint32_t SCROLL_DURATION = 130 * 2;
static const int16_t SCROLL_DIST_OUT = 20;
static const int16_t SCROLL_DIST_IN = 8;

typedef enum {
  ScrollDirectionDown,
  ScrollDirectionUp,
} ScrollDirection;

static Animation *create_outbound_anim(WorldClockData *data,
                                       ScrollDirection direction) {
  const int16_t to_dy =
      (direction == ScrollDirectionDown) ? -SCROLL_DIST_OUT : SCROLL_DIST_OUT;

  Animation *out_city = create_anim_scroll_out(
      text_layer_get_layer(data->city_layer), SCROLL_DURATION, to_dy);
  Animation *out_ruler = create_anim_scroll_out(data->horizontal_ruler_layer,
                                                SCROLL_DURATION, to_dy);
  Animation *out_ampm = create_anim_scroll_out(
      text_layer_get_layer(data->ampm_layer), SCROLL_DURATION, to_dy);

  return animation_spawn_create(out_city, out_ruler, out_ampm, NULL);
}

static Animation *create_inbound_anim(WorldClockData *data,
                                      ScrollDirection direction) {
  const int16_t from_dy =
      (direction == ScrollDirectionDown) ? -SCROLL_DIST_IN : SCROLL_DIST_IN;

  Animation *in_city = create_anim_scroll_in(
      text_layer_get_layer(data->city_layer), SCROLL_DURATION, from_dy);
  Animation *in_time = create_anim_scroll_in(
      text_layer_get_layer(data->time_layer), SCROLL_DURATION, from_dy);
  Animation *in_relative_info =
      create_anim_scroll_in(text_layer_get_layer(data->relative_info_layer),
                            SCROLL_DURATION, from_dy);
  Animation *in_ruler = create_anim_scroll_in(data->horizontal_ruler_layer,
                                              SCROLL_DURATION, from_dy);
  Animation *in_ampm = create_anim_scroll_in(
      text_layer_get_layer(data->ampm_layer), SCROLL_DURATION, from_dy);

  return animation_spawn_create(in_city, in_time, in_relative_info, in_ruler,
                                in_ampm, NULL);
}

static Animation *animation_for_scroll(WorldClockData *data,
                                       ScrollDirection direction,
                                       WorldClockDataPoint *next_data_point) {
  WorldClockMainWindowViewModel *view_model = &data->view_model;

  // sliding texts
  Animation *out_text = create_outbound_anim(data, direction);
  animation_set_handlers(out_text,
                         (AnimationHandlers){
                             .stopped = after_scroll_swap_text,
                         },
                         next_data_point);
  Animation *in_text = create_inbound_anim(data, direction);

  // changing numbers (time and offset)
  Animation *number_animation = world_clock_create_view_model_animation_numbers(
      view_model, next_data_point);
  animation_set_duration(number_animation, SCROLL_DURATION * 2);

  // scrolling background color
  Animation *bg_animation = world_clock_create_view_model_animation_bgcolor(
      view_model, next_data_point);
  animation_set_duration(bg_animation, BACKGROUND_SCROLL_DURATION);
  animation_set_reverse(bg_animation, (direction == ScrollDirectionDown));

  return animation_spawn_create(
      animation_sequence_create(out_text, in_text, NULL), number_animation,
      bg_animation, NULL);
}

static Animation *animation_for_bounce(WorldClockData *data,
                                       ScrollDirection direction) {
  return create_inbound_anim(data, direction);
}

static void ask_for_scroll(WorldClockData *data, ScrollDirection direction) {
  int delta = direction == ScrollDirectionUp ? -1 : +1;
  WorldClockDataPoint *next_data_point =
      world_clock_data_point_delta(data->data_point, delta);

  Animation *scroll_animation;

  if (!next_data_point) {
    scroll_animation = animation_for_bounce(data, direction);
  } else {
    // Get next city index for animation
    int next_city_index = world_clock_index_of_data_point(next_data_point);

    // data point switches immediately
    data->data_point = next_data_point;

    // Start dot animation to new city
    start_dot_animation(data, next_city_index);

    scroll_animation = animation_for_scroll(data, direction, next_data_point);
  }

  animation_unschedule(data->previous_animation);
  animation_schedule(scroll_animation);
  data->previous_animation = scroll_animation;
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  WorldClockData *data = context;
  ask_for_scroll(data, ScrollDirectionUp);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  WorldClockData *data = context;
  ask_for_scroll(data, ScrollDirectionDown);
}

static void select_long_click_handler(ClickRecognizerRef recognizer,
                                      void *context) {
  WorldClockData *data = context;
  // Toggle DST for current city
  data->data_point->is_dst = !data->data_point->is_dst;
  save_dst_settings();
  // Refresh display
  WorldClockDataViewNumbers numbers =
      world_clock_data_point_view_model_numbers(data->data_point);
  world_clock_view_model_fill_numbers(&data->view_model, numbers,
                                      data->data_point);
  text_layer_set_text(data->relative_info_layer,
                      data->view_model.relative_info.text);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, select_long_click_handler,
                              NULL);
}

static void init() {
  load_dst_settings();

  WorldClockData *data = malloc(sizeof(WorldClockData));
  memset(data, 0, sizeof(WorldClockData));

  WorldClockDataPoint *dp = world_clock_data_point_at(0);
  set_data_point(data, dp);

  s_main_window = window_create();
  window_set_click_config_provider_with_context(s_main_window,
                                                click_config_provider, data);
  window_set_user_data(s_main_window, data);
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload,
                                            });
  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_failed(prv_outbox_failed);
  app_message_register_outbox_sent(prv_outbox_sent);
  app_message_open(WORLD_MAP_APP_MESSAGE_INBOX_SIZE,
                   WORLD_MAP_APP_MESSAGE_OUTBOX_SIZE);

  window_stack_push(s_main_window, true);
  update_status_bar_time();

  tick_timer_service_subscribe(MINUTE_UNIT, prv_handle_minute_tick);
}

static void deinit() {
  WorldClockData *data = window_get_user_data(s_main_window);
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
  free(data);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
