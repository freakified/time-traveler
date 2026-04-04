#pragma once

// City coordinates for map display
typedef struct {
    float longitude;
    float latitude;
} CityCoordinates;

CityCoordinates *time_traveller_get_city_coordinates(int city_index);