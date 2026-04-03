#include "world_clock_animations.h"
#include "world_clock_data.h"
#include "world_clock_messaging.h"
#include "world_clock_overlay.h"
#include "world_clock_private.h"
#include "world_clock_ui.h"
#include <pebble.h>
#include <stdlib.h>
#include <string.h>

#define MARGIN 8
#define WORLD_MAP_BOTTOM_TRIM 10

#if defined(PBL_PLATFORM_GABBRO)
#define WORLD_MAP_TOP 45
#else
#define WORLD_MAP_TOP STATUS_BAR_LAYER_HEIGHT
#endif

static Window *s_main_window;

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

static void dot_animation_update(Animation *animation,
                                 const AnimationProgress progress) {
  WorldClockData *data = window_get_user_data(s_main_window);
  data->dot_animation_progress = progress;
  layer_mark_dirty(data->map_layer);
}

static void dot_animation_stopped(Animation *animation, bool finished,
                                  void *context) {
  WorldClockData *data = window_get_user_data(s_main_window);
  data->dot_animation_active = false;
  data->dot_animation_progress = ANIMATION_NORMALIZED_MAX;
  data->current_dot_position = data->target_dot_position;
  layer_mark_dirty(data->map_layer);
}

static void start_dot_animation(WorldClockData *data, int new_city_index) {
  CityCoordinates *coords = world_clock_get_city_coordinates(new_city_index);
  if (!coords)
    return;

  GPoint target_pos =
      world_clock_lon_lat_to_screen(coords->longitude, coords->latitude,
                                    world_clock_calibrated_map_rect(data));

  data->target_dot_position = target_pos;
  data->dot_animation_active = true;
  data->dot_animation_progress = 0;

  static const AnimationImplementation dot_anim_impl = {
      .update = dot_animation_update};

  Animation *dot_animation = animation_create();
  animation_set_duration(dot_animation, 300);
  animation_set_curve(dot_animation, AnimationCurveEaseInOut);
  animation_set_implementation(dot_animation, &dot_anim_impl);
  animation_set_handlers(dot_animation,
                         (AnimationHandlers){.stopped = dot_animation_stopped},
                         NULL);

  animation_schedule(dot_animation);
}

