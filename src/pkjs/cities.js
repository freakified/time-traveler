var CITIES = [
  { name: "PAGO PAGO",         lat: -14.27, lon: -170.70, std: -660,  dst: null },
  { name: "HONOLULU",          lat: 21.31,  lon: -157.86, std: -600,  dst: null },
  { name: "ANCHORAGE",         lat: 61.22,  lon: -149.90, std: -540,  dst: { off: 60,  sm: 3, sw: 2, sd: 0, em: 11, ew: 1, ed: 0 } },
  { name: "VANCOUVER",         lat: 49.28,  lon: -123.12, std: -480,  dst: { off: 60,  sm: 3, sw: 2, sd: 0, em: 11, ew: 1, ed: 0 } },
  { name: "SAN FRANCISCO",     lat: 37.77,  lon: -122.42, std: -480,  dst: { off: 60,  sm: 3, sw: 2, sd: 0, em: 11, ew: 1, ed: 0 } },
  { name: "EDMONTON",          lat: 53.54,  lon: -113.49, std: -420,  dst: { off: 60,  sm: 3, sw: 2, sd: 0, em: 11, ew: 1, ed: 0 } },
  { name: "DENVER",            lat: 39.74,  lon: -104.99, std: -420,  dst: { off: 60,  sm: 3, sw: 2, sd: 0, em: 11, ew: 1, ed: 0 } },
  { name: "MEXICO CITY",       lat: 19.43,  lon: -99.13,  std: -360,  dst: null },
  { name: "CHICAGO",           lat: 41.88,  lon: -87.63,  std: -360,  dst: { off: 60,  sm: 3, sw: 2, sd: 0, em: 11, ew: 1, ed: 0 } },
  { name: "NEW YORK",          lat: 40.71,  lon: -74.01,  std: -300,  dst: { off: 60,  sm: 3, sw: 2, sd: 0, em: 11, ew: 1, ed: 0 } },
  { name: "SANTIAGO",          lat: -33.45, lon: -70.67,  std: -240,  dst: { off: 60,  sm: 9, sn: 2, sd: 0, em: 4,  en: 2, ed: 0 } },
  { name: "HALIFAX",           lat: 44.65,  lon: -63.57,  std: -240,  dst: { off: 60,  sm: 3, sw: 2, sd: 0, em: 11, ew: 1, ed: 0 } },
  { name: "ST. JOHNS",         lat: 47.56,  lon: -52.71,  std: -210,  dst: { off: 60,  sm: 3, sw: 2, sd: 0, em: 11, ew: 1, ed: 0 } },
  { name: "RIO DE JANEIRO",    lat: -22.91, lon: -43.17,  std: -180,  dst: null },
  { name: "F. DE NORONHA",     lat: -3.86,  lon: -32.42,  std: -120,  dst: null },
  { name: "PRAIA",             lat: 14.92,  lon: -23.51,  std: -60,   dst: null },
  { name: "UTC",               lat: 0.00,   lon: 0.00,    std: 0,     dst: null },
  { name: "LISBON",            lat: 38.72,  lon: -9.14,   std: 0,     dst: { off: 60,  sm: 3, sw: 0, sd: 0, em: 10, ew: 0, ed: 0 } },
  { name: "LONDON",            lat: 51.51,  lon: -0.13,   std: 0,     dst: { off: 60,  sm: 3, sw: 0, sd: 0, em: 10, ew: 0, ed: 0 } },
  { name: "MADRID",            lat: 40.42,  lon: -3.70,   std: 60,    dst: { off: 60,  sm: 3, sw: 0, sd: 0, em: 10, ew: 0, ed: 0 } },
  { name: "PARIS",             lat: 48.86,  lon: 2.35,    std: 60,    dst: { off: 60,  sm: 3, sw: 0, sd: 0, em: 10, ew: 0, ed: 0 } },
  { name: "ROME",              lat: 41.90,  lon: 12.50,   std: 60,    dst: { off: 60,  sm: 3, sw: 0, sd: 0, em: 10, ew: 0, ed: 0 } },
  { name: "BERLIN",            lat: 52.52,  lon: 13.41,   std: 60,    dst: { off: 60,  sm: 3, sw: 0, sd: 0, em: 10, ew: 0, ed: 0 } },
  { name: "STOCKHOLM",         lat: 59.33,  lon: 18.07,   std: 60,    dst: { off: 60,  sm: 3, sw: 0, sd: 0, em: 10, ew: 0, ed: 0 } },
  { name: "ATHENS",            lat: 37.98,  lon: 23.73,   std: 120,   dst: { off: 60,  sm: 3, sw: 0, sd: 0, em: 10, ew: 0, ed: 0 } },
  { name: "CAIRO",             lat: 30.04,  lon: 31.24,   std: 120,   dst: { off: 60,  sm: 4, sw: 0, sd: 5, em: 10, ew: 0, ed: 4, eo: 1 } },
  { name: "JERUSALEM",         lat: 31.77,  lon: 35.23,   std: 120,   dst: { off: 60,  sm: 3, sn: 23, sd: 5, em: 10, ew: 0, ed: 0 } },
  { name: "MOSCOW",            lat: 55.76,  lon: 37.62,   std: 180,   dst: null },
  { name: "JEDDAH",            lat: 21.49,  lon: 39.19,   std: 180,   dst: null },
  { name: "TEHRAN",            lat: 35.69,  lon: 51.39,   std: 210,   dst: null },
  { name: "DUBAI",             lat: 25.20,  lon: 55.27,   std: 240,   dst: null },
  { name: "KABUL",             lat: 34.53,  lon: 69.17,   std: 270,   dst: null },
  { name: "KARACHI",           lat: 24.86,  lon: 67.01,   std: 300,   dst: null },
  { name: "DELHI",             lat: 28.61,  lon: 77.21,   std: 330,   dst: null },
  { name: "KATHMANDU",         lat: 27.72,  lon: 85.32,   std: 345,   dst: null },
  { name: "DHAKA",             lat: 23.81,  lon: 90.41,   std: 360,   dst: null },
  { name: "YANGON",            lat: 16.87,  lon: 96.20,   std: 390,   dst: null },
  { name: "BANGKOK",           lat: 13.76,  lon: 100.50,  std: 420,   dst: null },
  { name: "SINGAPORE",         lat: 1.35,   lon: 103.82,  std: 480,   dst: null },
  { name: "HONG KONG",         lat: 22.32,  lon: 114.17,  std: 480,   dst: null },
  { name: "BEIJING",           lat: 39.90,  lon: 116.40,  std: 480,   dst: null },
  { name: "TAIPEI",            lat: 25.03,  lon: 121.57,  std: 480,   dst: null },
  { name: "SEOUL",             lat: 37.57,  lon: 126.98,  std: 540,   dst: null },
  { name: "TOKYO",             lat: 35.68,  lon: 139.69,  std: 540,   dst: null },
  { name: "ADELAIDE",          lat: -34.93, lon: 138.60,  std: 570,   dst: { off: 60,  sm: 10, sw: 1, sd: 0, em: 4, ew: 1, ed: 0 } },
  { name: "GUAM",              lat: 13.44,  lon: 144.79,  std: 600,   dst: null },
  { name: "SYDNEY",            lat: -33.87, lon: 151.21,  std: 600,   dst: { off: 60,  sm: 10, sw: 1, sd: 0, em: 4, ew: 1, ed: 0 } },
  { name: "NOUMEA",            lat: -22.28, lon: 166.46,  std: 660,   dst: null },
  { name: "WELLINGTON",        lat: -41.29, lon: 174.78,  std: 720,   dst: { off: 60,  sm: 9, sw: 0, sd: 0, em: 4, ew: 1, ed: 0 } }
];

