#include <Arduino.h>
#include <Wire.h>
#include <I2CKeyPad.h>

// Definisi Pin I2C
#define PIN_I2C_SDA 13
#define PIN_I2C_SCL 14

// Alamat I2C Keypad (bisa 0x20 atau 0x27, tergantung modul I2C yang digunakan)
#define KEYPAD_I2C_ADDR 0x27

// Objek I2C Keypad
I2CKeyPad keyPad(KEYPAD_I2C_ADDR);

// Layout Keypad 4x4 Standar
char keyMap[] = "123A456B789C*0#D";

void setup() {
  Serial.begin(115200);
  while (!Serial); // Tunggu sampai Serial siap
  
  Serial.println("\nMemulai Test I2C Keypad...");
  Serial.printf("Menggunakan SDA: %d, SCL: %d\n", PIN_I2C_SDA, PIN_I2C_SCL);

  // Inisialisasi I2C Bus dengan pin custom
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Inisialisasi Keypad
  // Pastikan Wire.begin() dipanggil sebelum keyPad.begin()
  if (keyPad.begin()) {
    Serial.println("I2C Keypad berhasil diinisialisasi pada alamat 0x27.");
    
    // Load layout keypad kustom
    keyPad.loadKeyMap(keyMap);
  } else {
    Serial.println("GAGAL inisialisasi I2C Keypad. Periksa koneksi atau alamat I2C (coba 0x20 atau 0x27).");
    for (;;); // Berhenti di sini jika gagal
  }

  Serial.println("\n--- Sistem Siap ---");
  Serial.println("Silakan tekan tombol pada keypad...");
}

void loop() {
  // Mengecek apakah ada tombol yang ditekan
  if (keyPad.isPressed()) {
    char key = keyPad.getChar();
    
    // Pastikan nilai tombol valid (biasanya library akan me-return 'N' jika tidak ada)
    if (key != 'N') {
      Serial.print("Tombol ditekan: ");
      Serial.println(key);
      
      // Delay sederhana untuk debounce dan mencegah pembacaan ganda yang cepat
      delay(200); 
    }
  }
}
