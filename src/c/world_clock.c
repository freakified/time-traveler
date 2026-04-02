#include "pdc-transform/pdc-transform.h"
#include "world_clock_animations.h"
#include "world_clock_data.h"
#include "world_clock_private.h"
#include <pebble.h>

// Forward declarations for functions not in headers yet
int world_clock_index_of_data_point(WorldClockDataPoint *dp);
CityCoordinates *world_clock_get_city_coordinates(int city_index);

#define DST_PERSIST_KEY 1

static Window *s_main_window;

#define MARGIN 8
#define SCALE_FACTOR_BASE 10
#define WORLD_MAP_NATIVE_WIDTH 109
#define WORLD_MAP_NATIVE_HEIGHT 50

// Calibrated against the provided native PDC anchor pixels and lightly smoothed
// to avoid overfitting the low-resolution source art.
#define WORLD_MAP_CAL_LEFT 0
#define WORLD_MAP_CAL_TOP -1
#define WORLD_MAP_CAL_WIDTH 107
#define WORLD_MAP_CAL_HEIGHT 56

#if defined(PBL_PLATFORM_GABBRO)
#define WORLD_MAP_TOP 45
#else
#define WORLD_MAP_TOP STATUS_BAR_LAYER_HEIGHT
#endif

////////////////////
// update procs for our three custom layers

static void bg_update_proc(Layer *layer, GContext *ctx) {
  WorldClockData *data = window_get_user_data(s_main_window);
  WorldClockMainWindowViewModel *model = &data->view_model;
  const GRect bounds = layer_get_bounds(layer);

  int16_t y = (model->bg_color.to_bottom_normalized * bounds.size.h) /
              ANIMATION_NORMALIZED_MAX;

  graphics_context_set_fill_color(
      ctx, PBL_IF_COLOR_ELSE(model->bg_color.bottom, GColorWhite));
  GRect rect_top = bounds;
  rect_top.size.h = y;
  graphics_fill_rect(ctx, rect_top, 0, GCornerNone);

  graphics_context_set_fill_color(
      ctx, PBL_IF_COLOR_ELSE(model->bg_color.top, GColorWhite));
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
  const GRect draw_rect = data->world_map_draw_rect;
  if (draw_rect.size.w <= 0 || draw_rect.size.h <= 0) {
    return draw_rect;
  }

  const int16_t left =
      draw_rect.origin.x +
      (WORLD_MAP_CAL_LEFT * draw_rect.size.w) / WORLD_MAP_NATIVE_WIDTH;
  const int16_t top =
      draw_rect.origin.y +
      (WORLD_MAP_CAL_TOP * draw_rect.size.h) / WORLD_MAP_NATIVE_HEIGHT;
  const int16_t right =
      draw_rect.origin.x +
      ((WORLD_MAP_CAL_LEFT + WORLD_MAP_CAL_WIDTH) * draw_rect.size.w) /
          WORLD_MAP_NATIVE_WIDTH;
  const int16_t bottom =
      draw_rect.origin.y +
      ((WORLD_MAP_CAL_TOP + WORLD_MAP_CAL_HEIGHT) * draw_rect.size.h) /
          WORLD_MAP_NATIVE_HEIGHT;

  return GRect(left, top, right - left, bottom - top);
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

static void map_update_proc(Layer *layer, GContext *ctx) {
  WorldClockData *data = window_get_user_data(s_main_window);
  const GRect bounds = layer_get_bounds(layer);
  const GRect map_rect = calibrated_map_rect(data);

  // Draw the scaled world map PDC within its fitted rect.
  if (data->world_map_image) {
    gdraw_command_image_draw(ctx, data->world_map_image,
                             data->world_map_draw_rect.origin);
  }

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

  // Draw crosshair lines (subtle white lines)
  graphics_context_set_stroke_color(ctx, GColorWhite);

  // Horizontal line across the entire width
  graphics_draw_line(ctx, GPoint(0, dot_position.y),
                     GPoint(bounds.size.w, dot_position.y));

  // Vertical line across the entire height
  graphics_draw_line(ctx, GPoint(dot_position.x, 0),
                     GPoint(dot_position.x, bounds.size.h));

  // Draw single animated dot
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_circle(ctx, dot_position, 4);

  // Draw border for better visibility
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_circle(ctx, dot_position, 4);
}

static void horizontal_ruler_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  // y relative to layer's bounds to support clipping after some vertical
  // scrolling
  const int16_t yy = 11;

  graphics_context_set_stroke_color(
      ctx, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
  graphics_draw_line(ctx, GPoint(0, yy), GPoint(bounds.size.w, yy));
}

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
  text_layer_set_text_color(*text_layer,
                            PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
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
  text_layer_set_text_color(*layer,
                            PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
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

  // Load and scale the world map PDC image to fit the available width.
  data->world_map_image =
      gdraw_command_image_create_with_resource(RESOURCE_ID_WORLD_MAP);

  int16_t map_height = 72;
  data->world_map_draw_rect = GRectZero;
  if (data->world_map_image) {
    const GSize image_size =
        gdraw_command_image_get_bounds_size(data->world_map_image);
    int scale10 =
        (int)(((int32_t)map_width * SCALE_FACTOR_BASE) / image_size.w);
    if (scale10 < 1) {
      scale10 = 1;
    }

    pdc_transform_scale_image(data->world_map_image, scale10);

    const GSize scaled_size =
        gdraw_command_image_get_bounds_size(data->world_map_image);
    map_height = scaled_size.h;
    data->world_map_draw_rect =
        GRect((map_width - scaled_size.w) / 2, 0, scaled_size.w, scaled_size.h);
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
  text_layer_set_text_color(data->ampm_layer,
                            PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
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
}

static void main_window_unload(Window *window) {
  WorldClockData *data = window_get_user_data(window);
  data->view_model.announce_changed = NULL;
  world_clock_view_model_deinit(&data->view_model);

  layer_destroy(data->horizontal_ruler_layer);
  layer_destroy(data->map_layer);
  gdraw_command_image_destroy(data->world_map_image);
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
  window_stack_push(s_main_window, true);
  update_status_bar_time();
  tick_timer_service_subscribe(MINUTE_UNIT, prv_handle_minute_tick);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
