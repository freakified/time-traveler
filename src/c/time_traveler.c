#include "metrics.h"
#include "time_traveler_main_window.h"
#include "time_traveler_private.h"
#include "time_traveler_data.h"
#include "time_traveler_settings.h"
#include "time_traveler_messaging.h"
#include "time_traveler_overlay.h"
#include <pebble.h>

#define PERSIST_KEY_USER_CITY_INDEX 100
#define PERSIST_KEY_CITY_DATA_BLOB 101
#define PERSIST_KEY_USER_LAT 102
#define PERSIST_KEY_USER_LON 103

static void set_data_point(WorldClockData *data, WorldClockDataPoint *dp) {
  if (!dp) return;
  data->data_point = dp;
  time_traveler_view_model_fill_all(&data->view_model, dp);

  // Show/hide arrow (arrow only for virtual CURRENT LOCATION entry)
  if (data->gps_arrow_layer) {
    bool is_current_location = time_traveler_data_is_user_location(dp);
    layer_set_hidden(data->gps_arrow_layer, !is_current_location);

    if (is_current_location) {
      GRect city_frame = layer_get_frame(text_layer_get_layer(data->city_layer));
      GFont city_font = fonts_get_system_font(LAYOUT_FONT_CITY);
      GSize text_size = graphics_text_layout_get_content_size(
          data->view_model.city, city_font,
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

static void prv_city_data_received(const uint8_t *blob, uint16_t length,
                                     int8_t user_city_index, void *context) {
  WorldClockData *data = window_get_user_data(time_traveler_main_window_get());
  if (!data) return;

  // Caching coordinates from data model
  if (time_traveler_data_has_user_location()) {
    float lat, lon;
    time_traveler_data_get_user_location(&lat, &lon);
    int32_t lat_fixed = (int32_t)(lat * 100);
    int32_t lon_fixed = (int32_t)(lon * 100);
    persist_write_data(PERSIST_KEY_USER_LAT, &lat_fixed, sizeof(lat_fixed));
    persist_write_data(PERSIST_KEY_USER_LON, &lon_fixed, sizeof(lon_fixed));
  }
  
  time_traveler_data_apply_js_blob(blob, length);

  // Cache identifying data for next startup
  persist_write_data(PERSIST_KEY_CITY_DATA_BLOB, blob, length);

  // Apply to current city and refresh display
  if (data->data_point) {
    set_data_point(data, data->data_point);
  } else if (time_traveler_num_data_points() > 0) {
    // Check if we can find current location now
    int user_idx = -1;
    for (int i = 0; i < time_traveler_num_data_points(); i++) {
      if (time_traveler_data_is_user_location(time_traveler_data_point_at(i))) {
        user_idx = i;
        break;
      }
    }
    
    if (user_idx >= 0) {
      set_data_point(data, time_traveler_data_point_at(user_idx));
    } else {
      set_data_point(data, time_traveler_data_point_at(0));
    }
  }

  if (data->data_point) {
    time_traveler_main_window_update_night_mode(data);
  }
}

static void prv_handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  WorldClockData *data = window_get_user_data(time_traveler_main_window_get());

  time_traveler_main_window_update_status_bar_time();

  if (data->data_point) {
    WorldClockDataViewNumbers numbers =
        time_traveler_data_point_view_model_numbers(data->data_point);
    time_traveler_view_model_fill_numbers(&data->view_model, numbers,
                                        data->data_point);
    const GRect map_rect = time_traveler_calibrated_map_rect(data);
    time_traveler_overlay_update(&data->overlay, map_rect.size.w, map_rect.size.h);
    layer_mark_dirty(data->map_layer);
  } else {
    time_t now = time(NULL);
    struct tm *current_local = localtime(&now);
    time_traveler_view_model_set_time(&data->view_model, current_local->tm_hour, current_local->tm_min);
    time_traveler_main_window_force_view_model_change((struct WorldClockMainWindowViewModel *)&data->view_model);
  }
}

static void init() {
  time_traveler_settings_init();
  WorldClockData *data = malloc(sizeof(WorldClockData));
  memset(data, 0, sizeof(WorldClockData));

  // Restore user coordinates
  if (persist_exists(PERSIST_KEY_USER_LAT) && persist_exists(PERSIST_KEY_USER_LON)) {
    int32_t lat_fixed, lon_fixed;
    persist_read_data(PERSIST_KEY_USER_LAT, &lat_fixed, sizeof(lat_fixed));
    persist_read_data(PERSIST_KEY_USER_LON, &lon_fixed, sizeof(lon_fixed));
    time_traveler_data_set_user_location((float)lat_fixed / 100.0f, (float)lon_fixed / 100.0f);
  }

  // Check for cached city data
  if (persist_exists(PERSIST_KEY_CITY_DATA_BLOB)) {
    // Read up to current number of cities, or what's stored
    uint8_t blob[49 * 4]; // Max possible cities
    int read = persist_read_data(PERSIST_KEY_CITY_DATA_BLOB, blob, sizeof(blob));
    if (read > 0) {
      time_traveler_data_apply_js_blob(blob, (uint16_t)read);
    }
  }

  int start_index = -1;
  if (persist_exists(PERSIST_KEY_USER_CITY_INDEX)) {
    start_index = persist_read_int(PERSIST_KEY_USER_CITY_INDEX);
  }

  // Find "CURRENT LOCATION" index and always prefer it on startup
  int user_idx = -1;
  for (int i = 0; i < time_traveler_num_data_points(); i++) {
    if (time_traveler_data_is_user_location(time_traveler_data_point_at(i))) {
      user_idx = i;
      break;
    }
  }

  if (user_idx >= 0) {
    start_index = user_idx;
  }

  if (start_index >= 0) {
    WorldClockDataPoint *dp = time_traveler_data_point_at(start_index);
    if (dp) {
      set_data_point(data, dp);
    }
  } 
  
  // If still no city (e.g. no persisted location yet), show loading and wait for JS
  if (!data->data_point) {
    time_traveler_view_model_fill_loading(&data->view_model);
  }

  time_traveler_main_window_push(data);
  time_traveler_messaging_init(prv_city_data_received, data);

  tick_timer_service_subscribe(MINUTE_UNIT, prv_handle_minute_tick);
}

static void deinit() {
  WorldClockData *data = window_get_user_data(time_traveler_main_window_get());
  tick_timer_service_unsubscribe();
  time_traveler_messaging_deinit();
  window_destroy(time_traveler_main_window_get());
  time_traveler_overlay_deinit(&data->overlay);
  free(data);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
