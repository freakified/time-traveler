#include "time_traveler_solar_math.h"

// ACOS_LOOKUP maps input [-TRIG_MAX_RATIO, TRIG_MAX_RATIO] to angle [0, TRIG_MAX_ANGLE/2]
// Array size is 256
static const uint16_t ACOS_LOOKUP[256] = {
    32768,31461,30918,30501,30148,29837,29555,29296,29053,28826,28609,28404,28206,28017,27834,27658,
    27486,27320,27158,27001,26847,26696,26549,26405,26263,26125,25988,25854,25722,25592,25465,25338,
    25214,25092,24970,24851,24733,24616,24500,24386,24273,24161,24050,23941,23832,23724,23617,23511,
    23406,23302,23199,23096,22994,22893,22792,22693,22594,22495,22397,22300,22203,22107,22011,21916,
    21822,21728,21634,21541,21448,21356,21264,21173,21081,20991,20900,20811,20721,20632,20543,20454,
    20366,20278,20190,20103,20016,19929,19842,19756,19669,19584,19498,19412,19327,19242,19157,19072,
    18988,18904,18819,18735,18652,18568,18484,18401,18318,18234,18151,18068,17986,17903,17820,17738,
    17655,17573,17490,17408,17326,17244,17162,17080,16998,16916,16834,16752,16670,16589,16507,16425,
    16343,16261,16179,16098,16016,15934,15852,15770,15688,15606,15524,15442,15360,15278,15195,15113,
    15030,14948,14865,14782,14700,14617,14534,14450,14367,14284,14200,14116,14033,13949,13864,13780,
    13696,13611,13526,13441,13356,13270,13184,13099,13012,12926,12839,12752,12665,12578,12490,12402,
    12314,12225,12136,12047,11957,11868,11777,11687,11595,11504,11412,11320,11227,11134,11040,10946,
    10852,10757,10661,10565,10468,10371,10273,10174,10075,9976,9875,9774,9672,9569,9466,9362,
    9257,9151,9044,8936,8827,8718,8607,8495,8382,8268,8152,8035,7917,7798,7676,7554,
    7430,7303,7176,7046,6914,6780,6643,6505,6363,6219,6072,5921,5767,5610,5448,5282,
    5110,4934,4751,4562,4364,4159,3942,3715,3472,3213,2931,2620,2267,1850,1307,0,
};

int32_t fixed_acos(int32_t x) {
    if (x <= -TRIG_MAX_RATIO) return ACOS_LOOKUP[0];
    if (x >= TRIG_MAX_RATIO) return ACOS_LOOKUP[255];
    
    // Map -65536..65536 to 0..255
    int32_t index = (x + TRIG_MAX_RATIO) * 255 / (TRIG_MAX_RATIO * 2);
    if (index < 0) index = 0;
    if (index > 255) index = 255;
    return ACOS_LOOKUP[index];
}

int32_t fixed_tanProduct(int32_t lat_angle, int32_t decl_angle) {
    int32_t sin_lat = sin_lookup(lat_angle);
    int32_t cos_lat = cos_lookup(lat_angle);
    int32_t sin_decl = sin_lookup(decl_angle);
    int32_t cos_decl = cos_lookup(decl_angle);
    
    if (cos_lat == 0 || cos_decl == 0) {
        return ((sin_lat > 0) == (sin_decl > 0)) ? TRIG_MAX_RATIO * 10 : -TRIG_MAX_RATIO * 10;
    }
    
    int64_t num = (int64_t)sin_lat * (int64_t)sin_decl;
    int64_t den = (int64_t)cos_lat * (int64_t)cos_decl;
    
    int64_t result = (num * TRIG_MAX_RATIO) / den;
    
    // Clamp to prevent overflow when assigning to int32
    if (result > TRIG_MAX_RATIO * 100) return TRIG_MAX_RATIO * 100;
    if (result < -TRIG_MAX_RATIO * 100) return -TRIG_MAX_RATIO * 100;
    return (int32_t)result;
}

