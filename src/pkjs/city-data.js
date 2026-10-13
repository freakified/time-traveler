var timezone = require('./cities');
var FULL_NIGHT_SENTINEL = require('./constants').FULL_NIGHT_SENTINEL;

var lastSentCityData = null;
var lastSentLocationAvailable = null;
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

// The watch renders names as UTF-8, and Pebble draws nothing for a string with
// invalid UTF-8, so names can't be written one byte per character (ö is 0xF6 in
// UTF-16 but C3 B6 in UTF-8). Truncates on a character boundary.
function utf8Bytes(str, maxBytes) {
  var bytes = [];
  for (var i = 0; i < str.length; i++) {
    var c = str.charCodeAt(i);
    if (c >= 0xD800 && c < 0xDC00 && i + 1 < str.length) {
      c = 0x10000 + ((c - 0xD800) << 10) + (str.charCodeAt(++i) - 0xDC00);
    }
    var ch;
    if (c < 0x80) ch = [c];
    else if (c < 0x800) ch = [0xC0 | (c >> 6), 0x80 | (c & 0x3F)];
    else if (c < 0x10000) ch = [0xE0 | (c >> 12), 0x80 | ((c >> 6) & 0x3F), 0x80 | (c & 0x3F)];
    else ch = [0xF0 | (c >> 18), 0x80 | ((c >> 12) & 0x3F), 0x80 | ((c >> 6) & 0x3F), 0x80 | (c & 0x3F)];
    if (bytes.length + ch.length > maxBytes) break;
    bytes.push.apply(bytes, ch);
  }
  return bytes;
}

// New blob format: 24 bytes per city
//   bytes 0-15:  city name as UTF-8, null-terminated (up to 15 bytes)
//   bytes 16-17: latitude  × 100 as int16, big-endian
//   bytes 18-19: longitude × 100 as int16, big-endian
//   bytes 20-21: offset_minutes as int16, big-endian
//   byte  22:    day_label (0=today, 1=tomorrow, 255=yesterday)
//   byte  23:    is_night (0 or 1)
// ~50km threshold: 0.45 degrees squared ≈ 0.2025
var PROXIMITY_THRESHOLD_SQ = 0.2025;

function findNearestEntryByCoords(entries, lat, lon) {
  var bestIdx = -1;
  var bestDist = PROXIMITY_THRESHOLD_SQ;

  for (var i = 0; i < entries.length; i++) {
    var dlat = entries[i].lat - lat;
    var dlon = entries[i].lon - lon;
    var dist = dlat * dlat + dlon * dlon;
    if (dist < bestDist) {
      bestDist = dist;
      bestIdx = i;
    }
  }
  return bestIdx;
}

function computeCityDataBlob(now, coords) {
  var CITIES = timezone.CITIES;
  var pinnedNames = getPinnedCities();
  var customCities = getCustomCities();

  // Build a flat list of entries (standard + custom), then sort west→east by lon.
  // `zone` is anything cityOffsetMinutes accepts: { tz, std }.
  var entries = [];

  for (var i = 0; i < CITIES.length; i++) {
    var city = CITIES[i];
    if (!pinnedNames.includes(city.name)) continue;
    entries.push({ name: city.name, lat: city.lat, lon: city.lon, zone: city });
  }

  for (var j = 0; j < customCities.length; j++) {
    var cc = customCities[j];
    var zone = null;
    if (cc.tz) {
      zone = { tz: cc.tz, std: cc.std || 0 };
    } else {
      // Saved before custom cities had their own IANA zone
      for (var k = 0; k < CITIES.length; k++) {
        if (CITIES[k].name === cc.tzCityName) { zone = CITIES[k]; break; }
      }
    }
    if (!zone) continue;
    entries.push({ name: cc.displayName.toUpperCase(), lat: cc.lat, lon: cc.lon, zone: zone });
  }

  entries.sort(function (a, b) { return a.lon - b.lon; });

  // After sorting, determine proximity match
  var matchedCityIndex = -1;
  if (coords) {
    matchedCityIndex = findNearestEntryByCoords(entries, coords.latitude, coords.longitude);
  }

  var blob = [];
  for (var e = 0; e < entries.length; e++) {
    var entry = entries[e];
    var name = utf8Bytes(entry.name, 15);
    for (var n = 0; n < 16; n++) blob.push(n < name.length ? name[n] : 0);
    pushInt16BE(blob, Math.round(entry.lat * 100));
    pushInt16BE(blob, Math.round(entry.lon * 100));
    pushInt16BE(blob, timezone.cityOffsetMinutes(entry.zone, now));
    var label = timezone.dayLabelForCity(entry.zone, now);
    blob.push(label < 0 ? FULL_NIGHT_SENTINEL : label);
    blob.push(0); // is_night
  }

  return { blob: blob, matchedCityIndex: matchedCityIndex };
}

function cityDataBlobsEqual(a, b) {
  if (!a || !b || a.length !== b.length) return false;
  for (var i = 0; i < a.length; i++) {
    if (a[i] !== b[i]) return false;
  }
  return true;
}

function sendCityData(coords, locationAvailable) {
  var now = new Date();
  var result = computeCityDataBlob(now, coords);
  var blob = result.blob;
  var matchedCityIndex = result.matchedCityIndex;

  var locationAvailableChanged = (locationAvailable !== null && locationAvailable !== undefined
                                  && locationAvailable !== lastSentLocationAvailable);
  if (cityDataBlobsEqual(blob, lastSentCityData) && !coords && !locationAvailableChanged) {
    return;
  }
  lastSentCityData = blob;
  if (locationAvailable !== null && locationAvailable !== undefined) {
    lastSentLocationAvailable = locationAvailable;
  }

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
      dict.USER_UTC_OFFSET_MINUTES = -now.getTimezoneOffset();
      if (coords) {
        dict.USER_LAT = Math.round(coords.latitude * 100);
        dict.USER_LON = Math.round(coords.longitude * 100);
        dict.USER_MATCHED_CITY_INDEX = matchedCityIndex;
      }
      if (locationAvailable === false) {
        dict.LOCATION_AVAILABLE = 0;
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
