#pragma once

// City coordinates for map display
typedef struct {
    float longitude;
    float latitude;
} CityCoordinates;

CityCoordinates *world_clock_get_city_coordinates(int city_index);