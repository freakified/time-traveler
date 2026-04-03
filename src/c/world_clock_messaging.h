#pragma once

#include <pebble.h>

#define WORLD_CLOCK_MESSAGING_INBOX_SIZE 768
#define WORLD_CLOCK_MESSAGING_OUTBOX_SIZE 64

typedef void (*WorldClockMessageOverlayCallback)(uint16_t map_width,
                                                 uint16_t map_height,
                                                 uint32_t version,
                                                 uint16_t total_rows,
                                                 uint16_t row_start,
                                                 uint16_t row_count,
                                                 const uint8_t *row_data,
                                                 uint16_t row_data_len,
                                                 void *context);

void world_clock_messaging_init(WorldClockMessageOverlayCallback on_overlay_received,
                                void *context);

void world_clock_messaging_deinit(void);

void world_clock_messaging_request_overlay(uint16_t map_width,
                                           uint16_t map_height);