function nthWeekdayOfMonth(year, month, n, dayOfWeek) {
  if (n === 0) {
    var nextMonth = new Date(Date.UTC(year, month, 1));
    nextMonth.setUTCDate(0);
    var lastDay = nextMonth.getUTCDate();
    var lastDow = nextMonth.getUTCDay();
    var diff = lastDow - dayOfWeek;
    if (diff < 0) diff += 7;
    return lastDay - diff;
  }
  var first = new Date(Date.UTC(year, month - 1, 1));
  var firstDow = first.getUTCDay();
  var diff = dayOfWeek - firstDow;
  if (diff < 0) diff += 7;
  return 1 + diff + (n - 1) * 7;
}

// First `dayOfWeek` falling on or after `dom` (tzdata's "Sun>=2" form).
function weekdayOnOrAfter(year, month, dom, dayOfWeek) {
  var d = new Date(Date.UTC(year, month - 1, dom));
  var diff = dayOfWeek - d.getUTCDay();
  if (diff < 0) diff += 7;
  return dom + diff;
}

// One edge of a DST rule. `w` picks the nth weekday of the month (0 = last),
// `n` instead picks the first such weekday on or after that day of the month,
// and `o` shifts the result by whole days (Egypt ends at 24:00, i.e. the next
// day). Months are 1-based, weekdays 0 = Sunday.
function transitionDate(year, month, w, dayOfWeek, n, o) {
  var dom = (n !== undefined)
    ? weekdayOnOrAfter(year, month, n, dayOfWeek)
    : nthWeekdayOfMonth(year, month, w, dayOfWeek);
  return new Date(Date.UTC(year, month - 1, dom + (o || 0)));
}

