#include "index.h"


LiquidCrystal_I2C lcd(0x26, 16, 2);

void lcd_init(){
  lcd.init(PIN_I2C_SDA, PIN_I2C_SCL);
  lcd.backlight();
}


void lcd1(){
    lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Door is Closed");
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Check Alarm");
        lcd.setCursor(0, 1);
        lcd.print("Switch!");
        delay(2000);
        lcd.clear();
}

void lcd2(){
    lcd.setCursor(0, 0);
    lcd.print("Alarm Switch");
    lcd.setCursor(0, 1);
    lcd.print("is ON!, Fix it!");
}

void lcd3(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Alarm Switch");
    lcd.setCursor(0, 1);
    lcd.print("is OK!");
}

void lcd4(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout!");
    lcd.setCursor(0, 1);
    lcd.print("Locking...");
}

void lcd5(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door Closed!");
}

void lcd6(){
    lcd.setCursor(0, 0);
    lcd.print("Smart Brangkas");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    delay(3000);  
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connecting");
    lcd.setCursor(0, 1);
    lcd.print("Please Wait...");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
}

void lcd7(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door is open");
    lcd.setCursor(0, 1);
    lcd.print("Close it!");
}

void lcd8(){}

void lcd9(){}

void lcd10(){}

void lcd11(){}

void lcd12(){}

void lcd13(){}

void lcd14(){}

void lcd15(){}