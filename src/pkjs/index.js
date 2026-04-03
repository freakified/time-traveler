var OVERLAY_CHUNK_ROWS = 6;
var FULL_DAY_START = 0;
var FULL_NIGHT_SENTINEL = 255;

var lastRequestedSize = null;
var lastOverlayVersion = 0;
var minuteTimer = null;
var lastSentRows = null;
var lastSentCityData = null;
var dstCheckTimer = null;

// ============================================================
// City data: names, coordinates, timezone rules
// ============================================================
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
  { name: "SANTIAGO",          lat: -33.45, lon: -70.67,  std: -240,  dst: { off: 60,  sm: 9, sw: 1, sd: 6, em: 4,  ew: 1, ed: 6 } },
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
  { name: "CAIRO",             lat: 30.04,  lon: 31.24,   std: 120,   dst: null },
  { name: "JERUSALEM",         lat: 31.77,  lon: 35.23,   std: 120,   dst: { off: 60,  sm: 3, sw: 0, sd: 5, em: 10, ew: 1, ed: 0 } },
  { name: "MOSCOW",            lat: 55.76,  lon: 37.62,   std: 180,   dst: null },
  { name: "JEDDAH",            lat: 21.49,  lon: 39.19,   std: 180,   dst: null },
  { name: "TEHRAN",            lat: 35.69,  lon: 51.39,   std: 210,   dst: { off: 60,  sm: 3, sd: 21, em: 9, ed: 21 } },
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

var NUM_CITIES = CITIES.length;

// ============================================================
// DST calculation
// ============================================================
function nthWeekdayOfMonth(year, month, n, dayOfWeek) {
  if (n === 0) {
    // last occurrence: find first of next month, go back
    var nextMonth = new Date(Date.UTC(year, month, 1));
    // go to last day of current month
    nextMonth.setUTCDate(0);
    var lastDay = nextMonth.getUTCDate();
    var lastDow = nextMonth.getUTCDay();
    var diff = lastDow - dayOfWeek;
    if (diff < 0) diff += 7;
    return lastDay - diff;
  }
  // nth occurrence
  var first = new Date(Date.UTC(year, month - 1, 1));
  var firstDow = first.getUTCDay();
  var diff = dayOfWeek - firstDow;
  if (diff < 0) diff += 7;
  return 1 + diff + (n - 1) * 7;
}

function isDstActive(city, now) {
  var rule = city.dst;
  if (!rule) return false;

  var year = now.getUTCFullYear();

  var startDay, endDay;
  if (rule.sd !== undefined && rule.sw === undefined) {
    // fixed day (Tehran)
    startDay = new Date(Date.UTC(year, rule.sm - 1, rule.sd));
    endDay = new Date(Date.UTC(year, rule.em - 1, rule.ed));
  } else {
    startDay = new Date(Date.UTC(year, rule.sm - 1, nthWeekdayOfMonth(year, rule.sm, rule.sw, rule.sd)));
    endDay = new Date(Date.UTC(year, rule.em - 1, nthWeekdayOfMonth(year, rule.em, rule.ew, rule.ed)));
  }

  return now >= startDay && now < endDay;
}

function cityOffsetMinutes(city, now) {
  if (isDstActive(city, now)) {
    return city.std + city.dst.off;
  }
  return city.std;
}

// ============================================================
// Day label: calendar-aware yesterday/today/tomorrow
// ============================================================
function dayLabelForCity(city, now) {
  // offsetDiff = city's UTC offset minus user's UTC offset (in minutes)
  // now.getTimezoneOffset() returns minutes behind UTC (negative if ahead)
  var offsetDiff = cityOffsetMinutes(city, now) + now.getTimezoneOffset();
  var cityTotalMinutes = now.getHours() * 60 + now.getMinutes() + offsetDiff;
  var dayDiff = Math.floor(cityTotalMinutes / 1440);

  if (dayDiff <= -1) return -1;
  if (dayDiff >= 1) return 1;
  return 0;
}

// ============================================================
// User timezone auto-detection
// ============================================================
function detectUserCityIndex(now) {
  // Skip Intl - crashes in some JS environments (emulator, older JSC)
  // Fallback: match by current UTC offset
  var userOffsetMin = -now.getTimezoneOffset();
  var bestIdx = 16; // UTC as default
  var bestDiff = Math.abs(userOffsetMin);

  for (var i = 0; i < NUM_CITIES; i++) {
    var diff = Math.abs(userOffsetMin - CITIES[i].std);
    if (diff < bestDiff) {
      bestDiff = diff;
      bestIdx = i;
    }
  }
  return bestIdx;
}

