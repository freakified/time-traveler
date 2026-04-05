#include "time_traveler_messaging.h"
#include "time_traveler_overlay.h"
#include "time_traveler_settings.h"
#include "time_traveler_data.h"
#include <string.h>

typedef struct {
  WorldClockMessageCityDataCallback on_city_data_received;
  void *context;
  // City data reassembly buffer
  uint8_t city_data_buf[256]; // 49 cities * 4 bytes = 196 bytes max
  uint16_t city_data_total;
  uint16_t city_data_received;
  int8_t city_data_user_index;
} MessagingContext;

static MessagingContext s_messaging_ctx;

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  // Check for settings update first
  Tuple *pinned_cities_tuple = dict_find(iter, MESSAGE_KEY_SETTING_PINNED_CITIES);
  if (pinned_cities_tuple) {
    if (pinned_cities_tuple->type == TUPLE_BYTE_ARRAY) {
      const uint8_t *indices = pinned_cities_tuple->value->data;
      int count = pinned_cities_tuple->length;
      APP_LOG(APP_LOG_LEVEL_INFO, "Received %d pinned cities indices", count);
      
      time_traveler_settings_set_pinned_indices(indices, count);
      
      // Request new city data from JS (which will only include pinned cities)
      time_traveler_messaging_request_city_data();
    }
    return;
  }

  // Check for city data
  Tuple *data_start = dict_find(iter, MESSAGE_KEY_CITY_DATA_START);
  Tuple *data_count = dict_find(iter, MESSAGE_KEY_CITY_DATA_COUNT);
  Tuple *data_total = dict_find(iter, MESSAGE_KEY_CITY_DATA_TOTAL);
  Tuple *data_payload = dict_find(iter, MESSAGE_KEY_CITY_DATA);
  Tuple *user_idx = dict_find(iter, MESSAGE_KEY_USER_CITY_INDEX);
  Tuple *user_lat_tuple = dict_find(iter, MESSAGE_KEY_USER_LAT);
  Tuple *user_lon_tuple = dict_find(iter, MESSAGE_KEY_USER_LON);

  if (user_lat_tuple && user_lon_tuple) {
    // Latitude and longitude are sent as fixed-point integers (multiplied by 100)
    float lat = (float)user_lat_tuple->value->int32 / 100.0f;
    float lon = (float)user_lon_tuple->value->int32 / 100.0f;
    time_traveler_data_set_user_location(lat, lon);
  }

  if (data_start && data_count && data_total && data_payload) {
    if (data_payload->type != TUPLE_BYTE_ARRAY) {
      APP_LOG(APP_LOG_LEVEL_WARNING, "city_data tuple type invalid: %d",
              data_payload->type);
      return;
    }

    uint16_t start = (uint16_t)data_start->value->int32;
    uint16_t count = (uint16_t)data_count->value->int32;
    uint16_t total = (uint16_t)data_total->value->int32;

    if (total == 0 || total > sizeof(s_messaging_ctx.city_data_buf) ||
        start + count > total ||
        data_payload->length < count) {
      APP_LOG(APP_LOG_LEVEL_WARNING, "city_data payload bounds invalid");
      return;
    }

    // First chunk: reset state
    if (start == 0) {
      s_messaging_ctx.city_data_total = total;
      s_messaging_ctx.city_data_received = 0;
      s_messaging_ctx.city_data_user_index = -1;
      if (user_idx) {
        s_messaging_ctx.city_data_user_index = (int8_t)user_idx->value->int32;
      }
    }

    // Copy chunk into buffer
    memcpy(&s_messaging_ctx.city_data_buf[start],
           data_payload->value->data, count);
    s_messaging_ctx.city_data_received += count;

    // All chunks received
    if (s_messaging_ctx.city_data_received >= s_messaging_ctx.city_data_total) {
      if (s_messaging_ctx.on_city_data_received) {
        s_messaging_ctx.on_city_data_received(
            s_messaging_ctx.city_data_buf,
            s_messaging_ctx.city_data_total,
            s_messaging_ctx.city_data_user_index,
            s_messaging_ctx.context);
      }
    }
    return;
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

void time_traveler_messaging_init(WorldClockMessageCityDataCallback on_city_data_received,
                                void *context) {
  memset(&s_messaging_ctx, 0, sizeof(MessagingContext));
  s_messaging_ctx.on_city_data_received = on_city_data_received;
  s_messaging_ctx.context = context;

  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_failed(prv_outbox_failed);
  app_message_register_outbox_sent(prv_outbox_sent);
  app_message_open(time_traveler_MESSAGING_INBOX_SIZE,
                   time_traveler_MESSAGING_OUTBOX_SIZE);
}

void time_traveler_messaging_deinit(void) {
  app_message_deregister_callbacks();
}

void time_traveler_messaging_request_city_data(void) {
  DictionaryIterator *iter = NULL;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK || !iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "city data request begin failed: %d", result);
    return;
  }

  dict_write_uint8(iter, MESSAGE_KEY_REQUEST_CITY_DATA, 1);
  dict_write_end(iter);

  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "city data request send failed: %d", result);
  }
}
