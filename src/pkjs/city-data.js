var timezone = require('./timezone');
var FULL_NIGHT_SENTINEL = require('./constants').FULL_NIGHT_SENTINEL;

var lastSentCityData = null;
var cachedPinnedCities = null;

function sendDictionary(dictionary, success, failure) {
  Pebble.sendAppMessage(dictionary, success, function(error) {
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
  
  // Default to sensible world cities if no setting found
  return [
    "HONOLULU", "ANCHORAGE", "SAN FRANCISCO", "DENVER", "CHICAGO", "NEW YORK",
    "ST. JOHNS", "RIO DE JANEIRO", "LONDON", "BERLIN", "CAIRO", "MOSCOW",
    "DUBAI", "DELHI", "KATHMANDU", "BANGKOK", "BEIJING", "TOKYO", "SYDNEY",
    "WELLINGTON"
  ];
  }


function getPinnedCityIndices() {
  var pinnedNames = getPinnedCities();
  var indices = [];
  var CITIES = timezone.CITIES;
  for (var i = 0; i < CITIES.length; i++) {
    if (pinnedNames.includes(CITIES[i].name)) {
      indices.push(i);
    }
  }
  return indices;
}

function computeCityDataBlob(now) {
  var CITIES = timezone.CITIES;
  var pinnedIndices = getPinnedCityIndices();
  var blob = [];
  
  // Only include pinned fixed cities
  for (var i = 0; i < pinnedIndices.length; i++) {
    var cityIdx = pinnedIndices[i];
    var city = CITIES[cityIdx];
    var offset = timezone.cityOffsetMinutes(city, now);
    var label = timezone.dayLabelForCity(city, now);
    var labelByte = label < 0 ? FULL_NIGHT_SENTINEL : label;
    var isNight = 0;

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

function sendCityData(coords) {
  var now = new Date();
  var blob = computeCityDataBlob(now);

  if (cityDataBlobsEqual(blob, lastSentCityData) && !coords) {
    return;
  }
  lastSentCityData = blob;

  var CHUNK_SIZE = 24;
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
      // Send coords as integers multiplied by 100 to preserve precision in int32
      dict.USER_LAT = Math.round(coords.latitude * 100);
      dict.USER_LON = Math.round(coords.longitude * 100);
    }

    chunks.push(dict);
  }

  function sendChunk(index) {
    if (index >= chunks.length) return;
    sendDictionary(chunks[index], function() {
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
  getPinnedCityIndices: getPinnedCityIndices
};
