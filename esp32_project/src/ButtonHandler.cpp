#include "ButtonHandler.h"
#include <Arduino.h>

#define BUTTON_IMAGE  25
#define BUTTON_TEMP   33
#define BUTTON_TIME   32
#define BUTTON_BACK   27

const int buttonPins[4] = {
  BUTTON_IMAGE,
  BUTTON_TEMP,
  BUTTON_TIME,
  BUTTON_BACK
};

void initButtons() {
  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

// 添加到全局变量区域
const unsigned long DEBOUNCE_DELAY = 50;  // 50ms防抖延时
unsigned long lastDebounceTime[4] = {0, 0, 0, 0};  // 每个按键的上次变化时间
bool lastButtonState[4] = {false, false, false, false};  // 每个按键的上次状态

int getPressedButton() {
  for (int i = 0; i < 4; i++) {
    bool currentState = !digitalRead(buttonPins[i]);  // LOW = 按下
    
    // 如果按键状态发生变化，重置防抖定时器
    if (currentState != lastButtonState[i]) {
      lastDebounceTime[i] = millis();
      lastButtonState[i] = currentState;
    }
    
    // 如果状态稳定超过防抖时间
    if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
      if (currentState) {  // 按键确实被按下
        return i;
      }
    }
  }
  return -1;
}