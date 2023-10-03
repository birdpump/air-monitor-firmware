#include <FastLED.h>
#include <Arduino.h>
#include "s8_uart.h"


CRGB leds[1];
unsigned long startTime; // Variable to store the start time


/* BEGIN CONFIGURATION */
#define DEBUG_BAUDRATE 115200

#if (defined USE_SOFTWARE_SERIAL || defined ARDUINO_ARCH_RP2040)
  #define S8_RX_PIN D7         // Rx pin which the S8 Tx pin is attached to (change if it is needed)
  #define S8_TX_PIN D5         // Tx pin which the S8 Rx pin is attached to (change if it is needed)
// #else
//   #define S8_UART_PORT  1     // Change UART port if it is needed
#endif

#ifdef USE_SOFTWARE_SERIAL
  SoftwareSerial S8_serial(S8_RX_PIN, S8_TX_PIN);
// #else
//   #if defined(ARDUINO_ARCH_RP2040)
//     REDIRECT_STDOUT_TO(Serial)    // to use printf (Serial.printf not supported)
//     UART S8_serial(S8_TX_PIN, S8_RX_PIN, NC, NC);
//   #else
//     HardwareSerial S8_serial(S8_UART_PORT);   
//   #endif
#endif

S8_UART *sensor_S8;
S8_sensor sensor;


void setup() {

  // Configure serial port, we need it for debug
  Serial.begin(DEBUG_BAUDRATE);


  FastLED.addLeds<WS2812B, D6>(leds, 1);


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
      while (1) { delay(1); };
  }
}

void showStat(int r, int g, int b, int s, int n){
  startTime = millis(); // Record the start time

  leds[0].setRGB(g, r, b);
  //values int i = 100; i <= 256; i++
  for(int t = n; t > 0; t--){
    for (int i = 150; i <= 256; i++) {
      int value = pow(2, i / 32.0);
      FastLED.setBrightness(value);
      FastLED.show();
      delay(s);
    }
    //values int i = 256; i >= 100; i--
    for (int i = 256; i >= 150; i--) {
      int value = pow(2, i / 32.0);
      FastLED.setBrightness(value);
      FastLED.show();    
      delay(s);
    }
  }

  unsigned long endTime = millis(); // Record the end time
  unsigned long duration = endTime - startTime; // Calculate the duration
  Serial.println(duration);
}

void loop() {

  sensor.co2 = sensor_S8->get_co2();
  printf("CO2 value = %d ppm\n", sensor.co2);

  showStat(0, 0, 255, 10, 1);
  delay(3000);
}