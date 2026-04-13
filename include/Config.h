#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. DEFINISI PIN HARDWARE & ALAMAT
// ==========================================
#define RELAY_PIN 0
#define LIMIT_SWITCH_1_PIN 15
#define LIMIT_SWITCH_2_PIN 2
#define BUZZER_PIN 12

// Pin Kamera WROVER-KIT
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    21
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      19
#define Y4_GPIO_NUM      18
#define Y3_GPIO_NUM       5
#define Y2_GPIO_NUM       4
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

#define PIN_I2C_SDA 13
#define PIN_I2C_SCL 14
#define KEYPAD_I2C_ADDR 0x27
#define SCREEN_ADDRESS  0x3C

#define PIN_FP_RX 32
#define PIN_FP_TX 33

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// ==========================================
// 2. DEFINISI KONSTANTA SISTEM
// ==========================================
const String USER_PIN = "1234";    // PIN User default
const String MASTER_PIN = "9999";  // PIN Master default

#endif // CONFIG_H
