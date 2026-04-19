var cityData = require('./city-data');
var timezone = require('./cities');

var configDataUri = 'https://freakified.github.io/time-traveler/';

var dstCheckTimer = null;
var locationAvailable = null;

function startDstChecks() {
  if (dstCheckTimer) clearInterval(dstCheckTimer);
  dstCheckTimer = setInterval(function () {
    cityData.sendCityData(null, locationAvailable);
  }, 30 * 60 * 1000);
}

Pebble.addEventListener("ready", function () {
  startDstChecks();

  if (navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(function (pos) {
      locationAvailable = true;
      cityData.sendCityData(pos.coords, locationAvailable);
    }, function () {
      locationAvailable = false;
      cityData.sendCityData(null, locationAvailable);
    }, {
      enableHighAccuracy: false,
      timeout: 5000,
      maximumAge: 600000
    });
  } else {
    locationAvailable = false;
    cityData.sendCityData(null, locationAvailable);
  }
});

Pebble.addEventListener("appmessage", function (event) {
  var payload = event.payload || {};

  if (payload.REQUEST_CITY_DATA) {
    cityData.forceResend();
    cityData.sendCityData(null, locationAvailable);
    return;
  }
});

// ---- Configuration ----

Pebble.addEventListener('showConfiguration', function () {
  var url = configDataUri;

  var watchInfo = Pebble.getActiveWatchInfo();
  url += (url.indexOf('?') === -1 ? '?' : '&') + 'watchInfo=' + encodeURIComponent(JSON.stringify({
    platform: watchInfo.platform,
    model: watchInfo.model,
    language: watchInfo.language,
    firmware: {
      major: watchInfo.firmware.major,
      minor: watchInfo.firmware.minor
    }
  }));

  var persistedSettings = localStorage.getItem('timeTravelerSettings');
  if (persistedSettings) {
    try {
      var settings = JSON.parse(persistedSettings);
      url += '&settings=' + encodeURIComponent(JSON.stringify(settings));
    } catch (e) {
      console.log('Error loading persisted settings:', e);
    }
  }

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e.response || e.response === 'CANCELLED' || e.response === 'null' || e.response === '{}') {
    return;
  }

  var configData;
  try {
    configData = JSON.parse(decodeURIComponent(e.response));
  } catch (err) {
    console.log('Error parsing configuration response: ' + err);
    try {
      configData = JSON.parse(e.response);
    } catch (err2) {
      console.log('Failed to parse config data even without decoding');
      return;
    }
  }

  if (configData.return_to) {
    delete configData.return_to;
  }

  // Save to localStorage for persistence
  localStorage.setItem('timeTravelerSettings', JSON.stringify(configData));

  // Clear cached city list and send fresh data to watch
  cityData.clearCachedPinnedCities();
  cityData.forceResend();
  cityData.sendCityData(null, locationAvailable);
});
