#pragma once

#include <pebble.h>

#define time_traveler_MESSAGING_INBOX_SIZE 768
#define time_traveler_MESSAGING_OUTBOX_SIZE 64

typedef void (*WorldClockMessageCityDataCallback)(const uint8_t *blob,
                                                  uint16_t length,
                                                  int8_t user_city_index,
                                                  void *context);

void time_traveler_messaging_init(WorldClockMessageCityDataCallback on_city_data_received,
                                void *context);

void time_traveler_messaging_deinit(void);

void time_traveler_messaging_request_city_data(void);
