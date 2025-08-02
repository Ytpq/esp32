#ifndef GPSPAGE_H
#define GPSPAGE_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

class GpsPage {
public:
  GpsPage(TFT_eSPI& tftRef, HardwareSerial& serialRef);
  void begin();
  void update();

private:
  TFT_eSPI& tft;
  HardwareSerial& gpsSerial;
  TinyGPSPlus gps;

  unsigned long lastReport = 0;

  void drawText(const String& label, const String& value, int y);
};

#endif