function isDstActive(city, now) {
  var rule = city.dst;
  if (!rule) return false;

  var year = now.getUTCFullYear();
  var startDay = transitionDate(year, rule.sm, rule.sw, rule.sd, rule.sn, rule.so);
  var endDay = transitionDate(year, rule.em, rule.ew, rule.ed, rule.en, rule.eo);

  // Southern hemisphere rules straddle the new year (e.g. Oct -> Apr), so the
  // active window wraps around instead of being a plain range.
  if (startDay > endDay) return now >= startDay || now < endDay;
  return now >= startDay && now < endDay;
}

function cityOffsetMinutes(city, now) {
  if (isDstActive(city, now)) {
    return city.std + city.dst.off;
  }
  return city.std;
}

function dayLabelForCity(city, now) {
  var offsetDiff = cityOffsetMinutes(city, now) + now.getTimezoneOffset();
  var cityTotalMinutes = now.getHours() * 60 + now.getMinutes() + offsetDiff;
  var dayDiff = Math.floor(cityTotalMinutes / 1440);

  if (dayDiff <= -1) return -1;
  if (dayDiff >= 1) return 1;
  return 0;
}

function findNearestCityByCoords(lat, lon) {
  var bestIdx = 16;
  var bestDist = 999999;

  for (var i = 0; i < CITIES.length; i++) {
    var dlat = CITIES[i].lat - lat;
    var dlon = CITIES[i].lon - lon;
    var dist = dlat * dlat + dlon * dlon;
    if (dist < bestDist) {
      bestDist = dist;
      bestIdx = i;
    }
  }
  return bestIdx;
}

function detectUserCityIndex(now) {
  var userOffsetMin = -now.getTimezoneOffset();
  var bestIdx = 16;
  var bestDiff = Math.abs(userOffsetMin);

  for (var i = 0; i < CITIES.length; i++) {
    var diff = Math.abs(userOffsetMin - cityOffsetMinutes(CITIES[i], now));
    if (diff < bestDiff) {
      bestDiff = diff;
      bestIdx = i;
    }
  }
  return bestIdx;
}

function detectUserCityIndexGeolocation(callback) {
  if (typeof navigator === "undefined" || !navigator.geolocation) {
    callback(-1);
    return;
  }

  var done = false;
  var timer = setTimeout(function() {
    if (!done) {
      done = true;
      callback(-1);
    }
  }, 3000);

  navigator.geolocation.getCurrentPosition(function(pos) {
    if (!done) {
      done = true;
      clearTimeout(timer);
      var idx = findNearestCityByCoords(pos.coords.latitude, pos.coords.longitude);
      callback(idx);
    }
  }, function() {
    if (!done) {
      done = true;
      clearTimeout(timer);
      callback(-1);
    }
  }, {
    enableHighAccuracy: false,
    timeout: 5000,
    maximumAge: 600000
  });
}

module.exports = {
  CITIES: CITIES,
  cityOffsetMinutes: cityOffsetMinutes,
  dayLabelForCity: dayLabelForCity,
  detectUserCityIndex: detectUserCityIndex,
  detectUserCityIndexGeolocation: detectUserCityIndexGeolocation
};