SolarPosition time_traveler_solar_position(time_t current_time) {
  struct tm *t = gmtime(&current_time);
  
  int32_t elapsed_seconds = t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec;
  
  // gamma angle representing time of year
  // (2 * Math.PI / 365) * (day - 1 + (fractionalHour - 12) / 24)
  // tm_yday is day-1.
  int32_t gamma = (t->tm_yday * TRIG_MAX_ANGLE) / 365;
  gamma += ((elapsed_seconds - 43200) * TRIG_MAX_ANGLE) / (365 * 86400);

  // declination in radians, translated to TRIG_MAX_ANGLE units
  // constants scaled by (32768 / PI)
  int32_t declination = 72
    - (4171 * cos_lookup(gamma)) / TRIG_MAX_RATIO
    + (733 * sin_lookup(gamma)) / TRIG_MAX_RATIO
    - (70 * cos_lookup(2 * gamma)) / TRIG_MAX_RATIO
    + (9 * sin_lookup(2 * gamma)) / TRIG_MAX_RATIO
    - (28 * cos_lookup(3 * gamma)) / TRIG_MAX_RATIO
    + (15 * sin_lookup(3 * gamma)) / TRIG_MAX_RATIO;

  // equation of time base, scaled by 1,000,000
  int32_t eq_base = 75
    + (1868 * cos_lookup(gamma)) / TRIG_MAX_RATIO
    - (32077 * sin_lookup(gamma)) / TRIG_MAX_RATIO
    - (14615 * cos_lookup(2 * gamma)) / TRIG_MAX_RATIO
    - (40849 * sin_lookup(2 * gamma)) / TRIG_MAX_RATIO;
    
  // equation of time in minutes: 229.18 * eq_base / 1,000,000
  int32_t eq_time_minutes = (229 * eq_base) / 1000000;
  
  // subsolar longitude natively mapped correctly matching normalized 0..360 range
  int32_t total_minutes = (elapsed_seconds / 60) + eq_time_minutes;
  int32_t subsolar = - (total_minutes * TRIG_MAX_ANGLE) / 1440;
  
  while (subsolar < 0) subsolar += TRIG_MAX_ANGLE;
  while (subsolar >= TRIG_MAX_ANGLE) subsolar -= TRIG_MAX_ANGLE;

  SolarPosition pos;
  pos.declination = declination;
  pos.subsolar_longitude = subsolar;
  return pos;
}

static uint8_t longitude_to_pixel(int32_t longitude, uint16_t width) {
  while (longitude < 0) longitude += TRIG_MAX_ANGLE;
  while (longitude >= TRIG_MAX_ANGLE) longitude -= TRIG_MAX_ANGLE;
  
  // longitude / 65536 * (width - 1)
  int32_t px = (longitude * (width - 1)) / TRIG_MAX_ANGLE;
  if (px > 254) px = 254;
  return (uint8_t)px;
}

void time_traveler_daylight_interval(int32_t latitude, SolarPosition solar, uint16_t width, uint8_t *start_out, uint8_t *end_out) {
  int32_t tan_product = fixed_tanProduct(latitude, solar.declination);
  
  // tan(lat)*tan(decl) >= 1: same hemisphere, sun never sets → polar day
  if (tan_product >= TRIG_MAX_RATIO) {
    *start_out = 0;
    *end_out = (uint8_t)((width - 1 > 254) ? 254 : width - 1);
    return;
  }

  // tan(lat)*tan(decl) <= -1: opposite hemispheres, sun never rises → polar night
  if (tan_product <= -TRIG_MAX_RATIO) {
    *start_out = 255;
    *end_out = 255;
    return;
  }
  
  // hourAngle in TRIG units from fixed_acos
  int32_t hour_angle = fixed_acos(-tan_product);

  // When hour_angle == TRIG_MAX_ANGLE/2 (180°), start_lon and end_lon both
  // collapse to the anti-subsolar meridian, so start_px == end_px. The acos
  // lookup table has limited resolution near this boundary, so rows just inside
  // the full-day threshold get index 0 and produce an erroneous full-night row.
  // Treat them as full-day to match the tan_product <= -TRIG_MAX_RATIO path above.
  if (hour_angle >= TRIG_MAX_ANGLE / 2) {
    *start_out = 0;
    *end_out = (uint8_t)((width - 1 > 254) ? 254 : width - 1);
    return;
  }

  // The hourAngle spans from 0 to TRIG_MAX_ANGLE/2
  int32_t start_lon = solar.subsolar_longitude - hour_angle;
  int32_t end_lon = solar.subsolar_longitude + hour_angle;
  
  *start_out = longitude_to_pixel(start_lon, width);
  *end_out = longitude_to_pixel(end_lon, width);
}
