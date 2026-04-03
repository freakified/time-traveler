var OVERLAY_CHUNK_ROWS = 6;
var FULL_DAY_START = 0;
var FULL_NIGHT_SENTINEL = 255;

var lastRequestedSize = null;
var lastOverlayVersion = 0;
var minuteTimer = null;

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
  if (value < 0) {
    return 0;
  }
  if (value >= width) {
    return width - 1;
  }
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
    rows.push(interval[0] + "," + interval[1]);
  }

  return rows;
}

function nextOverlayVersion() {
  lastOverlayVersion = (lastOverlayVersion + 1) % 2147483647;
  if (lastOverlayVersion === 0) {
    lastOverlayVersion = 1;
  }
  return lastOverlayVersion;
}

function sendDictionary(dictionary, success, failure) {
  Pebble.sendAppMessage(dictionary, success, function(error) {
    console.log("sendAppMessage failed", JSON.stringify(error));
    if (failure) {
      failure(error);
    }
  });
}

function sendOverlayForSize(width, height) {
  if (!width || !height) {
    return;
  }

  var version = nextOverlayVersion();
  var rows = computeOverlayRows(width, height, new Date());
  var chunks = [];

  for (var start = 0; start < rows.length; start += OVERLAY_CHUNK_ROWS) {
    var chunkRows = rows.slice(start, start + OVERLAY_CHUNK_ROWS);
    chunks.push({
      overlay_map_width: width,
      overlay_map_height: height,
      overlay_version: version,
      overlay_total_rows: rows.length,
      overlay_row_start: start,
      overlay_row_count: chunkRows.length,
      overlay_rows: chunkRows.join(";")
    });
  }

  function sendChunk(index) {
    if (index >= chunks.length) {
      return;
    }

    sendDictionary(chunks[index], function() {
      sendChunk(index + 1);
    });
  }

  sendChunk(0);
}

function requestDrivenOverlayUpdate() {
  if (!lastRequestedSize) {
    return;
  }
  sendOverlayForSize(lastRequestedSize.width, lastRequestedSize.height);
}

function startMinuteUpdates() {
  if (minuteTimer) {
    clearInterval(minuteTimer);
  }

  minuteTimer = setInterval(requestDrivenOverlayUpdate, 60000);
}

Pebble.addEventListener("ready", function() {
  console.log("World Clock JS ready");
  startMinuteUpdates();
});

Pebble.addEventListener("appmessage", function(event) {
  var payload = event.payload || {};
  if (!payload.request_overlay) {
    return;
  }

  var width = payload.overlay_map_width;
  var height = payload.overlay_map_height;
  if (!width || !height) {
    return;
  }

  lastRequestedSize = {
    width: width,
    height: height
  };

  sendOverlayForSize(width, height);
});