GPoint world_clock_lon_lat_to_screen(float longitude, float latitude,
                                     const GRect map_bounds) {
  if (map_bounds.size.w <= 0 || map_bounds.size.h <= 0) {
    return GPointZero;
  }

  int16_t x = map_bounds.origin.x +
              (int16_t)((longitude + 180.0) / 360.0 * map_bounds.size.w);
  int16_t y = map_bounds.origin.y +
              (int16_t)((90.0 - latitude) / 180.0 * map_bounds.size.h);

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

bool world_clock_is_city_index_night(WorldClockData *data, int city_index) {
  CityCoordinates *coords = world_clock_get_city_coordinates(city_index);
  if (!coords) {
    return false;
  }

  const GRect map_rect = world_clock_calibrated_map_rect(data);
  GPoint city_pos = world_clock_lon_lat_to_screen(coords->longitude,
                                                  coords->latitude, map_rect);

  int16_t row = city_pos.y - map_rect.origin.y;
  int16_t col = city_pos.x - map_rect.origin.x;

  return world_clock_overlay_is_city_night(&data->overlay, map_rect.size.w, row,
                                           col);
}

static bool prv_is_current_city_night(WorldClockData *data) {
  int current_city_index = world_clock_index_of_data_point(data->data_point);
  return world_clock_is_city_index_night(data, current_city_index);
}

GRect world_clock_calibrated_map_rect(const WorldClockData *data) {
  GRect rect = data->world_map_draw_rect;
  if (rect.size.h > WORLD_MAP_BOTTOM_TRIM) {
    rect.size.h -= WORLD_MAP_BOTTOM_TRIM;
  }
  return rect;
}

static void prv_world_map_draw_overlay(WorldClockData *data, GContext *ctx,
                                       const GRect map_rect) {
  if (!data->overlay.valid || data->overlay.expected_rows == 0) {
    return;
  }

  const int16_t width = map_rect.size.w;
  const int16_t row_count = data->overlay.expected_rows < map_rect.size.h
                                ? data->overlay.expected_rows
                                : map_rect.size.h;

  for (int16_t row = 0; row < row_count; ++row) {
    uint8_t day_start_value, day_end_value;
    if (!world_clock_overlay_query_row(&data->overlay, row, &day_start_value,
                                       &day_end_value)) {
      continue;
    }

    const int16_t y = map_rect.origin.y + row;
    int16_t day_start = world_clock_ui_clamp_x(day_start_value, width);
    int16_t day_end = world_clock_ui_clamp_x(day_end_value, width);

    if (day_start_value == WORLD_CLOCK_OVERLAY_FULL_NIGHT &&
        day_end_value == WORLD_CLOCK_OVERLAY_FULL_NIGHT) {
      if (PBL_IF_COLOR_ELSE(true, false)) {
        world_clock_ui_draw_bitmap_segment(ctx, data->world_map_night_image,
                                          map_rect, row, 0, width - 1);
      } else {
        world_clock_ui_draw_segment(ctx, y, map_rect.origin.x,
                                    map_rect.origin.x + width - 1,
                                    COLOR_MAP_NIGHT_OVERLAY);
      }
      continue;
    }

    if (day_start == 0 && day_end == width - 1) {
      continue;
    }

    if (day_start <= day_end) {
      world_clock_ui_draw_bitmap_segment(ctx, data->world_map_night_image,
                                         map_rect, row, 0, day_start - 1);
      world_clock_ui_draw_bitmap_segment(ctx, data->world_map_night_image,
                                         map_rect, row, day_end + 1, width - 1);
    } else {
      world_clock_ui_draw_bitmap_segment(ctx, data->world_map_night_image,
                                         map_rect, row, day_end + 1,
                                         day_start - 1);
    }
  }
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  WorldClockData *data = window_get_user_data(s_main_window);
  WorldClockMainWindowViewModel *model = &data->view_model;
  const GRect bounds = layer_get_bounds(layer);

  int16_t y = (model->bg_color.to_bottom_normalized * bounds.size.h) /
              ANIMATION_NORMALIZED_MAX;

  graphics_context_set_fill_color(ctx, model->bg_color.bottom);
  GRect rect_top = bounds;
  rect_top.size.h = y;
  graphics_fill_rect(ctx, rect_top, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, model->bg_color.top);
  GRect rect_bottom = bounds;
  rect_bottom.origin.y += y;
  graphics_fill_rect(ctx, rect_bottom, 0, GCornerNone);
}

static void map_update_proc(Layer *layer, GContext *ctx) {
  WorldClockData *data = window_get_user_data(s_main_window);
  const GRect bounds = layer_get_bounds(layer);
  const GRect map_rect = world_clock_calibrated_map_rect(data);

  graphics_context_set_fill_color(ctx, COLOR_MAP_BACKGROUND);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if (data->world_map_image) {
    graphics_draw_bitmap_in_rect(ctx, data->world_map_image,
                                 data->world_map_draw_rect);
  }

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

  GPoint dot_position;
  if (data->dot_animation_active) {
    dot_position = interpolate_points(
        data->current_dot_position, data->target_dot_position,
        data->dot_animation_progress, ANIMATION_NORMALIZED_MAX);
  } else {
    int current_city_index = world_clock_index_of_data_point(data->data_point);
    CityCoordinates *coords =
        world_clock_get_city_coordinates(current_city_index);
    if (coords) {
      dot_position = world_clock_lon_lat_to_screen(coords->longitude,
                                                   coords->latitude, map_rect);
      data->current_dot_position = dot_position;
    } else {
      dot_position = GPoint(bounds.size.w / 2, bounds.size.h / 2);
    }
  }

  graphics_context_set_stroke_color(ctx, data->view_model.crosshair_color);
  graphics_draw_line(ctx, GPoint(0, dot_position.y),
                     GPoint(bounds.size.w, dot_position.y));
  graphics_draw_line(ctx, GPoint(dot_position.x, 0),
                     GPoint(dot_position.x, bounds.size.h));

  graphics_context_set_fill_color(ctx, data->view_model.dot_fill_color);
  graphics_fill_circle(ctx, dot_position, 4);

  graphics_context_set_stroke_color(ctx, data->view_model.dot_outline_color);
  graphics_draw_circle(ctx, dot_position, 4);
}

static void horizontal_ruler_update_proc(Layer *layer, GContext *ctx) {
  WorldClockData *data = window_get_user_data(s_main_window);
  const GRect bounds = layer_get_bounds(layer);
  const int16_t yy = 11;

  graphics_context_set_stroke_color(ctx, data->view_model.ruler_color);
  graphics_draw_line(ctx, GPoint(0, yy), GPoint(bounds.size.w, yy));
}

static void prv_overlay_received(uint16_t map_width, uint16_t map_height,
                                  uint32_t version, uint16_t total_rows,
                                  uint16_t row_start, uint16_t row_count,
                                  const uint8_t *row_data, uint16_t row_data_len,
                                  void *context) {
  WorldClockData *data = window_get_user_data(s_main_window);
  if (!data) {
    return;
  }

  if (data->overlay.version != version ||
      data->overlay.map_width != map_width ||
      data->overlay.map_height != map_height ||
      data->overlay.expected_rows != total_rows) {
    world_clock_overlay_reset(&data->overlay, version, map_width, map_height,
                              total_rows);
  }

  bool complete = world_clock_overlay_feed_chunk(
      &data->overlay, row_start, row_count, row_data, row_data_len);

  if (complete) {
    data->overlay.valid = true;
    world_clock_overlay_cancel_timeout(&data->overlay);
    world_clock_overlay_save_cache(&data->overlay);
    if (data->map_layer) {
      layer_mark_dirty(data->map_layer);
    }
  }
}

static void set_data_point(WorldClockData *data, WorldClockDataPoint *dp) {
  data->data_point = dp;
  world_clock_view_model_fill_all(&data->view_model, dp);
}

static void prv_city_data_received(const uint8_t *blob, uint16_t length,
                                    int8_t user_city_index, void *context) {
  WorldClockData *data = window_get_user_data(s_main_window);
  if (!data) return;

  world_clock_data_apply_js_blob(blob, length);

  // Apply to current city and refresh display
  world_clock_view_model_fill_all(&data->view_model, data->data_point);

  // If we have a user city index and haven't initialized yet, start there
  if (user_city_index >= 0 && user_city_index < world_clock_num_data_points()) {
    WorldClockDataPoint *user_city = world_clock_data_point_at(user_city_index);
    if (user_city && data->data_point != user_city) {
      set_data_point(data, user_city);
    }
  }
}

static void update_status_bar_time() {
  WorldClockData *data = window_get_user_data(s_main_window);

  time_t now = time(NULL);
  struct tm *local_time = localtime(&now);

  static char time_str[] = "00:00";
  if (clock_is_24h_style()) {
    strftime(time_str, sizeof(time_str), "%H:%M", local_time);
  } else {
    strftime(time_str, sizeof(time_str), "%I:%M %p", local_time);
    if (time_str[0] == '0') {
      memmove(time_str, time_str + 1, strlen(time_str));
    }
  }

  text_layer_set_text(data->fake_statusbar, time_str);
}

static void prv_handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  WorldClockData *data = window_get_user_data(s_main_window);

  update_status_bar_time();

  WorldClockDataViewNumbers numbers =
      world_clock_data_point_view_model_numbers(data->data_point);
  world_clock_view_model_fill_numbers(&data->view_model, numbers,
                                      data->data_point);
  const GRect map_rect = world_clock_calibrated_map_rect(data);
  world_clock_messaging_request_overlay(map_rect.size.w, map_rect.size.h);
}

