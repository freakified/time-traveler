#include <pebble.h>
#include "world_clock_private.h"
#include "world_clock_animations.h"
#include "world_clock_data.h"

#define STATUS_BAR_HEIGHT 16
#define DST_PERSIST_KEY 1

static Window *s_main_window;

static const int16_t MARGIN = 8;
static const int16_t ICON_DIMENSIONS = 48;

////////////////////
// update procs for our three custom layers

static void bg_update_proc(Layer *layer, GContext *ctx) {
  WorldClockData *data = window_get_user_data(s_main_window);
  WorldClockMainWindowViewModel *model = &data->view_model;
  const GRect bounds = layer_get_bounds(layer);

  int16_t y = (model->bg_color.to_bottom_normalized * bounds.size.h) / ANIMATION_NORMALIZED_MAX;

  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(model->bg_color.bottom, GColorWhite));
  GRect rect_top = bounds;
  rect_top.size.h = y;
  graphics_fill_rect(ctx, rect_top, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(model->bg_color.top, GColorWhite));
  GRect rect_bottom = bounds;
  rect_bottom.origin.y += y;
  graphics_fill_rect(ctx, rect_bottom, 0, GCornerNone);
}

static void horizontal_ruler_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  // y relative to layer's bounds to support clipping after some vertical scrolling
  const int16_t yy = 11;

  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
  graphics_draw_line(ctx, GPoint(0, yy), GPoint(bounds.size.w, yy));
}

////////////////////
// App boilerplate

//! helper to construct the various text layers as they appear in this app
static GRect init_text_layer(Layer *parent_layer, TextLayer **text_layer, int16_t y, int16_t h, int16_t additional_right_margin, char *font_key) {
  // why "-1" (and then "+2")? because for this font we need to compensate for weird white-spacing
  const int16_t font_compensator = strcmp(font_key, FONT_KEY_LECO_38_BOLD_NUMBERS) == 0 ? 3 : 1;

  const GRect frame = GRect(MARGIN - font_compensator, y, layer_get_bounds(parent_layer).size.w - 2 * MARGIN + 2 * font_compensator - additional_right_margin, h);

  *text_layer = text_layer_create(frame);
  text_layer_set_background_color(*text_layer, GColorClear);
  text_layer_set_text_color(*text_layer, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
  text_layer_set_font(*text_layer, fonts_get_system_font(font_key));
  layer_add_child(parent_layer, text_layer_get_layer(*text_layer));
  return frame;
}

void init_statusbar_text_layer(Layer *parent, TextLayer **layer) {
  init_text_layer(parent, layer, 0, STATUS_BAR_HEIGHT, 0, FONT_KEY_GOTHIC_14);
  GRect sb_bounds = layer_get_bounds(text_layer_get_layer(*layer));
  sb_bounds.origin.y -= 1;
  layer_set_bounds(text_layer_get_layer(*layer), sb_bounds);
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

    // Format time for status bar (12-hour format)
    static char time_str[] = "00:00";
    strftime(time_str, sizeof(time_str), "%I:%M %p", local_time);

    // Remove leading zero from hour
    if (time_str[0] == '0') {
      memmove(time_str, time_str + 1, strlen(time_str));
    }

    text_layer_set_text(data->fake_statusbar, time_str);
}

static void prv_handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  WorldClockData *data = window_get_user_data(s_main_window);

   // Update status bar time
   update_status_bar_time();

   // Update world clock times
    WorldClockDataViewNumbers numbers = world_clock_data_point_view_model_numbers(data->data_point);
    world_clock_view_model_fill_numbers(&data->view_model, numbers, data->data_point);
}

static void view_model_changed(struct WorldClockMainWindowViewModel *arg) {
  WorldClockMainWindowViewModel *model = (WorldClockMainWindowViewModel *)arg;

  WorldClockData *data = window_get_user_data(s_main_window);

  text_layer_set_text(data->city_layer, model->city);
  text_layer_set_text(data->time_layer, model->time.text);
  text_layer_set_text(data->relative_info_layer, model->relative_info.text);
  text_layer_set_text(data->pagination_layer, model->pagination.text);

  // Force redraw by temporarily clearing and resetting the text
  text_layer_set_text(data->relative_info_layer, "");
  text_layer_set_text(data->relative_info_layer, model->relative_info.text);

  // make sure to redraw (if no string pointer changed none of the layers would be dirty)
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

  data->horizontal_ruler_layer = layer_create(GRect(MARGIN, 40, bounds.size.w - 2 * MARGIN, 20));
  layer_set_update_proc(data->horizontal_ruler_layer, horizontal_ruler_update_proc);
  layer_add_child(window_layer, data->horizontal_ruler_layer);

  init_text_layer(window_layer, &data->city_layer, 23, 30, 0, FONT_KEY_GOTHIC_18_BOLD);
  const int16_t time_top = 49;
  init_text_layer(window_layer, &data->time_layer, time_top, 40, 0, FONT_KEY_LECO_38_BOLD_NUMBERS);
  init_text_layer(window_layer, &data->relative_info_layer, 91, 19, 0, FONT_KEY_GOTHIC_14);

   init_statusbar_text_layer(window_layer, &data->fake_statusbar);

  init_statusbar_text_layer(window_layer, &data->pagination_layer);
  text_layer_set_text_alignment(data->pagination_layer, GTextAlignmentRight);

  // propagate all view model content to the UI
  world_clock_main_window_view_model_announce_changed(&data->view_model);
}

