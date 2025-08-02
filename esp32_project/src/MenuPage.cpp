#include "MenuPage.h"

void drawMenu(TFT_eSPI &tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);

  tft.setCursor(40, 60);
  tft.print("1. Show Image");

  tft.setCursor(40, 120);
  tft.print("2. Show Temp");

  tft.setCursor(40, 180);
  tft.print("3. Show Time");

  tft.setCursor(20, 260);
  tft.setTextSize(1);
  tft.print("Press button to select, back to return.");
}
