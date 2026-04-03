#include "world_clock_messaging.h"
#include "world_clock_overlay.h"
#include "message_keys.auto.h"
#include <string.h>

typedef struct {
  WorldClockMessageOverlayCallback on_overlay_received;
  void *context;
} MessagingContext;

static MessagingContext s_messaging_ctx;

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *width_tuple = dict_find(iter, MESSAGE_KEY_overlay_map_width);
  Tuple *height_tuple = dict_find(iter, MESSAGE_KEY_overlay_map_height);
  Tuple *version_tuple = dict_find(iter, MESSAGE_KEY_overlay_version);
  Tuple *total_rows_tuple = dict_find(iter, MESSAGE_KEY_overlay_total_rows);
  Tuple *row_start_tuple = dict_find(iter, MESSAGE_KEY_overlay_row_start);
  Tuple *row_count_tuple = dict_find(iter, MESSAGE_KEY_overlay_row_count);
  Tuple *row_data_tuple = dict_find(iter, MESSAGE_KEY_overlay_row_data);

  if (!width_tuple || !height_tuple || !version_tuple || !total_rows_tuple ||
      !row_start_tuple || !row_count_tuple || !row_data_tuple) {
    return;
  }

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

  if (total_rows == 0 || total_rows > WORLD_CLOCK_OVERLAY_MAX_ROWS ||
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
}

static void prv_inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "inbox dropped: %d", reason);
}

static void prv_outbox_failed(DictionaryIterator *iter, AppMessageResult reason,
                              void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "outbox failed: %d", reason);
}

static void prv_outbox_sent(DictionaryIterator *iter, void *context) {}

void world_clock_messaging_init(WorldClockMessageOverlayCallback on_overlay_received,
                                void *context) {
  memset(&s_messaging_ctx, 0, sizeof(MessagingContext));
  s_messaging_ctx.on_overlay_received = on_overlay_received;
  s_messaging_ctx.context = context;

  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_failed(prv_outbox_failed);
  app_message_register_outbox_sent(prv_outbox_sent);
  app_message_open(WORLD_CLOCK_MESSAGING_INBOX_SIZE,
                   WORLD_CLOCK_MESSAGING_OUTBOX_SIZE);
}

void world_clock_messaging_deinit(void) {
  app_message_deregister_callbacks();
}

void world_clock_messaging_request_overlay(uint16_t map_width,
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