static void update_ampm_position(WorldClockData *data) {
  if (clock_is_24h_style()) {
    layer_set_hidden(text_layer_get_layer(data->ampm_layer), true);
  } else {
    layer_set_hidden(text_layer_get_layer(data->ampm_layer), false);

    GRect time_bounds = layer_get_frame(text_layer_get_layer(data->time_layer));
    GSize time_content_size = text_layer_get_content_size(data->time_layer);

    int16_t ampm_x = time_bounds.origin.x + time_content_size.w + 0;
    int16_t ampm_y = time_bounds.origin.y + 12;

    GRect ampm_frame = layer_get_frame(text_layer_get_layer(data->ampm_layer));
    layer_set_frame(
        text_layer_get_layer(data->ampm_layer),
        GRect(ampm_frame.size.w, ampm_frame.size.h, ampm_x, ampm_y));
  }
}

static void prv_update_night_mode(WorldClockData *data) {
  bool is_night = prv_is_current_city_night(data);
  if (is_night != data->view_model.is_night) {
    world_clock_view_model_fill_night_mode(&data->view_model, is_night);
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

  text_layer_set_text_color(data->city_layer, model->text_color);
  text_layer_set_text_color(data->time_layer, model->text_color);
  text_layer_set_text_color(data->ampm_layer, model->text_color);
  text_layer_set_text_color(data->relative_info_layer, model->text_color);
  text_layer_set_text_color(data->fake_statusbar, model->statusbar_text_color);
  text_layer_set_text_color(data->pagination_layer,
                            model->statusbar_text_color);

  update_ampm_position(data);

  text_layer_set_text(data->relative_info_layer, "");
  text_layer_set_text(data->relative_info_layer, model->relative_info.text);

  layer_mark_dirty(window_get_root_layer(s_main_window));
  layer_mark_dirty(text_layer_get_layer(data->relative_info_layer));
}

static void main_window_load(Window *window) {
  WorldClockData *data = window_get_user_data(window);
  data->view_model.announce_changed = view_model_changed;

  Layer *window_layer = window_get_root_layer(window);
  const GRect bounds = layer_get_bounds(window_layer);
  layer_set_update_proc(window_layer, bg_update_proc);

  const int16_t screen_width = bounds.size.w;
  const int16_t base_margin = PBL_IF_ROUND_ELSE(4, MARGIN);
  const int16_t map_margin = PBL_IF_ROUND_ELSE(10, -10);
  const int16_t map_width = screen_width - 2 * map_margin;

  data->world_map_base_image =
      gbitmap_create_with_resource(RESOURCE_ID_WORLD_MAP);
  data->world_map_night_image =
      gbitmap_create_with_resource(RESOURCE_ID_WORLD_MAP);
  data->world_map_image = gbitmap_create_with_resource(RESOURCE_ID_WORLD_MAP);
  world_clock_ui_recolor(data->world_map_base_image);
  world_clock_ui_recolor_night(data->world_map_night_image);
  world_clock_ui_recolor(data->world_map_image);

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

  data->map_layer =
      layer_create(GRect(map_margin, WORLD_MAP_TOP, map_width, map_height));
  layer_set_update_proc(data->map_layer, map_update_proc);
  layer_add_child(window_layer, data->map_layer);

  if (!data->world_map_image) {
    data->world_map_draw_rect = layer_get_bounds(data->map_layer);
  }

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

  const int16_t ampm_y = time_top + 8;
  GRect ampm_frame = GRect(base_margin, ampm_y, 40, 20);
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

  data->dot_animation_active = false;
  data->dot_animation_progress = ANIMATION_NORMALIZED_MAX;

  int current_city_index = world_clock_index_of_data_point(data->data_point);
  CityCoordinates *coords =
      world_clock_get_city_coordinates(current_city_index);
  if (coords) {
    data->current_dot_position =
        world_clock_lon_lat_to_screen(coords->longitude, coords->latitude,
                                      world_clock_calibrated_map_rect(data));
    data->target_dot_position = data->current_dot_position;
  }

  world_clock_main_window_view_model_announce_changed(&data->view_model);

  update_ampm_position(data);

  const GRect map_rect = world_clock_calibrated_map_rect(data);
  world_clock_overlay_load_cache(&data->overlay, map_rect.size.w,
                                 map_rect.size.h);

  prv_update_night_mode(data);

  world_clock_messaging_request_overlay(map_rect.size.w, map_rect.size.h);
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

  Animation *out_text = create_outbound_anim(data, direction);
  animation_set_handlers(out_text,
                         (AnimationHandlers){
                             .stopped = after_scroll_swap_text,
                         },
                         next_data_point);
  Animation *in_text = create_inbound_anim(data, direction);

  Animation *number_animation = world_clock_create_view_model_animation_numbers(
      view_model, next_data_point);
  animation_set_duration(number_animation, SCROLL_DURATION * 2);

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
    int next_city_index = world_clock_index_of_data_point(next_data_point);

    data->data_point = next_data_point;

    prv_update_night_mode(data);

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

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void init() {
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
  world_clock_messaging_init(prv_overlay_received, prv_city_data_received, data);

  window_stack_push(s_main_window, true);
  update_status_bar_time();

  tick_timer_service_subscribe(MINUTE_UNIT, prv_handle_minute_tick);
}

static void deinit() {
  WorldClockData *data = window_get_user_data(s_main_window);
  tick_timer_service_unsubscribe();
  world_clock_messaging_deinit();
  window_destroy(s_main_window);
  world_clock_overlay_deinit(&data->overlay);
  free(data);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
