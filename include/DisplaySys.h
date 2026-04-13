#ifndef DISPLAY_SYS_H
#define DISPLAY_SYS_H

#include <Arduino.h>

void initDisplay();
void updateDisplay(String text1, String text2 = "", String text3 = "", String text4 = "");

#endif
