var cityData = require('./city-data');
var timezone = require('./timezone');

var dstCheckTimer = null;

function startDstChecks() {
  if (dstCheckTimer) clearInterval(dstCheckTimer);
  dstCheckTimer = setInterval(function() {
    cityData.sendCityData();
  }, 30 * 60 * 1000);
}

Pebble.addEventListener("ready", function() {
  startDstChecks();
  timezone.detectUserCityIndexGeolocation(function(idx) {
    cityData.sendCityData(idx);
  });
});

Pebble.addEventListener("appmessage", function(event) {
  var payload = event.payload || {};



  if (payload.request_city_data) {
    cityData.forceResend();
    cityData.sendCityData();
    return;
  }
});
