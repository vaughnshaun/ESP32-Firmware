#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Ticker.h>

// Example testing sketch for various DHT humidity/temperature sensors written by ladyada
// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include "DHT.h"

#define SENSOR_READ_DELAY 4
#define SENSOR_REQUEST_DELAY 5
#define DHTPIN 4     // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
// #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

// The router name and password
const char *ssid_Router     = "RouterName"; //Enter the router name
const char *password_Router = "Password"; //Enter the router password

// Set the hostname for the ESP32
const char* hostname = "DesiredHostName";  // You can choose any name you prefer

// Create WebServer instance on port 80
WebServer  server(80);

// A timer for reading the sensor
Ticker sensorTicker;

// Temperature Globals
float temperature = 0;
float humidity = 0;
  // Compute heat index in Fahrenheit (the default)
float heatIndex = 0;

void setup() {
  //Serial.begin(9600);
  Serial.begin(115200); // Should always match the upload speed
  Serial.println(F("DHTxx test!"));

  setupWiFi();
  setupHostname();
  setupWebServer();

  dht.begin();

  // Read the initial data
  readSensorData();

  // Initialize the sensor ticker to call readSensorData every SENSOR_READ_DELAY seconds 
  sensorTicker.attach(SENSOR_READ_DELAY, readSensorData);
}

void setupWiFi() {
  Serial.println("WiFi Setup start");
  WiFi.begin(ssid_Router, password_Router);
  Serial.println(String("Connecting to ")+ssid_Router);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected, IP address: ");
  Serial.println(WiFi.localIP()); // The local ip assigned by the router
  Serial.println("Wifi Setup End");
}

void setupHostname() {
  // Start mDNS with the desired hostname
  if (MDNS.begin(hostname)) {
    Serial.println("mDNS started successfully");
  } else {
    Serial.println("mDNS failed to start");
  }
}

void setupWebServer() {
  // Start the server
  server.begin();
  Serial.println("HTTP server started");

  // Define server routes
  server.on("/", handleWebServerRoot);       // Handle root URL
  
  // Endpoint for reading sensor data
  server.on("/read_sensor", HTTP_GET, handleGetWeatherData);
}

void handleWebServerRoot() {
    // Serve the HTML page
String htmlPage = "<html><body>"
                  "<h1>ESP32 DHT Sensor (Refreshed every " + String(SENSOR_REQUEST_DELAY) + " seconds)</h1>"
                  "<p>Temperature: <span id='temperature'></span> F</p>"
                  "<p>Humidity: <span id='humidity'></span> %</p>"
                  "<p>Heat Index: <span id='heatIndex'></span> F</p>"
                  "<p>Feels: <span id='feelsLike'></span> </p>"
                  "<script>"
                  "function fetchData() {"
                  "  fetch('/read_sensor').then(response => response.json()).then(data => {"
                  "    document.getElementById('temperature').innerText = data.temperature;"
                  "    document.getElementById('humidity').innerText = data.humidity;"
                  "    document.getElementById('heatIndex').innerText = data.heatIndex;"
                  "    document.getElementById('feelsLike').innerText = data.feelsLike;"
                  "  });"
                  "}"
                  "setInterval(fetchData, " + String(SENSOR_REQUEST_DELAY * 1000) + ");"  // Poll every SENSOR_REQUEST_DELAY seconds
                  "fetchData();"  // Initial fetch on page load
                  "</script>"
                  "</body></html>";

  // Send the HTML page to the client
  server.send(200, "text/html", htmlPage);
}

void readSensorData() {

  float newTemperature = dht.readTemperature(true); // isFahrenheit = true
  float newHumidity = dht.readHumidity();
   // Compute heat index in Fahrenheit (the default)
  float newHeatIndex = dht.computeHeatIndex(temperature, humidity);

  // Check if reading the sensor failed
  if (isnan(newTemperature) || isnan(newHumidity) || isnan(newHeatIndex)) {
    return;
  }

  // Update the weather parameters only if the read succeeds
  temperature = newTemperature;
  humidity = newHumidity;
   // Compute heat index in Fahrenheit (the default)
  heatIndex = newHeatIndex;
}

void handleGetWeatherData() {
  // Create a JSON response string
  String response = "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + ", \"heatIndex\": " + String(heatIndex)  + ", \"feelsLike\": \"" + categorizeWeather(temperature, heatIndex) + "\"}";
  server.send(200, "application/json", response);
}

String categorizeWeather(float temperature, float heatIndex) {
  // Cold weather categories based on actual temperature
  if (temperature < 20) {
    return "Extremely Cold";
  } else if (temperature >= 20 && temperature < 32) {
    return "Very Cold";
  } else if (temperature >= 32 && temperature < 50) {
    return "Cold";
  }

  // Heat index categories based on heat index (Fahrenheit)
  if (heatIndex < 82) {
    return "Warm";              // 82°F and below
  } else if (heatIndex >= 82 && heatIndex < 100) {
    return "Hot";               // 82°F - 99°F
  } else if (heatIndex >= 100 && heatIndex < 106) {
    return "Very Hot";          // 100°F - 105°F
  } else if (heatIndex >= 106 && heatIndex < 115) {
    return "Dangerous";         // 106°F - 114°F
  } else {
    return "Extremely Dangerous"; // 115°F and above
  }
}

void loop() {
  // Handle incoming client requests
  server.handleClient();
}