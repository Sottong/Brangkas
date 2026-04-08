#include <Arduino.h>
#include <Wire.h>
#include <I2CKeyPad.h>

// Definisi Pin I2C
#define PIN_I2C_SDA 13
#define PIN_I2C_SCL 14

// Alamat I2C Keypad (bisa 0x20 atau 0x27)
#define KEYPAD_I2C_ADDR 0x27

// Objek I2C Keypad
I2CKeyPad keyPad(KEYPAD_I2C_ADDR);

// Layout Keypad 4x4 Standar
char keyMap[] = "123A456B789C*0#D";

// Variabel untuk melacak status tombol sebelumnya
char lastKey = 'N'; 

void setup() {
  Serial.begin(115200);
  while (!Serial); // Tunggu sampai Serial siap
  
  Serial.println("\nMemulai Test Keypad (Trigger Saat Dilepas)...");

  // Inisialisasi I2C Bus dengan pin custom
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Inisialisasi Keypad
  if (keyPad.begin()) {
    Serial.println("I2C Keypad berhasil diinisialisasi pada alamat 0x27.");
    keyPad.loadKeyMap(keyMap);
  } else {
    Serial.println("GAGAL inisialisasi I2C Keypad. Periksa koneksi atau alamat I2C.");
    for (;;); // Berhenti di sini jika gagal
  }

  Serial.println("\n--- Sistem Siap ---");
  Serial.println("Tahan tombol, dan lepaskan untuk memicu input.");
}

void loop() {
  // Secara default, asumsikan tidak ada tombol yang ditekan ('N' = None/Null)
  char currentKey = 'N'; 

  // Cek apakah ada tombol yang sedang ditekan
  if (keyPad.isPressed()) {
    currentKey = keyPad.getChar();
  }

  // LOGIKA TRIGGER SAAT DILEPAS (RELEASE DETECT):
  // Jika sebelumnya ada tombol yang ditekan (lastKey != 'N')
  // DAN saat ini tidak ada tombol yang ditekan (currentKey == 'N')
  if (lastKey != 'N' && currentKey == 'N') {
    Serial.print("Tombol diproses (setelah dilepas): ");
    Serial.println(lastKey);
    
    // Taruh aksi eksekusi/output di sini
    // (Misal: memasukkan karakter ke buffer, membunyikan buzzer, dll)
  }

  // Update lastKey dengan status tombol saat ini untuk pembacaan loop berikutnya
  lastKey = currentKey;

  // Delay kecil untuk debouncing fisik tombol
  delay(50);
}
