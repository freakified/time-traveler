var CITIES = [
  { name: "PAGO PAGO",        tz: "Pacific/Pago_Pago",    lat: -14.27, lon: -170.70, std: -660 },
  { name: "HONOLULU",         tz: "Pacific/Honolulu",     lat: 21.31,  lon: -157.86, std: -600 },
  { name: "ANCHORAGE",        tz: "America/Anchorage",    lat: 61.22,  lon: -149.90, std: -540 },
  { name: "VANCOUVER",        tz: "America/Vancouver",    lat: 49.28,  lon: -123.12, std: -480 },
  { name: "SAN FRANCISCO",    tz: "America/Los_Angeles",  lat: 37.77,  lon: -122.42, std: -480 },
  { name: "EDMONTON",         tz: "America/Edmonton",     lat: 53.54,  lon: -113.49, std: -420 },
  { name: "DENVER",           tz: "America/Denver",       lat: 39.74,  lon: -104.99, std: -420 },
  { name: "MEXICO CITY",      tz: "America/Mexico_City",  lat: 19.43,  lon: -99.13,  std: -360 },
  { name: "CHICAGO",          tz: "America/Chicago",      lat: 41.88,  lon: -87.63,  std: -360 },
  { name: "NEW YORK",         tz: "America/New_York",     lat: 40.71,  lon: -74.01,  std: -300 },
  { name: "SANTIAGO",         tz: "America/Santiago",     lat: -33.45, lon: -70.67,  std: -240 },
  { name: "HALIFAX",          tz: "America/Halifax",      lat: 44.65,  lon: -63.57,  std: -240 },
  { name: "ST. JOHNS",        tz: "America/St_Johns",     lat: 47.56,  lon: -52.71,  std: -210 },
  { name: "RIO DE JANEIRO",   tz: "America/Sao_Paulo",    lat: -22.91, lon: -43.17,  std: -180 },
  { name: "F. DE NORONHA",    tz: "America/Noronha",      lat: -3.86,  lon: -32.42,  std: -120 },
  { name: "PRAIA",            tz: "Atlantic/Cape_Verde",  lat: 14.92,  lon: -23.51,  std: -60 },
  { name: "UTC",              tz: "UTC",                  lat: 0.00,   lon: 0.00,    std: 0 },
  { name: "LISBON",           tz: "Europe/Lisbon",        lat: 38.72,  lon: -9.14,   std: 0 },
  { name: "LONDON",           tz: "Europe/London",        lat: 51.51,  lon: -0.13,   std: 0 },
  { name: "MADRID",           tz: "Europe/Madrid",        lat: 40.42,  lon: -3.70,   std: 60 },
  { name: "PARIS",            tz: "Europe/Paris",         lat: 48.86,  lon: 2.35,    std: 60 },
  { name: "ROME",             tz: "Europe/Rome",          lat: 41.90,  lon: 12.50,   std: 60 },
  { name: "BERLIN",           tz: "Europe/Berlin",        lat: 52.52,  lon: 13.41,   std: 60 },
  { name: "STOCKHOLM",        tz: "Europe/Stockholm",     lat: 59.33,  lon: 18.07,   std: 60 },
  { name: "ATHENS",           tz: "Europe/Athens",        lat: 37.98,  lon: 23.73,   std: 120 },
  { name: "CAIRO",            tz: "Africa/Cairo",         lat: 30.04,  lon: 31.24,   std: 120 },
  { name: "JERUSALEM",        tz: "Asia/Jerusalem",       lat: 31.77,  lon: 35.23,   std: 120 },
  { name: "MOSCOW",           tz: "Europe/Moscow",        lat: 55.76,  lon: 37.62,   std: 180 },
  { name: "JEDDAH",           tz: "Asia/Riyadh",          lat: 21.49,  lon: 39.19,   std: 180 },
  { name: "TEHRAN",           tz: "Asia/Tehran",          lat: 35.69,  lon: 51.39,   std: 210 },
  { name: "DUBAI",            tz: "Asia/Dubai",           lat: 25.20,  lon: 55.27,   std: 240 },
  { name: "KABUL",            tz: "Asia/Kabul",           lat: 34.53,  lon: 69.17,   std: 270 },
  { name: "KARACHI",          tz: "Asia/Karachi",         lat: 24.86,  lon: 67.01,   std: 300 },
  { name: "DELHI",            tz: "Asia/Kolkata",         lat: 28.61,  lon: 77.21,   std: 330 },
  { name: "KATHMANDU",        tz: "Asia/Kathmandu",       lat: 27.72,  lon: 85.32,   std: 345 },
  { name: "DHAKA",            tz: "Asia/Dhaka",           lat: 23.81,  lon: 90.41,   std: 360 },
  { name: "YANGON",           tz: "Asia/Yangon",          lat: 16.87,  lon: 96.20,   std: 390 },
  { name: "BANGKOK",          tz: "Asia/Bangkok",         lat: 13.76,  lon: 100.50,  std: 420 },
  { name: "SINGAPORE",        tz: "Asia/Singapore",       lat: 1.35,   lon: 103.82,  std: 480 },
  { name: "HONG KONG",        tz: "Asia/Hong_Kong",       lat: 22.32,  lon: 114.17,  std: 480 },
  { name: "BEIJING",          tz: "Asia/Shanghai",        lat: 39.90,  lon: 116.40,  std: 480 },
  { name: "TAIPEI",           tz: "Asia/Taipei",          lat: 25.03,  lon: 121.57,  std: 480 },
  { name: "SEOUL",            tz: "Asia/Seoul",           lat: 37.57,  lon: 126.98,  std: 540 },
  { name: "TOKYO",            tz: "Asia/Tokyo",           lat: 35.68,  lon: 139.69,  std: 540 },
  { name: "ADELAIDE",         tz: "Australia/Adelaide",   lat: -34.93, lon: 138.60,  std: 570 },
  { name: "GUAM",             tz: "Pacific/Guam",         lat: 13.44,  lon: 144.79,  std: 600 },
  { name: "SYDNEY",           tz: "Australia/Sydney",     lat: -33.87, lon: 151.21,  std: 600 },
  { name: "NOUMEA",           tz: "Pacific/Noumea",       lat: -22.28, lon: 166.46,  std: 660 },
  { name: "WELLINGTON",       tz: "Pacific/Auckland",     lat: -41.29, lon: 174.78,  std: 720 }
];

