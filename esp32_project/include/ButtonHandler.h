#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

void initButtons();       // 初始化按键
int getPressedButton();   // 返回被按下的按钮编号（0~3），没按返回 -1
void nextImage();
#endif
