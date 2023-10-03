#include <FastLED.h>


CRGB leds[1];
unsigned long startTime;


void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2812B, D6>(leds, 1);
}

//showStat funtion will allway take 5000ms
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
  // wifi error
  // showStat(255, 0, 0, 2, 2, false);

  // influxdb error
  // showStat(255, 120, 0, 2, 2, false);

  // sensor error
  // showStat(252, 0, 189, 2, 2, false);

  // normal operation
  showStat(0, 0, 255, 10, 1, true);
  //sensor reading function for testing, not implemented yet
}
