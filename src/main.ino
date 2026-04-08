#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Definisi Pin I2C
#define PIN_I2C_SDA 13
#define PIN_I2C_SCL 14

// Definisi Konfigurasi OLED
#define SCREEN_WIDTH   128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64 // OLED display height, in pixels
#define OLED_RESET      -1 // Reset pin (or -1 if using Arduino reset)
#define SCREEN_ADDRESS 0x3C // OLED I2C address

// Inisialisasi Objek Layar OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  while (!Serial); // Tunggu sampai Serial siap
  
  Serial.println("\nMemulai Test OLED...");

  // Inisialisasi I2C Bus dengan pin custom
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Inisialisasi Layar OLED
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed. Periksa koneksi atau alamat I2C."));
    for(;;); // Berhenti di sini jika gagal
  }

  Serial.println(F("OLED Berhasil Diinisialisasi."));

  // Tampilkan pesan awal di Layar
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE); // Gunakan warna putih
  
  display.setCursor(0, 0);
  display.println(F("Test Layar OLED"));
  
  display.setCursor(0, 16);
  display.println(F("Status: OK"));
  
  display.setCursor(0, 32);
  display.printf("SDA: %d | SCL: %d", PIN_I2C_SDA, PIN_I2C_SCL);
  
  display.setCursor(0, 48);
  display.println(F("Brangkas V2"));
  
  // Kirim buffer ke layar
  display.display();
}

void loop() {
  // Contoh animasi sederhana berkedip
  delay(2000);
  display.invertDisplay(true);
  delay(500);
  display.invertDisplay(false);
}
