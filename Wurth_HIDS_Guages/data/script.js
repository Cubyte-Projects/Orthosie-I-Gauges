
document.fonts && document.fonts.forEach(function(font) {
  font.loaded.then(function() {
      if (font.family.match(/Led/)) {
          document.gauges.forEach(function(gauge) {
              gauge.update();
              gauge.options.renderTo.style.visibility = 'true';
          });
      }
  });
});

// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// Create Temperature Gauge
var gaugeTemp = new RadialGauge({
  renderTo: 'gauge-hids-temperature',
  title: "TEMPERATURE",
  units: "C",
  width: 200,
  height: 200,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueBoxWidth: 30,
  valueInt: 0,
  valueDec: 2,
  value: 0,
  strokeTicks: true,
  colorPlate: "#fff",
  borderShadowWidth: 0,
  borders: false,
  colorNeedle: "#007F80",
  colorNeedleEnd: "#007F80",
  needleType: "arrow",
  needleWidth: 2,
  needleCircleSize: 7,
  colorNeedleCircleOuter: "#007F80",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear",
  minValue: -40,
  maxValue: 130,
  fontValue: "Led",
  majorTicks: [
      "-40",
      "-30",
      "-20",
      "-10",
      "0",
      "10",
      "20",
      "30",
      "40",
      "50",
      "60",
      "70",
      "80",
      "90",
      "100",
      "110",
      "120",
      "130",
  ],
  strokeTicks: true,
  highlights: [
      {
          "from": -40,
          "to": 0,
          "color": "rgba(0, 21, 225, .75)"
      },
      {
          "from": 0,
          "to": 32,
          "color": "#00BF00"
      },
      {
          "from": 32,
          "to": 100,
          "color": "#fc8c00"
      },
      {
          "from": 100,
          "to": 130,
          "color": "rgba(200, 50, 50, .75)"
      }
  ]
}).draw();
  
// Create Humidity Gauge
var gaugeHum = new RadialGauge({
  renderTo: 'gauge-hids-humidity',
  title: "HUMIDITY",
  units: "%",
  width: 200,
  height: 200,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueDec: 2,
  valueInt: 0,
  value: 0,
  strokeTicks: true,
  colorPlate: "#fff",
  borderShadowWidth: 0,
  borders: false,
  colorNeedle: "#007F80",
  colorNeedleEnd: "#007F80",
  needleType: "arrow",
  needleWidth: 2,
  needleCircleSize: 7,
  colorNeedleCircleOuter: "#007F80",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear",
  minValue: 0,
  maxValue: 100,
  fontValue: "Led",
  majorTicks: [
      "0",
      "10",
      "20",
      "30",
      "40",
      "50",
      "60",
      "70",
      "80",
      "90",
      "100",
  ],
  strokeTicks: true,
  highlights: [
       {
          "from": 0,
          "to": 20,
          "color": "#FFBF00"
      },
      {
          "from": 80,
          "to": 100,
          "color": "#FFBF00"
      }
  ]
}).draw();


  
// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);

      var temp         = myObj.temperature;
      var hum          = myObj.humidity;
    
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);

    gaugeTemp.value  = myObj.temperature;
    gaugeHum.value   = myObj.humidity;
  }, false);
}

