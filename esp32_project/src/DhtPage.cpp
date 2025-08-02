#include "DhtPage.h"

DhtPage::DhtPage(TFT_eSPI& tftRef, uint8_t pin) 
  : tft(tftRef), dht(pin, DHT11) {}

void DhtPage::begin() {
  dht.begin();
}

void DhtPage::update() {
  unsigned long now = millis();
  if (now - lastReadTime >= 2000) {  // 2秒读一次
    lastReadTime = now;
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    drawDisplay();
  }
}

void DhtPage::drawDisplay() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);

  if (isnan(temperature) || isnan(humidity)) {
    tft.setCursor(20, 120);
    tft.print("讀取失敗");
  } else {
    tft.setCursor(20, 80);
    tft.printf("溫度: %.1f C", temperature);
    tft.setCursor(20, 140);
    tft.printf("濕度: %.1f %%", humidity);
  }
}
