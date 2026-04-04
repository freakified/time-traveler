var constants = require('./constants');
var OVERLAY_CHUNK_ROWS = constants.OVERLAY_CHUNK_ROWS;
var FULL_DAY_START = constants.FULL_DAY_START;
var FULL_NIGHT_SENTINEL = constants.FULL_NIGHT_SENTINEL;

var lastOverlayVersion = 0;
var lastSentRows = null;
var lastRequestedSize = null;
var minuteTimer = null;

function sendDictionary(dictionary, success, failure) {
  Pebble.sendAppMessage(dictionary, success, function(error) {
    console.log("sendAppMessage failed", JSON.stringify(error));
    if (failure) {
      failure(error);
    }
  });
}

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

function requestOverlayUpdate() {
  if (!lastRequestedSize) return;
  sendOverlayForSize(lastRequestedSize.width, lastRequestedSize.height);
}

function startMinuteUpdates() {
  if (minuteTimer) clearInterval(minuteTimer);
  minuteTimer = setInterval(requestOverlayUpdate, 60000);
}

function handleOverlayRequest(width, height) {
  if (!width || !height) return;
  lastRequestedSize = { width: width, height: height };
  lastSentRows = null;
  sendOverlayForSize(width, height);
}

function resetOverlayState() {
  lastSentRows = null;
}

module.exports = {
  startMinuteUpdates: startMinuteUpdates,
  requestOverlayUpdate: requestOverlayUpdate,
  handleOverlayRequest: handleOverlayRequest,
  resetOverlayState: resetOverlayState
};
