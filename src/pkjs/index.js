var cityData = require('./city-data');
var timezone = require('./timezone');

var USE_LOCAL_CONFIG = true;
var configDataUri = 'https://halcyon.freakified.net/';
var configLocalUri = 'http://10.25.219.9:3000/index.html';

var dstCheckTimer = null;
var cachedSettings = null;

function startDstChecks() {
  if (dstCheckTimer) clearInterval(dstCheckTimer);
  dstCheckTimer = setInterval(function () {
    cityData.sendCityData();
  }, 30 * 60 * 1000);
}

function sendSettingsToWatch() {
  if (!cachedSettings) {
    console.log('No settings cached yet');
    return;
  }

  var dict = {};

  // Send pinned cities setting as a byte array of indices
  if (cachedSettings.SETTING_PINNED_CITIES) {
    var indices = cityData.getPinnedCityIndices();
    console.log('Pinned city indices for watch: ' + JSON.stringify(indices));
    dict['SETTING_PINNED_CITIES'] = indices;
    
    // Also try to send current coords if available
    if (navigator.geolocation) {
      navigator.geolocation.getCurrentPosition(function(pos) {
        dict.USER_LAT = Math.round(pos.coords.latitude * 100);
        dict.USER_LON = Math.round(pos.coords.longitude * 100);
        console.log('Sending settings with coords: ' + JSON.stringify(dict));
        Pebble.sendAppMessage(dict);
      }, function() {
        console.log('Sending settings without coords: ' + JSON.stringify(dict));
        Pebble.sendAppMessage(dict);
      }, { timeout: 2000, maximumAge: 600000 });
      return; // Handled in callback
    }
  }

  console.log('Sending settings to watch: ' + JSON.stringify(dict));

  Pebble.sendAppMessage(dict,
    function () { console.log('Settings sent to watch successfully'); },
    function (e) { console.log('Error sending settings to watch: ' + JSON.stringify(e)); }
  );
}

Pebble.addEventListener("ready", function () {
  startDstChecks();
  
  if (navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(function(pos) {
      cityData.sendCityData(pos.coords);
    }, function() {
      cityData.sendCityData();
    }, {
      enableHighAccuracy: false,
      timeout: 5000,
      maximumAge: 600000
    });
  } else {
    cityData.sendCityData();
  }

  // Restore settings from previous session
  var savedSettings = localStorage.getItem('timeTravelerSettings');
  if (savedSettings) {
    try { cachedSettings = JSON.parse(savedSettings); } catch (e) { }
  }

  // Send any cached settings to watch
  sendSettingsToWatch();
});

Pebble.addEventListener("appmessage", function (event) {
  var payload = event.payload || {};

  if (payload.REQUEST_CITY_DATA) {
    cityData.forceResend();
    cityData.sendCityData();
    return;
  }
});

// ---- Configuration ---- 

Pebble.addEventListener('showConfiguration', function () {
  var url = USE_LOCAL_CONFIG ? configLocalUri : configDataUri;

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

  console.log('Opening Config URL: ' + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function (e) {
  console.log('Configuration window closed');

  if (!e.response || e.response === 'CANCELLED' || e.response === 'null' || e.response === '{}') {
    console.log('No configuration data returned');
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
  cachedSettings = configData;

  // Clear cached pinned cities so they get re-read
  cityData.clearCachedPinnedCities();

  // Convert to proper format and send to watch
  sendSettingsToWatch();
});
