#pragma once

#include <pebble.h>

#define time_traveller_MESSAGING_INBOX_SIZE 768
#define time_traveller_MESSAGING_OUTBOX_SIZE 64

typedef void (*WorldClockMessageOverlayCallback)(uint16_t map_width,
                                                 uint16_t map_height,
                                                 uint32_t version,
                                                 uint16_t total_rows,
                                                 uint16_t row_start,
                                                 uint16_t row_count,
                                                 const uint8_t *row_data,
                                                 uint16_t row_data_len,
                                                 void *context);

typedef void (*WorldClockMessageCityDataCallback)(const uint8_t *blob,
                                                  uint16_t length,
                                                  int8_t user_city_index,
                                                  void *context);

void time_traveller_messaging_init(WorldClockMessageOverlayCallback on_overlay_received,
                                WorldClockMessageCityDataCallback on_city_data_received,
                                void *context);

void time_traveller_messaging_deinit(void);

void time_traveller_messaging_request_overlay(uint16_t map_width,
                                           uint16_t map_height);

void time_traveller_messaging_request_city_data(void);
