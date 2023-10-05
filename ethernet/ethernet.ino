#include <FastLED.h>
#include <InfluxDbClient.h>
#include <Arduino.h>
#include "s8_uart.h"


#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN D1
#define DHTTYPE    DHT11
DHT_Unified dht(DHTPIN, DHTTYPE);

// fully works in theory lol

// connect led to pin D6
// senseair TX = D1
// senseair RX = D2


//showStat counter and fastled config
CRGB leds[1];
unsigned long startTime;


#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ENC28J60lwIP.h>
#define CSPIN 16
ENC28J60lwIP eth(CSPIN);
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};


// InfluxDB  server url. Don't use localhost, always server name or ip address.
// E.g. http://192.168.1.48:8086 (In InfluxDB 2 UI -> Load Data -> Client Libraries),
#define INFLUXDB_URL "http://10.0.0.63:8086"
// InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "v0aEP8-dlgr-XBS7H6z7CIj15Z_pQqYE_I35NuY59WQdPILv3jrDhXnyYC4Lo7p8rgLcass7RbwT0UDgTFcq8g=="
// InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
#define INFLUXDB_ORG "birdpump"
// InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
#define INFLUXDB_BUCKET "air-monitor"
// InfluxDB v1 database name
//#define INFLUXDB_DB_NAME "database"
// InfluxDB client instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);



//senseairs8 config
#define DEBUG_BAUDRATE 115200

#if (defined USE_SOFTWARE_SERIAL || defined ARDUINO_ARCH_RP2040)
#define S8_RX_PIN D2  // Rx pin which the S8 Tx pin is attached to (change if it is needed)
#define S8_TX_PIN D3  // Tx pin which the S8 Rx pin is attached to (change if it is needed)
// #else
//   #define S8_UART_PORT  1     // Change UART port if it is needed
#endif

#ifdef USE_SOFTWARE_SERIAL
SoftwareSerial S8_serial(S8_RX_PIN, S8_TX_PIN);
#endif

S8_UART *sensor_S8;
S8_sensor sensor;



// create Data point
Point dataSensor("air_monitor");


void setup() {
  dht.begin();

  Serial.begin(DEBUG_BAUDRATE);

  //setup wled for boot
  FastLED.addLeds<WS2812B, D6>(leds, 1);
  leds[0].setRGB(0, 255, 0);
  FastLED.show();


  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setFrequency(4000000);

  eth.setDefault(); // use ethernet for default route
  int present = eth.begin(mac);
  if (!present) {
    Serial.println("no ethernet hardware present");
    while(1);
  }
  
  Serial.print("connecting ethernet");
  while (!eth.connected()) {
    Serial.print(".");
    showStat(0, 255, 0, 2, 2, true);
  }
  Serial.println();
  Serial.print("ethernet ip address: ");
  Serial.println(eth.localIP());

  // Add constant tags - only once
  dataSensor.addTag("device", "esp8266_ethernet");
  dataSensor.addTag("id", "sensor-1");

  configTzTime("PST8PDT", "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
  } else {
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
      showStat(255, 120, 0, 2, 2, false);
      delay(2000);
    };
  }
}


//showStat funtion will allway take 3000ms
void showStat(int r, int g, int b, int s, int n, bool on) {
  startTime = millis();  // Record the start time

  leds[0].setRGB(r, g, b);
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
  delay(3000 - duration);
}



void loop() {
  dataSensor.clearFields();


  //co2 data
  sensor.co2 = sensor_S8->get_co2();
  dataSensor.addField("co2", sensor.co2);

  //uptime data
  dataSensor.addField("uptime", millis());


  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    float temp = (event.temperature * 9/5) +32;
    dataSensor.addField("temperature", temp);
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    dataSensor.addField("humidity", event.relative_humidity);
  }



  Serial.print("Writing Data");
  Serial.println(client.pointToLineProtocol(dataSensor));

  // If no Wifi signal, try to reconnect it
  if (!eth.connected()) {
    Serial.println("Wifi connection lost");
    showStat(255, 0, 0, 2, 2, false);
  } else {
    //check influxdbs connenction
    // Write point
    if (!client.writePoint(dataSensor)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
      showStat(252, 0, 189, 2, 2, false);
    } else {
      //show stat for all ok if write is successful
      showStat(0, 0, 255, 10, 1, true);
    }
  }



  //delay to make function run 5 seconds
  delay(2000);
}
