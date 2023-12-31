/**
 * Basic Write Example code for InfluxDBClient library for Arduino
 * Data can be immediately seen in a InfluxDB UI: wifi_status measurement
 * Enter WiFi and InfluxDB parameters below
 *
 * Measures signal level of the actually connected WiFi network
 * This example supports only InfluxDB running from unsecure (http://...)
 * For secure (https://...) or Influx Cloud 2 use SecureWrite example
 **/
 #include <FastLED.h>


CRGB leds[1];
unsigned long startTime;

#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>

// WiFi AP SSID
#define WIFI_SSID "Basterma_Briggs_2G"
// WiFi password
#define WIFI_PASSWORD "8184344036"
// InfluxDB  server url. Don't use localhost, always server name or ip address.
// E.g. http://192.168.1.48:8086 (In InfluxDB 2 UI -> Load Data -> Client Libraries), 
#define INFLUXDB_URL " http://192.168.1.48:8086"
// InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "toked-id"
// InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
#define INFLUXDB_ORG "org"
// InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
#define INFLUXDB_BUCKET "bucket"
// InfluxDB v1 database name 
//#define INFLUXDB_DB_NAME "database"

// InfluxDB client instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
// InfluxDB client instance for InfluxDB 1
//InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);

// Data point
Point sensor("wifi_status");

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, D6>(leds, 1);
  leds[0].setRGB(0, 255, 0);
  FastLED.show();

  // Connect WiFi
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    showStat(0, 255, 0, 2, 2, true);
  }
  Serial.println();

  // Set InfluxDB 1 authentication params
  //client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);

  // Add constant tags - only once
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    showStat(252, 0, 189, 2, 2, false);
  }
}

//showStat funtion will allway take 3000ms
void showStat(int g, int r, int b, int s, int n, bool on) {
  startTime = millis();  // Record the start time

  leds[0].setRGB(g, r, b);
  //values int i = 100; i <= 256; i++
  for (int t = n; t > 0; t--) {
    for (int i = 150; i <= 250; i++) {
      int value = pow(2, i / 32.0);
      FastLED.setBrightness(value);
      FastLED.show();
      delay(s);
    }
    //values int i = 256; i >= 100; i--
    for (int i = 250; i >= 150; i--) {
      int value = pow(2, i / 32.0);
      FastLED.setBrightness(value);
      FastLED.show();
      delay(s);
    }
  }

  if (!on) {
    leds[0].setRGB(0, 0, 0);
    FastLED.show();
  }

  unsigned long endTime = millis();              // Record the end time
  unsigned long duration = endTime - startTime;  // Calculate the duration
  Serial.println(duration);
  delay(3000 - duration);
}



void loop() {
  // Store measured value into point
  sensor.clearFields();
  // Report RSSI of currently connected network
  sensor.addField("rssi", WiFi.RSSI());
  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(sensor));
  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
    showStat(255, 0, 0, 2, 2, false);
  }
  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
    showStat(255, 120, 0, 2, 2, false);
  }

  //Wait 10s
  Serial.println("Wait 10s");
  delay(2000);
}
