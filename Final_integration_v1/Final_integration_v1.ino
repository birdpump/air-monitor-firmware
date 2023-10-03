#include <FastLED.h>
#include <InfluxDbClient.h>
#include <Arduino.h>
#include "s8_uart.h"

// fully works in theory lol

// connect led to pin D6
// senseair TX = D5
// senseair RX = D7


//showStat counter and fastled config
CRGB leds[1];
unsigned long startTime;


//influxdb config
#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif
// WiFi AP SSID
#define WIFI_SSID "CB5"
// WiFi password
#define WIFI_PASSWORD "###########"
// InfluxDB  server url. Don't use localhost, always server name or ip address.
// E.g. http://192.168.1.48:8086 (In InfluxDB 2 UI -> Load Data -> Client Libraries), 
#define INFLUXDB_URL "http://pve-air-server:8086"
// InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "emlKy6kUmHULg93RjDJIIno6tJkXHThG9j-zIUWTjdnSo3ujhuLfvV4OO77fdywUwEadsAjHjwD-RUdjwSsIzQ=="
// InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
#define INFLUXDB_ORG "cvhs"
// InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
#define INFLUXDB_BUCKET "cvhs-main"
// InfluxDB v1 database name 
//#define INFLUXDB_DB_NAME "database"
// InfluxDB client instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);



//senseairs8 config
#define DEBUG_BAUDRATE 115200

#if (defined USE_SOFTWARE_SERIAL || defined ARDUINO_ARCH_RP2040)
  #define S8_RX_PIN D7         // Rx pin which the S8 Tx pin is attached to (change if it is needed)
  #define S8_TX_PIN D5         // Tx pin which the S8 Rx pin is attached to (change if it is needed)
// #else
//   #define S8_UART_PORT  1     // Change UART port if it is needed
#endif

#ifdef USE_SOFTWARE_SERIAL
  SoftwareSerial S8_serial(S8_RX_PIN, S8_TX_PIN);
#endif

S8_UART *sensor_S8;
S8_sensor sensor;



// create Data point
Point sensor("air_monitor");


void setup() {
  Serial.begin(DEBUG_BAUDRATE);

  //setup wled for boot
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

  // Add constant tags - only once
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());
  sensor.addTag("id", "sensor-1");

  configTzTime("PST8PDT", "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
  }else{
    showStat(252, 0, 189, 2, 2, false);
  }



  // client batch size 
  // 12 will cause update every minute
//   client.setWriteOptions(WriteOptions().batchSize(12));

  // client buffer will help with conectivity issues
  // buffer set to 2x batch size
  // buffer can we used without batching
  client.setWriteOptions(WriteOptions().bufferSize(24));

  //sensairs8 initalize
  // Wait port is open or timeout
  int i = 0;
  while (!Serial && i < 50) {
    delay(10);
    i++;
  }
  // Initialize S8 sensor
  S8_serial.begin(S8_BAUDRATE);
  sensor_S8 = new S8_UART(S8_serial);

  // Check if S8 is available
  sensor_S8->get_firmware_version(sensor.firm_version);
  int len = strlen(sensor.firm_version);
  if (len == 0) {
      Serial.println("SenseAir S8 CO2 sensor not found!");
      while (1) { 
        showStat(252, 0, 189, 2, 2, false);
        delay(2000);
      };
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
  sensor.clearFields();

  //wifi rssi data
  sensor.addField("rssi", WiFi.RSSI());

  //co2 data
  sensor.co2 = sensor_S8->get_co2();
  sensor.addField("co2", sensor.co2);

  //uptime data
  pointDevice.addField("uptime", millis());


  Serial.print("Writing Data");
  Serial.println(client.pointToLineProtocol(sensor));

  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
    showStat(255, 0, 0, 2, 2, false);
  }else{
    //check influxdbs connenction
    // Write point
    if (!client.writePoint(sensor)) {
        Serial.print("InfluxDB write failed: ");
        Serial.println(client.getLastErrorMessage());
        showStat(252, 0, 189, 2, 2, false);
    }else{
        //show stat for all ok if write is successful
        showStat(0, 0, 255, 10, 1, true);
    }
    }
  }

  
  //delay to make function run 5 seconds
  delay(2000);
}
