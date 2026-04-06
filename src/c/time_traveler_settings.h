#pragma once

#include <pebble.h>

#define SETTINGS_VERSION 2
#define SETTINGS_PERSIST_KEY 2

// Settings structure (reserved for future use)
typedef struct {
  uint8_t reserved;
} TimeTravelerSettings;

// Global settings instance
extern TimeTravelerSettings global_settings;

// Function declarations
void time_traveler_settings_init(void);
void time_traveler_settings_deinit(void);
