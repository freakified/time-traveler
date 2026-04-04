#include "time_traveller_overlay.h"
#include <string.h>

#define OVERLAY_PERSIST_KEY 2

typedef struct {
  WorldClockOverlay *overlay;
  WorldClockOverlayCompleteCallback on_complete;
  WorldClockOverlayTimeoutCallback on_timeout;
  void *context;
} OverlayCallbacks;

static void prv_timeout_callback(void *context) {
  OverlayCallbacks *callbacks = (OverlayCallbacks *)context;
  if (callbacks->overlay->valid) {
    free(callbacks);
    return;
  }
  if (callbacks->on_timeout) {
    callbacks->on_timeout(callbacks->context);
  }
  free(callbacks);
}

void time_traveller_overlay_init(WorldClockOverlay *overlay,
                              WorldClockOverlayCompleteCallback on_complete,
                              WorldClockOverlayTimeoutCallback on_timeout,
                              void *context) {
  memset(overlay, 0, sizeof(WorldClockOverlay));

  OverlayCallbacks *callbacks = malloc(sizeof(OverlayCallbacks));
  callbacks->overlay = overlay;
  callbacks->on_complete = on_complete;
  callbacks->on_timeout = on_timeout;
  callbacks->context = context;

  overlay->valid = false;
}

void time_traveller_overlay_deinit(WorldClockOverlay *overlay) {
  time_traveller_overlay_cancel_timeout(overlay);
}

static bool prv_cache_is_fresh(uint32_t timestamp) {
  if (timestamp == 0) {
    return false;
  }
  time_t now = time(NULL);
  uint32_t age = (uint32_t)(now - timestamp);
  return age < time_traveller_OVERLAY_CACHE_MAX_AGE_SECONDS;
}

bool time_traveller_overlay_load_cache(WorldClockOverlay *overlay,
                                    uint16_t current_width,
                                    uint16_t current_height) {
  if (!persist_exists(OVERLAY_PERSIST_KEY)) {
    return false;
  }

  struct {
    uint32_t version;
    uint16_t map_width;
    uint16_t map_height;
    uint16_t total_rows;
    uint32_t timestamp;
    uint8_t daylight_start[time_traveller_OVERLAY_MAX_ROWS];
    uint8_t daylight_end[time_traveller_OVERLAY_MAX_ROWS];
  } backup;

  if (persist_read_data(OVERLAY_PERSIST_KEY, &backup, sizeof(backup)) !=
      sizeof(backup)) {
    return false;
  }

  if (!prv_cache_is_fresh(backup.timestamp)) {
    return false;
  }

  if (backup.map_width != current_width ||
      backup.map_height != current_height) {
    return false;
  }

  if (backup.total_rows == 0 ||
      backup.total_rows > time_traveller_OVERLAY_MAX_ROWS) {
    return false;
  }

  overlay->version = backup.version;
  overlay->map_width = backup.map_width;
  overlay->map_height = backup.map_height;
  overlay->expected_rows = backup.total_rows;
  overlay->received_rows = backup.total_rows;
  overlay->valid = true;
  overlay->cache_timestamp = backup.timestamp;

  memcpy(overlay->daylight_start, backup.daylight_start,
         sizeof(overlay->daylight_start));
  memcpy(overlay->daylight_end, backup.daylight_end,
         sizeof(overlay->daylight_end));
  memset(overlay->row_received, 1, sizeof(overlay->row_received));

  return true;
}

void time_traveller_overlay_reset(WorldClockOverlay *overlay, uint32_t version,
                               uint16_t map_width, uint16_t map_height,
                               uint16_t total_rows) {
  overlay->version = version;
  overlay->map_width = map_width;
  overlay->map_height = map_height;
  overlay->expected_rows = total_rows;
  overlay->received_rows = 0;
  overlay->valid = false;
  memset(overlay->row_received, 0, sizeof(overlay->row_received));
}

bool time_traveller_overlay_feed_chunk(WorldClockOverlay *overlay,
                                    uint16_t row_start, uint16_t row_count,
                                    const uint8_t *data, uint16_t data_len) {
  if (!data || data_len < row_count * 2) {
    return false;
  }

  uint16_t parsed = 0;
  for (uint16_t i = 0; i < row_count; i++) {
    uint16_t row_index = row_start + i;
    if (row_index >= time_traveller_OVERLAY_MAX_ROWS) {
      break;
    }

    overlay->daylight_start[row_index] = data[i * 2];
    overlay->daylight_end[row_index] = data[i * 2 + 1];

    if (!overlay->row_received[row_index]) {
      overlay->row_received[row_index] = true;
      overlay->received_rows++;
    }
    parsed++;
  }

  return overlay->received_rows >= overlay->expected_rows;
}

bool time_traveller_overlay_query_row(const WorldClockOverlay *overlay,
                                   uint16_t row, uint8_t *start_out,
                                   uint8_t *end_out) {
  if (!overlay->valid || row >= overlay->expected_rows) {
    return false;
  }
  if (!overlay->row_received[row]) {
    return false;
  }
  *start_out = overlay->daylight_start[row];
  *end_out = overlay->daylight_end[row];
  return true;
}

void time_traveller_overlay_start_timeout(WorldClockOverlay *overlay) {
  time_traveller_overlay_cancel_timeout(overlay);

  OverlayCallbacks *callbacks = malloc(sizeof(OverlayCallbacks));
  callbacks->overlay = overlay;
  callbacks->on_complete = NULL;
  callbacks->on_timeout = NULL;
  callbacks->context = NULL;

  app_timer_register(time_traveller_OVERLAY_TIMEOUT_MS, prv_timeout_callback,
                     callbacks);
}

void time_traveller_overlay_cancel_timeout(WorldClockOverlay *overlay) {
  (void)overlay;
}

void time_traveller_overlay_save_cache(const WorldClockOverlay *overlay) {
  if (!overlay->valid) {
    return;
  }

  struct {
    uint32_t version;
    uint16_t map_width;
    uint16_t map_height;
    uint16_t total_rows;
    uint32_t timestamp;
    uint8_t daylight_start[time_traveller_OVERLAY_MAX_ROWS];
    uint8_t daylight_end[time_traveller_OVERLAY_MAX_ROWS];
  } backup;

  backup.version = overlay->version;
  backup.map_width = overlay->map_width;
  backup.map_height = overlay->map_height;
  backup.total_rows = overlay->expected_rows;
  backup.timestamp = (uint32_t)time(NULL);

  memcpy(backup.daylight_start, overlay->daylight_start,
         sizeof(backup.daylight_start));
  memcpy(backup.daylight_end, overlay->daylight_end,
         sizeof(backup.daylight_end));

  persist_write_data(OVERLAY_PERSIST_KEY, &backup, sizeof(backup));
}

bool time_traveller_overlay_is_city_night(const WorldClockOverlay *overlay,
                                       uint16_t map_width, int16_t city_row,
                                       int16_t city_col) {
  if (!overlay->valid || overlay->expected_rows == 0) {
    return false;
  }

  if (city_row < 0 || city_row >= (int16_t)overlay->expected_rows) {
    return false;
  }

  uint8_t day_start = overlay->daylight_start[city_row];
  uint8_t day_end = overlay->daylight_end[city_row];

  if (day_start == time_traveller_OVERLAY_FULL_NIGHT &&
      day_end == time_traveller_OVERLAY_FULL_NIGHT) {
    return true;
  }

  if (day_start == 0 && day_end == map_width - 1) {
    return false;
  }

  if (day_start <= day_end) {
    return (city_col < day_start || city_col > day_end);
  } else {
    return (city_col > day_end && city_col < day_start);
  }
}
