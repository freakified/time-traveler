var timezone = require('./timezone');
var FULL_NIGHT_SENTINEL = require('./constants').FULL_NIGHT_SENTINEL;

var lastSentCityData = null;
var cachedPinnedCities = null;

function sendDictionary(dictionary, success, failure) {
  Pebble.sendAppMessage(dictionary, success, function (error) {
    console.log("sendAppMessage failed", JSON.stringify(error));
    if (failure) {
      failure(error);
    }
  });
}

function getPinnedCities() {
  if (cachedPinnedCities) return cachedPinnedCities;

  try {
    var savedSettings = localStorage.getItem('timeTravelerSettings');
    if (savedSettings) {
      var settings = JSON.parse(savedSettings);
      if (settings.SETTING_PINNED_CITIES) {
        cachedPinnedCities = JSON.parse(settings.SETTING_PINNED_CITIES);
        console.log('Using pinned cities from settings:', cachedPinnedCities);
        return cachedPinnedCities;
      }
    }
  } catch (e) {
    console.log('Error parsing pinned cities:', e);
  }

  // Default to some fun cities around the world if no setting found
  return [
    "HONOLULU", "ANCHORAGE", "SAN FRANCISCO", "DENVER", "CHICAGO", "NEW YORK",
    "ST. JOHNS", "RIO DE JANEIRO", "LONDON", "BERLIN", "CAIRO", "MOSCOW",
    "DUBAI", "DELHI", "KATHMANDU", "BANGKOK", "BEIJING", "TOKYO", "SYDNEY",
    "WELLINGTON"
  ];
}

// Encode a signed 16-bit value as two big-endian bytes, appending to blob array
function pushInt16BE(blob, value) {
  var v = value & 0xFFFF;
  blob.push((v >> 8) & 0xFF);
  blob.push(v & 0xFF);
}

// New blob format: 24 bytes per city
//   bytes 0-15:  city name, null-terminated (up to 15 chars)
//   bytes 16-17: latitude  × 100 as int16, big-endian
//   bytes 18-19: longitude × 100 as int16, big-endian
//   bytes 20-21: offset_minutes as int16, big-endian
//   byte  22:    day_label (0=today, 1=tomorrow, 255=yesterday)
//   byte  23:    is_night (0 or 1)
function computeCityDataBlob(now) {
  var CITIES = timezone.CITIES;
  var pinnedNames = getPinnedCities();
  var blob = [];

  for (var i = 0; i < CITIES.length; i++) {
    var city = CITIES[i];
    if (!pinnedNames.includes(city.name)) continue;

    // Name: 16 bytes, null-padded
    var name = city.name.substring(0, 15);
    for (var c = 0; c < 16; c++) {
      blob.push(c < name.length ? name.charCodeAt(c) : 0);
    }

    // lat × 100, lon × 100 as int16 big-endian
    pushInt16BE(blob, Math.round(city.lat * 100));
    pushInt16BE(blob, Math.round(city.lon * 100));

    // offset_minutes as int16 big-endian
    var offset = timezone.cityOffsetMinutes(city, now);
    pushInt16BE(blob, offset);

    // day_label and is_night
    var label = timezone.dayLabelForCity(city, now);
    blob.push(label < 0 ? FULL_NIGHT_SENTINEL : label);
    blob.push(0); // is_night
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

function sendCityData(coords) {
  var now = new Date();
  var blob = computeCityDataBlob(now);

  if (cityDataBlobsEqual(blob, lastSentCityData) && !coords) {
    return;
  }
  lastSentCityData = blob;

  var CHUNK_SIZE = 120; // 5 cities × 24 bytes
  var chunks = [];
  for (var start = 0; start < blob.length; start += CHUNK_SIZE) {
    var chunk = blob.slice(start, start + CHUNK_SIZE);
    var dict = {
      CITY_DATA_START: start,
      CITY_DATA_COUNT: chunk.length,
      CITY_DATA_TOTAL: blob.length,
      CITY_DATA: chunk
    };

    if (start === 0 && coords) {
      dict.USER_LAT = Math.round(coords.latitude * 100);
      dict.USER_LON = Math.round(coords.longitude * 100);
    }

    chunks.push(dict);
  }

  function sendChunk(index) {
    if (index >= chunks.length) return;
    sendDictionary(chunks[index], function () {
      sendChunk(index + 1);
    });
  }
  sendChunk(0);
}

function forceResend() {
  lastSentCityData = null;
}

function clearCachedPinnedCities() {
  cachedPinnedCities = null;
}

module.exports = {
  sendCityData: sendCityData,
  forceResend: forceResend,
  clearCachedPinnedCities: clearCachedPinnedCities,
};
