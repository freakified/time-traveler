#include "time_traveler_scroll.h"
#include "time_traveler_main_window.h"
#include "time_traveler_animations.h"
#include "time_traveler_data.h"
#include "metrics.h"
#include <pebble.h>

static void after_scroll_swap_text(Animation *animation, bool finished,
                                   void *context) {
  WorldClockData *data = window_get_user_data(time_traveler_main_window_get());
  WorldClockDataPoint *data_point = context;

  time_traveler_view_model_fill_strings_and_pagination(&data->view_model,
                                                     data_point);
  time_traveler_view_model_fill_numbers(
      &data->view_model, time_traveler_data_point_view_model_numbers(data_point),
      data_point);

  if (data->gps_arrow_layer) {
    bool is_current_location = time_traveler_data_is_user_location(data_point);
    layer_set_hidden(data->gps_arrow_layer, !is_current_location);

    if (is_current_location) {
      GRect city_frame = layer_get_frame(text_layer_get_layer(data->city_layer));
      GFont city_font = fonts_get_system_font(LAYOUT_FONT_CITY);
      GSize text_size = graphics_text_layout_get_content_size(
          data_point->city, city_font,
          GRect(0, 0, city_frame.size.w, city_frame.size.h),
          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);

      const int16_t spacing = LAYOUT_GPS_ARROW_TEXT_SPACING;
      int16_t text_right_x;
      if (PBL_IF_ROUND_ELSE(true, false)) {
        int16_t text_left_x = city_frame.origin.x + (city_frame.size.w - text_size.w) / 2;
        text_right_x = text_left_x + text_size.w;

        int16_t half_total = (text_size.w + spacing + LAYOUT_GPS_ARROW_WIDTH) / 2;
        int16_t center = city_frame.origin.x + city_frame.size.w / 2;
        int16_t new_text_left = center - half_total;
        GRect shifted_frame = city_frame;
        shifted_frame.origin.x = new_text_left;
        layer_set_frame(text_layer_get_layer(data->city_layer), shifted_frame);
        text_layer_set_text_alignment(data->city_layer, GTextAlignmentLeft);
        layer_mark_dirty(text_layer_get_layer(data->city_layer));

        text_right_x = new_text_left + text_size.w;
      } else {
        text_right_x = city_frame.origin.x + text_size.w;
      }
      layer_set_frame(data->gps_arrow_layer,
                      GRect(text_right_x + spacing + LAYOUT_GPS_ARROW_POSITION_ADJUST,
                            city_frame.origin.y +
                                (city_frame.size.h - LAYOUT_GPS_ARROW_HEIGHT) / 2 + LAYOUT_GPS_ARROW_POSITION_ADJUST,
                            LAYOUT_GPS_ARROW_WIDTH, LAYOUT_GPS_ARROW_HEIGHT));
      layer_mark_dirty(data->gps_arrow_layer);
    } else if (PBL_IF_ROUND_ELSE(true, false)) {
      GRect city_frame = layer_get_frame(text_layer_get_layer(data->city_layer));
      const int16_t current_margin = LAYOUT_BASE_MARGIN;
      GRect restored_frame = GRect(current_margin + LAYOUT_ROUND_CITY_FRAME_ADJUST, city_frame.origin.y,
                                   city_frame.size.w, city_frame.size.h);
      layer_set_frame(text_layer_get_layer(data->city_layer), restored_frame);
      text_layer_set_text_alignment(data->city_layer, GTextAlignmentCenter);
      layer_mark_dirty(text_layer_get_layer(data->city_layer));
    }
  }
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

static Animation *create_outbound_anim(WorldClockData *data,
                                       ScrollDirection direction) {
  const int16_t to_dy =
      (direction == ScrollDirectionDown) ? -LAYOUT_SCROLL_DIST_OUT : LAYOUT_SCROLL_DIST_OUT;

  Animation *out_city = create_anim_scroll_out(
      text_layer_get_layer(data->city_layer), LAYOUT_SCROLL_DURATION_MS, to_dy);
  Animation *out_ruler = create_anim_scroll_out(data->horizontal_ruler_layer,
                                                LAYOUT_SCROLL_DURATION_MS, to_dy);
  Animation *out_gps_arrow = create_anim_scroll_out(data->gps_arrow_layer,
                                                    LAYOUT_SCROLL_DURATION_MS, to_dy);

  return animation_spawn_create(out_city, out_ruler, out_gps_arrow, NULL);
}

static Animation *create_inbound_anim(WorldClockData *data,
                                      ScrollDirection direction) {
  const int16_t from_dy =
      (direction == ScrollDirectionDown) ? -LAYOUT_SCROLL_DIST_IN : LAYOUT_SCROLL_DIST_IN;

  Animation *in_city = create_anim_scroll_in(
      text_layer_get_layer(data->city_layer), LAYOUT_SCROLL_DURATION_MS, from_dy);
  Animation *in_time = create_anim_scroll_in(
      data->time_row_layer, LAYOUT_SCROLL_DURATION_MS, from_dy);
  Animation *in_relative_info =
      create_anim_scroll_in(text_layer_get_layer(data->relative_info_layer),
                            LAYOUT_SCROLL_DURATION_MS, from_dy);
  Animation *in_ruler = create_anim_scroll_in(data->horizontal_ruler_layer,
                                              LAYOUT_SCROLL_DURATION_MS, from_dy);
  Animation *in_gps_arrow = create_anim_scroll_in(data->gps_arrow_layer,
                                                  LAYOUT_SCROLL_DURATION_MS, from_dy);

  return animation_spawn_create(in_city, in_time, in_relative_info, in_ruler,
                                in_gps_arrow, NULL);
}

static void prv_scroll_animation_stopped(Animation *animation, bool finished,
                                          void *context) {
  WorldClockData *data = window_get_user_data(time_traveler_main_window_get());
  time_traveler_main_window_force_view_model_change((struct WorldClockMainWindowViewModel *)&data->view_model);
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

  Animation *number_animation = time_traveler_create_view_model_animation_numbers(
      view_model, next_data_point);
  animation_set_duration(number_animation, LAYOUT_SCROLL_DURATION_MS * 2);

  Animation *bg_animation = time_traveler_create_view_model_animation_bgcolor(
      view_model, next_data_point);
  animation_set_duration(bg_animation, LAYOUT_BACKGROUND_SCROLL_DURATION_MS);
  animation_set_reverse(bg_animation, (direction == ScrollDirectionDown));

  Animation *text_sequence = animation_sequence_create(out_text, in_text, NULL);
  animation_set_handlers(text_sequence,
                         (AnimationHandlers){
                             .stopped = prv_scroll_animation_stopped,
                         },
                         NULL);

  return animation_spawn_create(
      text_sequence, number_animation,
      bg_animation, NULL);
}

static Animation *animation_for_bounce(WorldClockData *data,
                                        ScrollDirection direction) {
  Animation *inbound = create_inbound_anim(data, direction);
  animation_set_handlers(inbound,
                         (AnimationHandlers){
                             .stopped = prv_scroll_animation_stopped,
                         },
                         NULL);
  return inbound;
}

void time_traveler_scroll_ask(WorldClockData *data, ScrollDirection direction) {
  int delta = direction == ScrollDirectionUp ? -1 : +1;
  WorldClockDataPoint *next_data_point =
      time_traveler_data_point_delta(data->data_point, delta);

  Animation *scroll_animation;

  if (!next_data_point) {
    scroll_animation = animation_for_bounce(data, direction);
  } else {
    if (data->gps_arrow_layer) {
      layer_set_hidden(data->gps_arrow_layer, true);
    }
    
    int next_city_index = time_traveler_index_of_data_point(next_data_point);

    data->data_point = next_data_point;

    time_traveler_main_window_update_night_mode(data);
    time_traveler_main_window_start_dot_animation(data, next_city_index);

    scroll_animation = animation_for_scroll(data, direction, next_data_point);
  }

  animation_unschedule(data->previous_animation);
  animation_schedule(scroll_animation);
  data->previous_animation = scroll_animation;
}

void time_traveler_scroll_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  WorldClockData *data = context;
  time_traveler_scroll_ask(data, ScrollDirectionUp);
}