function tzNameToCityIndex(tz) {
  var map = {
    "Pacific/Pago_Pago": 0,
    "Pacific/Honolulu": 1,
    "America/Anchorage": 2,
    "America/Vancouver": 3,
    "America/Los_Angeles": 4,
    "America/Edmonton": 5,
    "America/Denver": 6,
    "America/Mexico_City": 7,
    "America/Chicago": 8,
    "America/New_York": 9,
    "America/Santiago": 10,
    "America/Halifax": 11,
    "America/St_Johns": 12,
    "America/Sao_Paulo": 13,
    "America/Noronha": 14,
    "Atlantic/Cape_Verde": 15,
    "Etc/UTC": 16, "UTC": 16,
    "Europe/Lisbon": 17,
    "Europe/London": 18,
    "Europe/Madrid": 19,
    "Europe/Paris": 20,
    "Europe/Rome": 21,
    "Europe/Berlin": 22,
    "Europe/Stockholm": 23,
    "Europe/Athens": 24,
    "Africa/Cairo": 25,
    "Asia/Jerusalem": 26,
    "Europe/Moscow": 27,
    "Asia/Riyadh": 28,
    "Asia/Tehran": 29,
    "Asia/Dubai": 30,
    "Asia/Kabul": 31,
    "Asia/Karachi": 32,
    "Asia/Kolkata": 33,
    "Asia/Kathmandu": 34,
    "Asia/Dhaka": 35,
    "Asia/Yangon": 36,
    "Asia/Bangkok": 37,
    "Asia/Singapore": 38,
    "Asia/Hong_Kong": 39,
    "Asia/Shanghai": 40,
    "Asia/Taipei": 41,
    "Asia/Seoul": 42,
    "Asia/Tokyo": 43,
    "Australia/Adelaide": 44,
    "Pacific/Guam": 45,
    "Australia/Sydney": 46,
    "Pacific/Noumea": 47,
    "Pacific/Auckland": 48
  };
  if (map[tz] !== undefined) return map[tz];
  return -1;
}

// ============================================================
// Binary blob: 4 bytes per city
//   bytes 0-1: offset_minutes (int16, big-endian)
//   byte 2:    day_label (-1, 0, 1) stored as uint8 (255, 0, 1)
//   byte 3:    is_night (0 or 1)
// ============================================================
function computeCityDataBlob(now) {
  var blob = [];
  for (var i = 0; i < NUM_CITIES; i++) {
    var city = CITIES[i];
    var offset = cityOffsetMinutes(city, now);
    var label = dayLabelForCity(city, now);
    var labelByte = label < 0 ? 255 : label; // -1 -> 255
    var isNight = 0; // TODO: compute from solar position when needed

    blob.push((offset >> 8) & 0xFF);
    blob.push(offset & 0xFF);
    blob.push(labelByte);
    blob.push(isNight);
  }
  return blob;
}

function cityDataBlobsEqual(a, b) {
  if (!a || !b || a.length !== b.length) return false;
  for (var i = 0; i < a.length; i++) {
    if (a[i] !== b[i]) return false;
  }
  return true;
}

// ============================================================
// AppMessage communication
// ============================================================
function sendDictionary(dictionary, success, failure) {
  Pebble.sendAppMessage(dictionary, success, function(error) {
    console.log("sendAppMessage failed", JSON.stringify(error));
    if (failure) {
      failure(error);
    }
  });
}

function sendCityData() {
  var now = new Date();
  var blob = computeCityDataBlob(now);
  var userIdx = detectUserCityIndex(now);

  if (cityDataBlobsEqual(blob, lastSentCityData)) {
    return;
  }
  lastSentCityData = blob;

  var CHUNK_SIZE = 24;
  var chunks = [];
  for (var start = 0; start < blob.length; start += CHUNK_SIZE) {
    var chunk = blob.slice(start, start + CHUNK_SIZE);
    chunks.push({
      city_data_start: start,
      city_data_count: chunk.length,
      city_data_total: blob.length,
      city_data: chunk,
      user_city_index: (start === 0 ? userIdx : -1)
    });
  }

  function sendChunk(index) {
    if (index >= chunks.length) return;
    sendDictionary(chunks[index], function() {
      sendChunk(index + 1);
    });
  }
  sendChunk(0);
}

// ============================================================
// Solar position / daylight overlay (existing logic)
// ============================================================
function dayOfYearUtc(date) {
  var start = Date.UTC(date.getUTCFullYear(), 0, 0);
  var current = Date.UTC(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate());
  return Math.floor((current - start) / 86400000);
}

function normalizeLongitudeDegrees(longitude) {
  var wrapped = ((longitude + 180) % 360 + 360) % 360 - 180;
  return wrapped === -180 ? 180 : wrapped;
}

function clampPixel(value, width) {
  if (value < 0) return 0;
  if (value >= width) return width - 1;
  return value;
}

