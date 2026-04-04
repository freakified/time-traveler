#include "time_traveller_messaging.h"
#include "time_traveller_overlay.h"
#include "message_keys.auto.h"
#include <string.h>

typedef struct {
  WorldClockMessageOverlayCallback on_overlay_received;
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
  // Check for overlay data
  Tuple *width_tuple = dict_find(iter, MESSAGE_KEY_overlay_map_width);
  Tuple *height_tuple = dict_find(iter, MESSAGE_KEY_overlay_map_height);
  Tuple *version_tuple = dict_find(iter, MESSAGE_KEY_overlay_version);
  Tuple *total_rows_tuple = dict_find(iter, MESSAGE_KEY_overlay_total_rows);
  Tuple *row_start_tuple = dict_find(iter, MESSAGE_KEY_overlay_row_start);
  Tuple *row_count_tuple = dict_find(iter, MESSAGE_KEY_overlay_row_count);
  Tuple *row_data_tuple = dict_find(iter, MESSAGE_KEY_overlay_row_data);

  if (width_tuple && height_tuple && version_tuple && total_rows_tuple &&
      row_start_tuple && row_count_tuple && row_data_tuple) {
    if (row_data_tuple->type != TUPLE_BYTE_ARRAY) {
      APP_LOG(APP_LOG_LEVEL_WARNING, "overlay row_data tuple type invalid: %d",
              row_data_tuple->type);
      return;
    }

    const uint16_t map_width = (uint16_t)width_tuple->value->int32;
    const uint16_t map_height = (uint16_t)height_tuple->value->int32;
    const uint32_t version = (uint32_t)version_tuple->value->int32;
    const uint16_t total_rows = (uint16_t)total_rows_tuple->value->int32;
    const uint16_t row_start = (uint16_t)row_start_tuple->value->int32;
    const uint16_t row_count = (uint16_t)row_count_tuple->value->int32;

    if (total_rows == 0 || total_rows > time_traveller_OVERLAY_MAX_ROWS ||
        row_count == 0 || row_start + row_count > total_rows) {
      APP_LOG(APP_LOG_LEVEL_WARNING, "overlay payload bounds invalid");
      return;
    }

    if (s_messaging_ctx.on_overlay_received) {
      s_messaging_ctx.on_overlay_received(
          map_width, map_height, version, total_rows, row_start, row_count,
          (const uint8_t *)row_data_tuple->value->data, row_data_tuple->length,
          s_messaging_ctx.context);
    }
    return;
  }

  // Check for city data
  Tuple *data_start = dict_find(iter, MESSAGE_KEY_city_data_start);
  Tuple *data_count = dict_find(iter, MESSAGE_KEY_city_data_count);
  Tuple *data_total = dict_find(iter, MESSAGE_KEY_city_data_total);
  Tuple *data_payload = dict_find(iter, MESSAGE_KEY_city_data);
  Tuple *user_idx = dict_find(iter, MESSAGE_KEY_user_city_index);

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

void time_traveller_messaging_init(WorldClockMessageOverlayCallback on_overlay_received,
                                WorldClockMessageCityDataCallback on_city_data_received,
                                void *context) {
  memset(&s_messaging_ctx, 0, sizeof(MessagingContext));
  s_messaging_ctx.on_overlay_received = on_overlay_received;
  s_messaging_ctx.on_city_data_received = on_city_data_received;
  s_messaging_ctx.context = context;

  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_failed(prv_outbox_failed);
  app_message_register_outbox_sent(prv_outbox_sent);
  app_message_open(time_traveller_MESSAGING_INBOX_SIZE,
                   time_traveller_MESSAGING_OUTBOX_SIZE);
}

void time_traveller_messaging_deinit(void) {
  app_message_deregister_callbacks();
}

void time_traveller_messaging_request_overlay(uint16_t map_width,
                                           uint16_t map_height) {
  DictionaryIterator *iter = NULL;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK || !iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "overlay request begin failed: %d", result);
    return;
  }

  dict_write_uint8(iter, MESSAGE_KEY_request_overlay, 1);
  dict_write_uint16(iter, MESSAGE_KEY_overlay_map_width, map_width);
  dict_write_uint16(iter, MESSAGE_KEY_overlay_map_height, map_height);
  dict_write_end(iter);

  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "overlay request send failed: %d", result);
  }
}

void time_traveller_messaging_request_city_data(void) {
  DictionaryIterator *iter = NULL;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK || !iter) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "city data request begin failed: %d", result);
    return;
  }

  dict_write_uint8(iter, MESSAGE_KEY_request_city_data, 1);
  dict_write_end(iter);

  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "city data request send failed: %d", result);
  }
}