static void main_window_unload(Window *window) {
  WorldClockData *data = window_get_user_data(window);
  data->view_model.announce_changed = NULL;
  world_clock_view_model_deinit(&data->view_model);

  layer_destroy(data->horizontal_ruler_layer);
  text_layer_destroy(data->city_layer);
  text_layer_destroy(data->time_layer);
  text_layer_destroy(data->relative_info_layer);
  text_layer_destroy(data->fake_statusbar);
  text_layer_destroy(data->pagination_layer);
}

static void after_scroll_swap_text(Animation *animation, bool finished, void *context) {
  WorldClockData *data = window_get_user_data(s_main_window);
  WorldClockDataPoint *data_point = context;

  world_clock_view_model_fill_strings_and_pagination(&data->view_model, data_point);
  world_clock_view_model_fill_numbers(&data->view_model, world_clock_data_point_view_model_numbers(data_point), data_point);
}

static Animation *create_anim_scroll_out(Layer *layer, uint32_t duration, int16_t dy) {
  GPoint to_origin = GPoint(0, dy);
  Animation *result = (Animation *) property_animation_create_bounds_origin(layer, NULL, &to_origin);
  animation_set_duration(result, duration);
  animation_set_curve(result, AnimationCurveLinear);
  return result;
}

static Animation *create_anim_scroll_in(Layer *layer, uint32_t duration, int16_t dy) {
  GPoint from_origin = GPoint(0, dy);
  Animation *result = (Animation *) property_animation_create_bounds_origin(layer, &from_origin, &GPointZero);
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

static Animation *create_outbound_anim(WorldClockData *data, ScrollDirection direction) {
  const int16_t to_dy = (direction == ScrollDirectionDown) ? -SCROLL_DIST_OUT : SCROLL_DIST_OUT;

  Animation *out_city = create_anim_scroll_out(text_layer_get_layer(data->city_layer), SCROLL_DURATION, to_dy);
  Animation *out_ruler = create_anim_scroll_out(data->horizontal_ruler_layer, SCROLL_DURATION, to_dy);

  return animation_spawn_create(out_city, out_ruler, NULL);
}

static Animation *create_inbound_anim(WorldClockData *data, ScrollDirection direction) {
  const int16_t from_dy = (direction == ScrollDirectionDown) ? -SCROLL_DIST_IN : SCROLL_DIST_IN;

  Animation *in_city = create_anim_scroll_in(text_layer_get_layer(data->city_layer), SCROLL_DURATION, from_dy);
  Animation *in_time = create_anim_scroll_in(text_layer_get_layer(data->time_layer), SCROLL_DURATION, from_dy);
  Animation *in_relative_info = create_anim_scroll_in(text_layer_get_layer(data->relative_info_layer), SCROLL_DURATION, from_dy);
  Animation *in_ruler = create_anim_scroll_in(data->horizontal_ruler_layer, SCROLL_DURATION, from_dy);

  return animation_spawn_create(in_city, in_time, in_relative_info, in_ruler, NULL);
}

static Animation *animation_for_scroll(WorldClockData *data, ScrollDirection direction, WorldClockDataPoint *next_data_point) {
  WorldClockMainWindowViewModel *view_model = &data->view_model;

  // sliding texts
  Animation *out_text = create_outbound_anim(data, direction);
  animation_set_handlers(out_text, (AnimationHandlers) {
    .stopped = after_scroll_swap_text,
  }, next_data_point);
  Animation *in_text = create_inbound_anim(data, direction);

  // changing numbers (time and offset)
  Animation *number_animation = world_clock_create_view_model_animation_numbers(view_model, next_data_point);
  animation_set_duration(number_animation, SCROLL_DURATION * 2);

  // scrolling background color
  Animation *bg_animation = world_clock_create_view_model_animation_bgcolor(view_model, next_data_point);
  animation_set_duration(bg_animation, BACKGROUND_SCROLL_DURATION);
  animation_set_reverse(bg_animation, (direction == ScrollDirectionDown));

  return animation_spawn_create(animation_sequence_create(out_text, in_text, NULL), number_animation, bg_animation, NULL);
}

static Animation *animation_for_bounce(WorldClockData *data, ScrollDirection direction) {
  return create_inbound_anim(data, direction);
}

static void ask_for_scroll(WorldClockData *data, ScrollDirection direction) {
  int delta = direction == ScrollDirectionUp ? -1 : +1;
  WorldClockDataPoint *next_data_point = world_clock_data_point_delta(data->data_point, delta);

  Animation *scroll_animation;

  if (!next_data_point) {
    scroll_animation = animation_for_bounce(data, direction);
  } else {
    // data point switches immediately
    data->data_point = next_data_point;
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

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  WorldClockData *data = context;
  // Toggle DST for current city
  data->data_point->is_dst = !data->data_point->is_dst;
  save_dst_settings();
  // Refresh display
  WorldClockDataViewNumbers numbers = world_clock_data_point_view_model_numbers(data->data_point);
  world_clock_view_model_fill_numbers(&data->view_model, numbers, data->data_point);
  text_layer_set_text(data->relative_info_layer, data->view_model.relative_info.text);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, select_long_click_handler, NULL);
}

static void init() {
  load_dst_settings();

  WorldClockData *data = malloc(sizeof(WorldClockData));
  memset(data, 0, sizeof(WorldClockData));

  WorldClockDataPoint *dp = world_clock_data_point_at(0);
  set_data_point(data, dp);

  s_main_window = window_create();
  window_set_click_config_provider_with_context(s_main_window, click_config_provider, data);
  window_set_user_data(s_main_window, data);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
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
