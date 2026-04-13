#include "DisplaySys.h"
#include "Globals.h"

void initDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("[ERROR] OLED gagal!");
  } else {
    Serial.println("[DEBUG] OLED berhasil diinisialisasi.");
    updateDisplay("Sistem Brangkas", "Memulai...");
  }
}

void updateDisplay(String text1, String text2, String text3, String text4) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println(text1);
  
  if (text2 != "") {
    display.setCursor(0, 16);
    display.println(text2);
  }
  if (text3 != "") {
    display.setCursor(0, 32);
    display.println(text3);
  }
  if (text4 != "") {
    display.setCursor(0, 48);
    display.println(text4);
  }
  
  display.display();
}
