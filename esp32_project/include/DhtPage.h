#ifndef DHTPAGE_H
#define DHTPAGE_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <DHT.h>

class DhtPage {
public:
  DhtPage(TFT_eSPI& tftRef, uint8_t pin);  // 需要 pin 参数
  void begin();
  void update();
private:
  TFT_eSPI& tft;
  DHT dht;
  unsigned long lastReadTime = 0;
  float temperature = NAN;
  float humidity = NAN;

  void drawDisplay();
};

#endif