function solarPosition(date) {
  var day = dayOfYearUtc(date);
  var utcHours = date.getUTCHours();
  var utcMinutes = date.getUTCMinutes();
  var utcSeconds = date.getUTCSeconds();
  var fractionalHour = utcHours + utcMinutes / 60 + utcSeconds / 3600;
  var gamma = (2 * Math.PI / 365) * (day - 1 + (fractionalHour - 12) / 24);

  var declination =
      0.006918 -
      0.399912 * Math.cos(gamma) +
      0.070257 * Math.sin(gamma) -
      0.006758 * Math.cos(2 * gamma) +
      0.000907 * Math.sin(2 * gamma) -
      0.002697 * Math.cos(3 * gamma) +
      0.00148 * Math.sin(3 * gamma);

  var equationOfTime =
      229.18 *
      (0.000075 +
       0.001868 * Math.cos(gamma) -
       0.032077 * Math.sin(gamma) -
       0.014615 * Math.cos(2 * gamma) -
       0.040849 * Math.sin(2 * gamma));

  var utcMinutesTotal = utcHours * 60 + utcMinutes + utcSeconds / 60;
  var subsolarLongitude = normalizeLongitudeDegrees(
      (720 - utcMinutesTotal - equationOfTime) / 4);

  return {
    declination: declination,
    subsolarLongitude: subsolarLongitude
  };
}

function longitudeToPixel(longitude, width) {
  var normalized = ((longitude + 180) % 360 + 360) % 360;
  return clampPixel(Math.round(normalized / 360 * (width - 1)), width);
}

function daylightIntervalForLatitude(latitudeDegrees, solar, width) {
  var latitude = latitudeDegrees * Math.PI / 180;
  var tanProduct = Math.tan(latitude) * Math.tan(solar.declination);

  if (tanProduct >= 1) {
    return [FULL_NIGHT_SENTINEL, FULL_NIGHT_SENTINEL];
  }

  if (tanProduct <= -1) {
    return [FULL_DAY_START, width - 1];
  }

  var hourAngle = Math.acos(-tanProduct) * 180 / Math.PI;
  var startLongitude = normalizeLongitudeDegrees(solar.subsolarLongitude - hourAngle);
  var endLongitude = normalizeLongitudeDegrees(solar.subsolarLongitude + hourAngle);

  return [
    longitudeToPixel(startLongitude, width),
    longitudeToPixel(endLongitude, width)
  ];
}

function computeOverlayRows(width, height, date) {
  var solar = solarPosition(date);
  var rows = [];

  for (var row = 0; row < height; row += 1) {
    var latitude = 90 - ((row + 0.5) / height) * 180;
    var interval = daylightIntervalForLatitude(latitude, solar, width);
    rows.push(interval);
  }

  return rows;
}

function rowsAreIdentical(a, b) {
  if (!a || !b || a.length !== b.length) return false;
  for (var i = 0; i < a.length; i++) {
    if (a[i][0] !== b[i][0] || a[i][1] !== b[i][1]) return false;
  }
  return true;
}

function rowsToBinary(rows) {
  var data = [];
  for (var i = 0; i < rows.length; i++) {
    data.push(rows[i][0]);
    data.push(rows[i][1]);
  }
  return data;
}

function nextOverlayVersion() {
  lastOverlayVersion = (lastOverlayVersion + 1) % 2147483647;
  if (lastOverlayVersion === 0) lastOverlayVersion = 1;
  return lastOverlayVersion;
}

function sendOverlayForSize(width, height) {
  if (!width || !height) return;

  var rows = computeOverlayRows(width, height, new Date());

  if (rowsAreIdentical(rows, lastSentRows)) return;
  lastSentRows = rows;

  var version = nextOverlayVersion();
  var chunks = [];

  for (var start = 0; start < rows.length; start += OVERLAY_CHUNK_ROWS) {
    var chunkRows = rows.slice(start, start + OVERLAY_CHUNK_ROWS);
    var binaryData = rowsToBinary(chunkRows);
    chunks.push({
      overlay_map_width: width,
      overlay_map_height: height,
      overlay_version: version,
      overlay_total_rows: rows.length,
      overlay_row_start: start,
      overlay_row_count: chunkRows.length,
      overlay_row_data: binaryData
    });
  }

  function sendChunk(index) {
    if (index >= chunks.length) return;
    sendDictionary(chunks[index], function() {
      sendChunk(index + 1);
    });
  }
  sendChunk(0);
}

function requestDrivenOverlayUpdate() {
  if (!lastRequestedSize) return;
  sendOverlayForSize(lastRequestedSize.width, lastRequestedSize.height);
}

function startMinuteUpdates() {
  if (minuteTimer) clearInterval(minuteTimer);
  minuteTimer = setInterval(requestDrivenOverlayUpdate, 60000);
}

function startDstChecks() {
  if (dstCheckTimer) clearInterval(dstCheckTimer);
  dstCheckTimer = setInterval(function() {
    sendCityData();
  }, 30 * 60 * 1000); // every 30 minutes
}

// ============================================================
// Event handlers
// ============================================================
Pebble.addEventListener("ready", function() {
  startMinuteUpdates();
  startDstChecks();
  sendCityData();
});

Pebble.addEventListener("appmessage", function(event) {
  var payload = event.payload || {};

  // Handle overlay request
  if (payload.request_overlay) {
    var width = payload.overlay_map_width;
    var height = payload.overlay_map_height;
    if (!width || !height) return;

    lastRequestedSize = { width: width, height: height };
    lastSentRows = null;
    sendOverlayForSize(width, height);
    return;
  }

  // Handle city data request
  if (payload.request_city_data) {
    lastSentCityData = null; // force resend
    sendCityData();
    return;
  }
});
