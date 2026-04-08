#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2CKeyPad.h>

// Definisi Pin I2C
#define PIN_I2C_SDA 13
#define PIN_I2C_SCL 14

// Definisi Konfigurasi OLED
#define SCREEN_WIDTH   128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64 // OLED display height, in pixels
#define OLED_RESET      -1 // Reset pin (or -1 if using Arduino reset)
#define SCREEN_ADDRESS 0x3C // OLED I2C address

// Alamat I2C Keypad (bisa 0x20 atau 0x27)
#define KEYPAD_I2C_ADDR 0x27

// Inisialisasi Objek Layar OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Objek I2C Keypad
I2CKeyPad keyPad(KEYPAD_I2C_ADDR);

// Layout Keypad 4x4 Standar
char keyMap[] = "123A456B789C*0#D";

String inputBuffer = ""; // Untuk menyimpan urutan tombol yang ditekan

void setup() {
  Serial.begin(115200);
  while (!Serial); // Tunggu sampai Serial siap
  
  Serial.println("\nMemulai Test Gabungan OLED & Keypad...");

  // Inisialisasi I2C Bus dengan pin custom
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Inisialisasi Layar OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed. Periksa koneksi atau alamat I2C."));
    for(;;); // Berhenti di sini jika gagal
  }
  
  // Inisialisasi Keypad
  if (keyPad.begin()) {
    Serial.println("I2C Keypad berhasil diinisialisasi pada alamat 0x27.");
    keyPad.loadKeyMap(keyMap);
  } else {
    Serial.println("GAGAL inisialisasi I2C Keypad. Periksa koneksi atau alamat I2C.");
    for (;;); // Berhenti di sini jika gagal
  }

  Serial.println("\n--- Sistem Siap ---");

  // Tampilan awal pada OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Test Sistem Brangkas"));
  display.setCursor(0, 16);
  display.println(F("OLED & Keypad: OK"));
  display.setCursor(0, 32);
  display.println(F("Tekan tombol..."));
  display.display();
}

void loop() {
  // Mengecek apakah ada tombol yang ditekan
  if (keyPad.isPressed()) {
    char key = keyPad.getChar();
    
    // Pastikan nilai tombol valid
    if (key != 'N') {
      Serial.print("Tombol ditekan: ");
      Serial.println(key);
      
      // Update tampilan OLED
      display.clearDisplay();
      
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("Test Sistem Brangkas"));
      
      display.setCursor(0, 16);
      display.println(F("Input Anda:"));
      
      // Logika untuk tombol khusus (misal: C untuk hapus semua, D untuk hapus 1 karakter)
      if (key == 'C') {
        inputBuffer = ""; // Clear buffer
      } else if (key == 'D') {
        if (inputBuffer.length() > 0) {
          inputBuffer.remove(inputBuffer.length() - 1); // Hapus karakter terakhir
        }
      } else {
        // Tambahkan karakter ke buffer (maksimal 10 karakter agar muat di layar)
        if (inputBuffer.length() < 10) {
          inputBuffer += key;
        }
      }
      
      // Tampilkan input yang diketik dengan ukuran text lebih besar (Text Size 2)
      display.setTextSize(2);
      display.setCursor(0, 32);
      display.print(inputBuffer);
      
      // Kirim buffer ke layar OLED
      display.display();
      
      // Delay sederhana untuk debounce dan mencegah pembacaan ganda
      delay(200); 
    }
  }
}
