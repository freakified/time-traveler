#include "time_traveler_settings.h"
#include "time_traveler_data.h"
#include <string.h>

// Global settings instance
TimeTravelerSettings global_settings;

// Default pinned cities
static const char *DEFAULT_PINNED_CITIES[] = {
  "SAN FRANCISCO",
  "NEW YORK",
  "LONDON",
  "PARIS",
  "TOKYO",
  "SYDNEY"
};

static const int DEFAULT_PINNED_CITIES_COUNT = sizeof(DEFAULT_PINNED_CITIES) / sizeof(DEFAULT_PINNED_CITIES[0]);

void time_traveler_settings_init() {
  // Initialize with default values
  global_settings.num_pinned_cities = 0;
  
  // Load from persistent storage
  time_traveler_settings_load();
  
  // If no settings loaded, use defaults
  if (global_settings.num_pinned_cities == 0) {
    for (int i = 0; i < DEFAULT_PINNED_CITIES_COUNT; i++) {
      if (i < MAX_PINNED_CITIES) {
        strncpy(global_settings.pinned_cities[i], DEFAULT_PINNED_CITIES[i], sizeof(global_settings.pinned_cities[i]) - 1);
        global_settings.pinned_cities[i][sizeof(global_settings.pinned_cities[i]) - 1] = '\0';
      }
    }
    global_settings.num_pinned_cities = DEFAULT_PINNED_CITIES_COUNT;
    time_traveler_settings_save();
  }
}

void time_traveler_settings_deinit() {
  // Nothing to do for now
}

void time_traveler_settings_load() {
  // Try to read from persistent storage
  int result = persist_read_data(SETTINGS_PERSIST_KEY, &global_settings, sizeof(TimeTravelerSettings));
  
  if (result == sizeof(TimeTravelerSettings)) {
    // Data loaded successfully
    APP_LOG(APP_LOG_LEVEL_INFO, "Loaded settings from storage");
  } else {
    // No settings found or error loading
    global_settings.num_pinned_cities = 0;
  }
}

void time_traveler_settings_save() {
  // Save to persistent storage
  persist_write_data(SETTINGS_PERSIST_KEY, &global_settings, sizeof(TimeTravelerSettings));
  APP_LOG(APP_LOG_LEVEL_INFO, "Saved settings to storage");
}

bool time_traveler_settings_is_city_pinned(const char *city_name) {
  for (int i = 0; i < global_settings.num_pinned_cities; i++) {
    if (strcmp(global_settings.pinned_cities[i], city_name) == 0) {
      return true;
    }
  }
  return false;
}

int time_traveler_settings_get_pinned_city_index(const char *city_name) {
  for (int i = 0; i < global_settings.num_pinned_cities; i++) {
    if (strcmp(global_settings.pinned_cities[i], city_name) == 0) {
      return i;
    }
  }
  return -1;
}