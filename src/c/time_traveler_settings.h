#pragma once

#include <pebble.h>

#define SETTINGS_VERSION 1
#define SETTINGS_PERSIST_KEY 1

// Maximum number of pinned cities
#define MAX_PINNED_CITIES 50

// Settings structure
typedef struct {
  char pinned_cities[MAX_PINNED_CITIES][32]; // Array of city names
  int num_pinned_cities;
} TimeTravelerSettings;

// Global settings instance
extern TimeTravelerSettings global_settings;

// Function declarations
void time_traveler_settings_init();
void time_traveler_settings_deinit();
void time_traveler_settings_load();
void time_traveler_settings_save();
bool time_traveler_settings_is_city_pinned(const char *city_name);
int time_traveler_settings_get_pinned_city_index(const char *city_name);
void time_traveler_settings_set_pinned_indices(const uint8_t *indices, int count);