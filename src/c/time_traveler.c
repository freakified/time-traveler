#include "metrics.h"
#include "time_traveler_main_window.h"
#include "time_traveler_private.h"
#include "time_traveler_data.h"
#include "time_traveler_messaging.h"
#include "time_traveler_overlay.h"
#include <pebble.h>

#define PERSIST_KEY_USER_CITY_INDEX 100
#define PERSIST_KEY_CITY_DATA_BLOB 105  // bumped to avoid replaying old-format blobs
#define PERSIST_KEY_USER_LAT 102
#define PERSIST_KEY_USER_LON 103
#define PERSIST_KEY_USER_MATCHED_CITY 106

static void set_data_point(WorldClockData *data, WorldClockDataPoint *dp) {
  if (!dp) return;
  data->data_point = dp;
  time_traveler_view_model_fill_all(&data->view_model, dp);
  time_traveler_main_window_update_gps_arrow(data, dp);
}

static void prv_city_data_received(const uint8_t *blob, uint16_t length,
                                     void *context) {
  WorldClockData *data = window_get_user_data(time_traveler_main_window_get());
  if (!data) return;

  // Caching coordinates and matched city from data model
  if (time_traveler_data_has_user_location()) {
    float lat, lon;
    time_traveler_data_get_user_location(&lat, &lon);
    int32_t lat_fixed = (int32_t)(lat * 100);
    int32_t lon_fixed = (int32_t)(lon * 100);
    persist_write_data(PERSIST_KEY_USER_LAT, &lat_fixed, sizeof(lat_fixed));
    persist_write_data(PERSIST_KEY_USER_LON, &lon_fixed, sizeof(lon_fixed));
  }
  int matched = time_traveler_data_get_user_matched_city();
  persist_write_int(PERSIST_KEY_USER_MATCHED_CITY, matched);
  
  time_traveler_data_apply_js_blob(blob, length);

  // Cache identifying data for next startup
  persist_write_data(PERSIST_KEY_CITY_DATA_BLOB, blob, length);

  // If the current city was removed from the list, its pointer is now stale.
  // Fall back to user location (guaranteed to exist if GPS is on) or index 0.
  if (data->data_point &&
      time_traveler_index_of_data_point(data->data_point) < 0) {
    data->data_point = NULL;
  }

  if (data->data_point) {
    set_data_point(data, data->data_point);
  } else if (time_traveler_num_data_points() > 0) {
    // Prefer matched city, then synthetic user location, then index 0
    int matched_idx = time_traveler_data_get_user_matched_city();
    if (matched_idx >= 0 && matched_idx < time_traveler_num_data_points()) {
      set_data_point(data, time_traveler_data_point_at(matched_idx));
    } else {
      int user_idx = time_traveler_data_find_user_location_index();
      set_data_point(data, time_traveler_data_point_at(user_idx >= 0 ? user_idx : 0));
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
  WorldClockData *data = malloc(sizeof(WorldClockData));
  memset(data, 0, sizeof(WorldClockData));

  // Restore user coordinates, or estimate from timezone if none persisted
  if (persist_exists(PERSIST_KEY_USER_LAT) && persist_exists(PERSIST_KEY_USER_LON)) {
    int32_t lat_fixed, lon_fixed;
    persist_read_data(PERSIST_KEY_USER_LAT, &lat_fixed, sizeof(lat_fixed));
    persist_read_data(PERSIST_KEY_USER_LON, &lon_fixed, sizeof(lon_fixed));
    time_traveler_data_set_user_location((float)lat_fixed / 100.0f, (float)lon_fixed / 100.0f);
  } else {
    time_traveler_data_estimate_location_from_timezone();
  }

  // Restore matched city index from cache
  if (persist_exists(PERSIST_KEY_USER_MATCHED_CITY)) {
    int matched = persist_read_int(PERSIST_KEY_USER_MATCHED_CITY);
    if (matched >= 0) {
      time_traveler_data_set_user_matched_city(matched);
    }
  }

  // Check for cached city data
  if (persist_exists(PERSIST_KEY_CITY_DATA_BLOB)) {
    static uint8_t blob[MAX_JS_CITIES * CITY_BLOB_BYTES_PER_CITY]; // static to avoid stack overflow
    int read = persist_read_data(PERSIST_KEY_CITY_DATA_BLOB, blob, sizeof(blob));
    if (read > 0) {
      time_traveler_data_apply_js_blob(blob, (uint16_t)read);
    }
  }

  int start_index = -1;
  if (persist_exists(PERSIST_KEY_USER_CITY_INDEX)) {
    start_index = persist_read_int(PERSIST_KEY_USER_CITY_INDEX);
  }

  // Prefer matched city on startup if available, otherwise fall back
  // to synthetic "CURRENT LOCATION" entry
  int matched = time_traveler_data_get_user_matched_city();
  if (matched >= 0 && matched < time_traveler_num_data_points()) {
    start_index = matched;
  } else {
    int user_idx = time_traveler_data_find_user_location_index();
    if (user_idx >= 0) {
      start_index = user_idx;
    }
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
