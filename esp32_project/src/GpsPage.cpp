#include "GpsPage.h"

GpsPage::GpsPage(TFT_eSPI& tftRef, HardwareSerial& serialRef)
  : tft(tftRef), gpsSerial(serialRef) {}

void GpsPage::begin() {
  gpsSerial.begin(9600, SERIAL_8N1, 16, -1); // RX = GPIO16，TX不接
}

void GpsPage::update() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  unsigned long now = millis();
  if (now - lastReport >= 3000) {
    lastReport = now;

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);

    int y = 10;

    if (gps.location.isValid()) {
      drawText("緯度", String(gps.location.lat(), 6), y); y += 30;
      drawText("經度", String(gps.location.lng(), 6), y); y += 30;
    } else {
      drawText("位置", "無效", y); y += 30;
    }

    if (gps.date.isValid()) {
      char buf[16];
      sprintf(buf, "%02d/%02d/%04d", gps.date.day(), gps.date.month(), gps.date.year());
      drawText("日期", buf, y); y += 30;
    } else {
      drawText("日期", "無效", y); y += 30;
    }

    if (gps.time.isValid()) {
      int hour = gps.time.hour() + 8;
      if (hour >= 24) hour -= 24;
      char buf[16];
      sprintf(buf, "%02d:%02d:%02d", hour, gps.time.minute(), gps.time.second());
      drawText("時間", buf, y); y += 30;
    } else {
      drawText("時間", "無效", y); y += 30;
    }

    drawText("衛星", String(gps.satellites.value()), y); y += 30;
    drawText("HDOP", String(gps.hdop.hdop(), 1), y); y += 30;
  }
}

void GpsPage::drawText(const String& label, const String& value, int y) {
  tft.setCursor(10, y);
  tft.print(label + ": " + value);
}
