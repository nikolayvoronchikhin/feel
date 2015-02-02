var xhrRequest = function(url,type,callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    callback(this.responseText);
  };
  xhr.open(type,url);
  xhr.send();
};

function locationSuccess(pos) {
  // construct url
  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" + pos.coords.latitude + "&lon=" + pos.coords.longitude;
  // send request
  xhrRequest(url, 'GET',
    function(responseText) {
      var json = JSON.parse(responseText);
      var temperature = Math.round(json.main.temp - 273.15);
      console.log("Temperature " + temperature);
      var humidity = json.main.humidity;
      console.log("Humidity " + humidity);
      var dewpoint = (function() {
        var b, c;
        if (temperature >= 0 && temperature <= 50) {
          b = 17.966; c = 247.15;
        }
        if (temperature >= -40 && temperature <= 0) {
          b = 17.368; c = 238.88;
        }
        return (c*(Math.log(humidity/100)+((b*temperature)/(c+temperature))))/(b-(Math.log(humidity/100)+((b*temperature)/(c+temperature))));
      })();
      console.log("Dew-point " + dewpoint);
      var dictionary = {
        "KEY_TEMPERATURE": temperature,
        "KEY_DEWPOINT": dewpoint
      };
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather information sent success.");
        },
        function(e) {
          console.log("Weather information failed to send");
        }
      );
    }
  );
}

function locationError(err) {
  console.log("Error retrieving location");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    { timeout: 15000, maximumAge: 60000 }
  );
}

// listen for opened watchface
Pebble.addEventListener('ready',
  function(e) {
    console.log("PebbleKit JS ready");
    getWeather(); // init weather
  }
);

// listen for received appmessage
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received");
    getWeather(); // weather updates
  }
);