var timezone = require('./cities');
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

function getDateFormat() {
  try {
    var savedSettings = localStorage.getItem('timeTravelerSettings');
    if (savedSettings) {
      var settings = JSON.parse(savedSettings);
      if (settings.SETTING_DATE_FORMAT !== undefined) {
        return parseInt(settings.SETTING_DATE_FORMAT, 10) || 0;
      }
    }
  } catch (e) {}
  return 0;
}

function getCustomCities() {
  try {
    var savedSettings = localStorage.getItem('timeTravelerSettings');
    if (savedSettings) {
      var settings = JSON.parse(savedSettings);
      if (settings.SETTING_CUSTOM_CITIES) {
        return JSON.parse(settings.SETTING_CUSTOM_CITIES);
      }
    }
  } catch (e) {
    console.log('Error parsing custom cities:', e);
  }
  return [];
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
  var customCities = getCustomCities();

  // Build a flat list of entries (standard + custom), then sort west→east by lon
  var entries = [];

  for (var i = 0; i < CITIES.length; i++) {
    var city = CITIES[i];
    if (!pinnedNames.includes(city.name)) continue;
    entries.push({ type: 'standard', city: city, lon: city.lon });
  }

  for (var j = 0; j < customCities.length; j++) {
    var cc = customCities[j];
    var refCity = null;
    for (var k = 0; k < CITIES.length; k++) {
      if (CITIES[k].name === cc.tzCityName) { refCity = CITIES[k]; break; }
    }
    if (!refCity) continue;
    entries.push({ type: 'custom', cc: cc, refCity: refCity, lon: cc.lon });
  }

  entries.sort(function (a, b) { return a.lon - b.lon; });

  var blob = [];
  for (var e = 0; e < entries.length; e++) {
    var entry = entries[e];

    if (entry.type === 'standard') {
      var sc = entry.city;
      var name = sc.name.substring(0, 15);
      for (var n = 0; n < 16; n++) blob.push(n < name.length ? name.charCodeAt(n) : 0);
      pushInt16BE(blob, Math.round(sc.lat * 100));
      pushInt16BE(blob, Math.round(sc.lon * 100));
      pushInt16BE(blob, timezone.cityOffsetMinutes(sc, now));
      var label = timezone.dayLabelForCity(sc, now);
      blob.push(label < 0 ? FULL_NIGHT_SENTINEL : label);
      blob.push(0); // is_night
    } else {
      var cu = entry.cc;
      var rf = entry.refCity;
      var ccName = cu.displayName.toUpperCase().substring(0, 15);
      for (var cn = 0; cn < 16; cn++) blob.push(cn < ccName.length ? ccName.charCodeAt(cn) : 0);
      pushInt16BE(blob, Math.round(cu.lat * 100));
      pushInt16BE(blob, Math.round(cu.lon * 100));
      pushInt16BE(blob, timezone.cityOffsetMinutes(rf, now));
      var ccLabel = timezone.dayLabelForCity(rf, now);
      blob.push(ccLabel < 0 ? FULL_NIGHT_SENTINEL : ccLabel);
      blob.push(0); // is_night
    }
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

    if (start === 0) {
      dict.SETTING_DATE_FORMAT = getDateFormat();
      if (coords) {
        dict.USER_LAT = Math.round(coords.latitude * 100);
        dict.USER_LON = Math.round(coords.longitude * 100);
      }
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
