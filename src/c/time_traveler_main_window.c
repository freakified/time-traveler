#include "time_traveler_main_window.h"
#include "time_traveler_scroll.h"
#include "metrics.h"
#include "time_traveler_animations.h"
#include "time_traveler_data.h"
#include "time_traveler_messaging.h"
#include "time_traveler_overlay.h"
#include "time_traveler_ui.h"
#include <pebble.h>
#include <string.h>

#define MARGIN LAYOUT_MARGIN
#define WORLD_MAP_BOTTOM_TRIM LAYOUT_WORLD_MAP_BOTTOM_TRIM
#define OVERLAY_TOP_TRIM 3
#define OVERLAY_BOTTOM_TRIM 3

#define GPS_ARROW_WIDTH (LAYOUT_GPS_ARROW_WIDTH)
#define GPS_ARROW_HEIGHT (LAYOUT_GPS_ARROW_HEIGHT)

#define WORLD_MAP_TOP LAYOUT_WORLD_MAP_TOP

Window *s_main_window;

Window *time_traveler_main_window_get(void) {
  return s_main_window;
}

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

void time_traveler_main_window_start_dot_animation(WorldClockData *data, int new_city_index) {
  CityCoordinates *coords = time_traveler_get_city_coordinates(new_city_index);
  if (!coords)
    return;

  GPoint target_pos =
      time_traveler_lon_lat_to_screen(coords->longitude, coords->latitude,
                                    time_traveler_calibrated_map_rect(data));

  data->target_dot_position = target_pos;
  data->dot_animation_active = true;
  data->dot_animation_progress = 0;

  static const AnimationImplementation dot_anim_impl = {
      .update = dot_animation_update};

  Animation *dot_animation = animation_create();
  animation_set_duration(dot_animation, LAYOUT_DOT_ANIMATION_DURATION_MS);
  animation_set_curve(dot_animation, AnimationCurveEaseInOut);
  animation_set_implementation(dot_animation, &dot_anim_impl);
  animation_set_handlers(dot_animation,
                         (AnimationHandlers){.stopped = dot_animation_stopped},
                         NULL);

  animation_schedule(dot_animation);
}

