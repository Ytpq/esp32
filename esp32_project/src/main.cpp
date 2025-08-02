#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ButtonHandler.h"
#include "MenuPage.h"
#include "ImageSlideshow.h"
#include "DhtPage.h"
#define SD_CS 4
#include "GpsPage.h"
#include <HardwareSerial.h>



TFT_eSPI tft = TFT_eSPI();        // 只定义一次
DhtPage dhtPage(tft, 17);  // 在主程序中这样初始化
HardwareSerial gpsSerial(1);        // 使用串口1
GpsPage gpsPage(tft, gpsSerial);    // 用这个 tft 实例创建 GPS 页面对象
ImageSlideshow slideshow(tft, SD_CS, 240, 320);  // 用同一个 tft 实例创建图片幻灯片对象

enum PageState { MENU, SHOW_IMAGE, SHOW_TEMP, SHOW_TIME };
PageState currentPage = MENU;
PageState lastPage = MENU;

void setup() {
  Serial.begin(115200);
  
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  initButtons();
  slideshow.begin();
  
  drawMenu(tft);  // 初始显示菜单
  gpsPage.begin();
  dhtPage.begin();  // 初始化 DHT 页面

}

void loop() {
  int btn = getPressedButton();
  
  // 按键处理
  if (btn != -1) {
    switch (btn) {
      case 0: currentPage = SHOW_IMAGE; break;
      case 1: currentPage = SHOW_TEMP; break;
      case 2: currentPage = SHOW_TIME; break;
      case 3: currentPage = MENU; break;
    }
  }

  // 只在页面改变时重绘
  if (currentPage != lastPage) {
    switch (currentPage) {
      case MENU:
        tft.fillScreen(TFT_BLACK);
        drawMenu(tft);
        break;
    }
    lastPage = currentPage;
  }

  // 图片页面持续更新
  if (currentPage == SHOW_IMAGE) {
    slideshow.update();
  }
  if( currentPage == SHOW_TIME) {
    gpsPage.update();
  }
  if (currentPage == SHOW_TEMP) {
    dhtPage.update();
  }
  
  delay(100);  // 减少CPU占用
}