void time_traveler_scroll_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  WorldClockData *data = context;
  time_traveler_scroll_ask(data, ScrollDirectionDown);
}

void time_traveler_scroll_up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  WorldClockData *data = context;
  // Go to first city in list
  WorldClockDataPoint *first_data_point = time_traveler_data_point_at(0);
  if (first_data_point && data->data_point != first_data_point) {
    if (data->gps_arrow_layer) {
      layer_set_hidden(data->gps_arrow_layer, true);
    }
    
    int delta = time_traveler_index_of_data_point(data->data_point);
    ScrollDirection direction = (delta > 0) ? ScrollDirectionUp : ScrollDirectionDown;
    
    data->data_point = first_data_point;
    time_traveler_main_window_update_night_mode(data);
    time_traveler_main_window_start_dot_animation(data, 0);
    
    Animation *scroll_animation = animation_for_scroll(data, direction, first_data_point);
    animation_unschedule(data->previous_animation);
    animation_schedule(scroll_animation);
    data->previous_animation = scroll_animation;
  }
}

void time_traveler_scroll_down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  WorldClockData *data = context;
  // Go to last city in list
  int last_index = time_traveler_num_data_points() - 1;
  WorldClockDataPoint *last_data_point = time_traveler_data_point_at(last_index);
  if (last_data_point && data->data_point != last_data_point) {
    if (data->gps_arrow_layer) {
      layer_set_hidden(data->gps_arrow_layer, true);
    }
    
    int delta = time_traveler_index_of_data_point(data->data_point);
    ScrollDirection direction = (delta < last_index) ? ScrollDirectionDown : ScrollDirectionUp;
    
    data->data_point = last_data_point;
    time_traveler_main_window_update_night_mode(data);
    time_traveler_main_window_start_dot_animation(data, last_index);
    
    Animation *scroll_animation = animation_for_scroll(data, direction, last_data_point);
    animation_unschedule(data->previous_animation);
    animation_schedule(scroll_animation);
    data->previous_animation = scroll_animation;
  }
}

void time_traveler_scroll_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  WorldClockData *data = context;
  // Go to user's current location (user city)
  int user_idx = -1;
  for (int i = 0; i < time_traveler_num_data_points(); i++) {
    if (time_traveler_data_is_user_location(time_traveler_data_point_at(i))) {
      user_idx = i;
      break;
    }
  }

  if (user_idx >= 0) {
    WorldClockDataPoint *user_city = time_traveler_data_point_at(user_idx);
    if (user_city && data->data_point != user_city) {
      if (data->gps_arrow_layer) {
        layer_set_hidden(data->gps_arrow_layer, true);
      }
      
      int current_index = time_traveler_index_of_data_point(data->data_point);
      ScrollDirection direction = (current_index < user_idx) ? ScrollDirectionDown : ScrollDirectionUp;
      
      data->data_point = user_city;
      time_traveler_main_window_update_night_mode(data);
      time_traveler_main_window_start_dot_animation(data, user_idx);
      
      Animation *scroll_animation = animation_for_scroll(data, direction, user_city);
      animation_unschedule(data->previous_animation);
      animation_schedule(scroll_animation);
      data->previous_animation = scroll_animation;
    }
  }
}