GPoint time_traveler_lon_lat_to_screen(float longitude, float latitude,
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

bool time_traveler_is_city_index_night(WorldClockData *data, int city_index) {
  CityCoordinates *coords = time_traveler_get_city_coordinates(city_index);
  if (!coords) {
    return false;
  }

  const GRect map_rect = time_traveler_calibrated_map_rect(data);
  GPoint city_pos = time_traveler_lon_lat_to_screen(coords->longitude,
                                                  coords->latitude, map_rect);

  int16_t row = city_pos.y - map_rect.origin.y;
  int16_t col = city_pos.x - map_rect.origin.x;

  return time_traveler_overlay_is_city_night(&data->overlay, map_rect.size.w, row,
                                           col);
}

static bool prv_is_current_city_night(WorldClockData *data) {
  int current_city_index = time_traveler_index_of_data_point(data->data_point);
  return time_traveler_is_city_index_night(data, current_city_index);
}

GRect time_traveler_calibrated_map_rect(const WorldClockData *data) {
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

  const int16_t row_start = OVERLAY_TOP_TRIM;
  const int16_t row_end = row_count - OVERLAY_BOTTOM_TRIM;

  for (int16_t row = row_start; row < row_end; ++row) {
    uint8_t day_start_value, day_end_value;
    if (!time_traveler_overlay_query_row(&data->overlay, row, &day_start_value,
                                       &day_end_value)) {
      continue;
    }

    int16_t day_start = time_traveler_ui_clamp_x(day_start_value, width);
    int16_t day_end = time_traveler_ui_clamp_x(day_end_value, width);

    if (day_start_value == time_traveler_OVERLAY_FULL_NIGHT &&
        day_end_value == time_traveler_OVERLAY_FULL_NIGHT) {
      time_traveler_ui_draw_bitmap_segment(ctx, data->world_map_night_image,
                                        map_rect, row, 0, width - 1);
      continue;
    }

    if (day_start == 0 && day_end == width - 1) {
      continue;
    }

    if (day_start <= day_end) {
      time_traveler_ui_draw_bitmap_segment(ctx, data->world_map_night_image,
                                         map_rect, row, 0, day_start - 1);
      time_traveler_ui_draw_bitmap_segment(ctx, data->world_map_night_image,
                                         map_rect, row, day_end + 1, width - 1);
    } else {
      time_traveler_ui_draw_bitmap_segment(ctx, data->world_map_night_image,
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
  const GRect map_rect = time_traveler_calibrated_map_rect(data);

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
    int current_city_index = time_traveler_index_of_data_point(data->data_point);
    CityCoordinates *coords =
        time_traveler_get_city_coordinates(current_city_index);
    if (coords) {
      dot_position = time_traveler_lon_lat_to_screen(coords->longitude,
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
  graphics_fill_circle(ctx, dot_position, LAYOUT_CITY_DOT_RADIUS);

  graphics_context_set_stroke_color(ctx, data->view_model.dot_outline_color);
  graphics_draw_circle(ctx, dot_position, LAYOUT_CITY_DOT_RADIUS);
}

static void horizontal_ruler_update_proc(Layer *layer, GContext *ctx) {
  WorldClockData *data = window_get_user_data(s_main_window);
  const GRect bounds = layer_get_bounds(layer);
  const int16_t yy = LAYOUT_RULER_LINE_Y_OFFSET;

  graphics_context_set_stroke_color(ctx, data->view_model.ruler_color);
  graphics_draw_line(ctx, GPoint(0, yy), GPoint(bounds.size.w, yy));
}

static void gps_arrow_update_proc(Layer *layer, GContext *ctx) {
  GDrawCommandImage *command_image = gdraw_command_image_create_with_resource(RESOURCE_ID_GPS_ARROW);
  if (!command_image) return;

  WorldClockData *data = window_get_user_data(s_main_window);
  GColor color = data ? data->view_model.text_color : GColorBlack;

  GDrawCommandList *command_list = gdraw_command_image_get_command_list(command_image);
  uint32_t num_commands = gdraw_command_list_get_num_commands(command_list);
  for (uint32_t i = 0; i < num_commands; i++) {
    GDrawCommand *cmd = gdraw_command_list_get_command(command_list, i);
    gdraw_command_set_stroke_color(cmd, color);
    gdraw_command_set_fill_color(cmd, color);
  }

  const GRect bounds = layer_get_bounds(layer);
  gdraw_command_image_draw(ctx, command_image, bounds.origin);
  gdraw_command_image_destroy(command_image);
}

static void update_ampm_position(WorldClockData *data) {
  if (clock_is_24h_style()) {
    layer_set_hidden(text_layer_get_layer(data->ampm_layer), true);
  } else {
    layer_set_hidden(text_layer_get_layer(data->ampm_layer), false);

    GRect time_bounds = layer_get_frame(text_layer_get_layer(data->time_layer));
    GSize time_content_size = text_layer_get_content_size(data->time_layer);

    GFont ampm_font = fonts_get_system_font(LAYOUT_FONT_AMPM);
    GSize ampm_content_size = graphics_text_layout_get_content_size(
        data->view_model.time.ampm, ampm_font,
        GRect(0, 0, LAYOUT_AMPM_LAYER_WIDTH, LAYOUT_AMPM_LAYER_HEIGHT),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);

    const int16_t screen_width = layer_get_bounds(window_get_root_layer(s_main_window)).size.w;
    const int16_t combined_width = time_content_size.w + LAYOUT_AMPM_X_OFFSET + ampm_content_size.w;
    const int16_t group_x = (screen_width - combined_width) / 2;

    GRect new_time_bounds = GRect(group_x, time_bounds.origin.y, combined_width, time_bounds.size.h);
    layer_set_frame(text_layer_get_layer(data->time_layer), new_time_bounds);
    text_layer_set_text_alignment(data->time_layer, GTextAlignmentLeft);

    int16_t ampm_x = group_x + time_content_size.w + LAYOUT_AMPM_X_OFFSET;
    int16_t ampm_y = time_bounds.origin.y + time_content_size.h - ampm_content_size.h;

    layer_set_frame(
        text_layer_get_layer(data->ampm_layer),
        GRect(ampm_x, ampm_y, ampm_content_size.w, ampm_content_size.h));
  }
}

void time_traveler_main_window_update_night_mode(WorldClockData *data) {
  bool is_night = prv_is_current_city_night(data);
  if (is_night != data->view_model.is_night) {
    time_traveler_view_model_fill_night_mode(&data->view_model, is_night);
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

void time_traveler_main_window_force_view_model_change(struct WorldClockMainWindowViewModel *arg) {
  view_model_changed(arg);
}

void time_traveler_main_window_update_status_bar_time(void) {
  if (!s_main_window) return;
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

static void main_window_load(Window *window) {
  WorldClockData *data = window_get_user_data(window);
  data->view_model.announce_changed = view_model_changed;

  Layer *window_layer = window_get_root_layer(window);
  const GRect bounds = layer_get_bounds(window_layer);
  layer_set_update_proc(window_layer, bg_update_proc);

  const int16_t screen_width = bounds.size.w;
  const int16_t base_margin = LAYOUT_BASE_MARGIN;
  const int16_t map_margin = LAYOUT_MAP_MARGIN;
  const int16_t map_width = screen_width - 2 * map_margin;

  data->world_map_base_image =
      gbitmap_create_with_resource(RESOURCE_ID_WORLD_MAP);
  data->world_map_night_image =
      gbitmap_create_with_resource(RESOURCE_ID_WORLD_MAP);
  data->world_map_image = gbitmap_create_with_resource(RESOURCE_ID_WORLD_MAP);
  time_traveler_ui_recolor(data->world_map_base_image);
  time_traveler_ui_recolor_night(data->world_map_night_image);
  time_traveler_ui_recolor(data->world_map_image);

  int16_t map_height = LAYOUT_FALLBACK_MAP_HEIGHT;
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
  const int16_t city_y = map_bottom + LAYOUT_CITY_Y_OFFSET;
  const int16_t ruler_y = map_bottom + LAYOUT_RULER_Y_OFFSET;
  const int16_t time_y = map_bottom + LAYOUT_TIME_Y_OFFSET;
  const int16_t relative_info_y = map_bottom + LAYOUT_RELATIVE_INFO_Y_OFFSET;

  data->horizontal_ruler_layer = layer_create(
      GRect(base_margin, ruler_y, screen_width - 2 * base_margin, LAYOUT_RULER_LAYER_HEIGHT));
  layer_set_update_proc(data->horizontal_ruler_layer,
                        horizontal_ruler_update_proc);
  layer_add_child(window_layer, data->horizontal_ruler_layer);

  init_text_layer(window_layer, &data->city_layer, city_y, LAYOUT_CITY_LAYER_HEIGHT, 0,
                  LAYOUT_FONT_CITY);

  GRect city_frame = layer_get_frame(text_layer_get_layer(data->city_layer));
  const int16_t city_font_height = LAYOUT_CITY_FONT_HEIGHT;
  data->gps_arrow_layer = layer_create(
      GRect(city_frame.origin.x + city_frame.size.w,
            city_frame.origin.y + (city_font_height - GPS_ARROW_HEIGHT) / 2,
            GPS_ARROW_WIDTH, GPS_ARROW_HEIGHT));
  layer_set_update_proc(data->gps_arrow_layer, gps_arrow_update_proc);
  layer_add_child(window_layer, data->gps_arrow_layer);
  layer_set_hidden(data->gps_arrow_layer, true);

  const int16_t time_top = time_y;
  init_text_layer(window_layer, &data->time_layer, time_top, LAYOUT_TIME_LAYER_HEIGHT, 0,
                  LAYOUT_FONT_TIME);

  const int16_t ampm_y = time_top;
  GRect ampm_frame = GRect(base_margin, ampm_y, LAYOUT_AMPM_LAYER_WIDTH, LAYOUT_AMPM_LAYER_HEIGHT);
  data->ampm_layer = text_layer_create(ampm_frame);
  text_layer_set_background_color(data->ampm_layer, GColorClear);
  text_layer_set_text_color(data->ampm_layer, COLOR_TEXT_DEFAULT);
  text_layer_set_font(
      data->ampm_layer,
      fonts_get_system_font(LAYOUT_FONT_AMPM));
  text_layer_set_text_alignment(data->ampm_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(data->ampm_layer));

  init_text_layer(window_layer, &data->relative_info_layer, relative_info_y, LAYOUT_RELATIVE_INFO_LAYER_HEIGHT,
                  0, LAYOUT_FONT_RELATIVE_INFO);

  init_statusbar_text_layer(window_layer, &data->fake_statusbar);

  init_statusbar_text_layer(window_layer, &data->pagination_layer);
  text_layer_set_text_alignment(data->pagination_layer, GTextAlignmentRight);

  data->dot_animation_active = false;
  data->dot_animation_progress = ANIMATION_NORMALIZED_MAX;

  int current_city_index = time_traveler_index_of_data_point(data->data_point);
  CityCoordinates *coords =
      time_traveler_get_city_coordinates(current_city_index);
  if (coords) {
    data->current_dot_position =
        time_traveler_lon_lat_to_screen(coords->longitude, coords->latitude,
                                      time_traveler_calibrated_map_rect(data));
    data->target_dot_position = data->current_dot_position;
  } else {
    data->current_dot_position = GPoint(bounds.size.w / 2, bounds.size.h / 2);
    data->target_dot_position = data->current_dot_position;
  }

  time_traveler_main_window_view_model_announce_changed(&data->view_model);

  update_ampm_position(data);

  const GRect map_rect = time_traveler_calibrated_map_rect(data);
  time_traveler_overlay_update(&data->overlay, map_rect.size.w, map_rect.size.h);
  layer_mark_dirty(data->map_layer);

  if (data->data_point) {
    time_traveler_main_window_update_night_mode(data);
  }
}

static void main_window_unload(Window *window) {
  WorldClockData *data = window_get_user_data(window);
  data->view_model.announce_changed = NULL;
  time_traveler_view_model_deinit(&data->view_model);

  layer_destroy(data->horizontal_ruler_layer);
  layer_destroy(data->map_layer);
  layer_destroy(data->gps_arrow_layer);
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

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, time_traveler_scroll_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, time_traveler_scroll_down_click_handler);
}

void time_traveler_main_window_push(WorldClockData *data) {
  if (!s_main_window) {
    s_main_window = window_create();
    window_set_click_config_provider_with_context(s_main_window,
                                                  click_config_provider, data);
    window_set_user_data(s_main_window, data);
    window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload,
                                            });
  }
  window_stack_push(s_main_window, true);
  time_traveler_main_window_update_status_bar_time();
}