// ---- Real IANA timezone data via Intl ----
// The phone apps (JavaScriptCore on iOS, a WebView on Android) expose the OS's
// tz database through Intl. pypkjs's V8 has an Intl object but no ICU locale
// data, and constructing any DateTimeFormat there is a fatal, uncatchable
// crash. supportedLocalesOf is safe to call, so probe with it first. See
// "Timezones in the emulator" in README.md for the upstream status/workaround.

var intlAvailable = null;
var intlFormatters = {};

function intlFormatter(tz) {
  if (!intlFormatters[tz]) {
    intlFormatters[tz] = new Intl.DateTimeFormat('en-US', {
      timeZone: tz,
      hour12: false,
      year: 'numeric', month: 'numeric', day: 'numeric',
      hour: 'numeric', minute: 'numeric'
    });
  }
  return intlFormatters[tz];
}

function intlOffsetMinutes(tz, now) {
  var p = {};
  intlFormatter(tz).formatToParts(now).forEach(function (part) {
    p[part.type] = part.value;
  });
  // Some engines render midnight as "24" with hour12:false.
  var wallAsUtc = Date.UTC(+p.year, +p.month - 1, +p.day, +p.hour % 24, +p.minute);
  var subMinuteMs = now.getUTCSeconds() * 1000 + now.getUTCMilliseconds();
  return Math.round((wallAsUtc - (now.getTime() - subMinuteMs)) / 60000);
}

function hasIntlTimezones() {
  if (intlAvailable !== null) return intlAvailable;

  intlAvailable = false;
  try {
    if (typeof Intl !== 'undefined' && Intl.DateTimeFormat &&
        Intl.DateTimeFormat.supportedLocalesOf(['en-US']).length > 0) {
      // Sanity check against a known DST zone before trusting the engine.
      intlAvailable =
        intlOffsetMinutes('America/New_York', new Date(Date.UTC(2026, 0, 15, 12))) === -300 &&
        intlOffsetMinutes('America/New_York', new Date(Date.UTC(2026, 6, 15, 12))) === -240;
    }
  } catch (e) {
    console.log('Intl timezone probe failed: ' + e);
  }
  console.log('Timezone source: ' + (intlAvailable ? 'Intl' : 'standard offsets (no DST)'));
  return intlAvailable;
}

function cityOffsetMinutes(city, now) {
  if (hasIntlTimezones()) {
    try {
      return intlOffsetMinutes(city.tz, now);
    } catch (e) {
      console.log('Intl offset failed for ' + city.tz + ': ' + e);
    }
  }
  // No usable Intl (only the emulator so far): standard time, ignoring DST.
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
