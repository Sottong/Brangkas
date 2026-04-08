#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2CKeyPad.h>
#include <Adafruit_Fingerprint.h>

// ==========================================
// 1. DEFINISI PIN HARDWARE & ALAMAT
// ==========================================
// I2C (Keypad & OLED)
#define PIN_I2C_SDA 13
#define PIN_I2C_SCL 14
#define KEYPAD_I2C_ADDR 0x27
#define SCREEN_ADDRESS  0x3C

// UART (Fingerprint)
#define PIN_FP_RX 32
#define PIN_FP_TX 33

// OLED Config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// ==========================================
// 2. INISIALISASI OBJEK
// ==========================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

I2CKeyPad keyPad(KEYPAD_I2C_ADDR);
char keyMap[] = "123A456B789C*0#D";
char lastKey = 'N'; // Untuk logika trigger saat dilepas

HardwareSerial mySerial(2); 
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// ==========================================
// 3. DEFINISI STATE MACHINE
// ==========================================
enum SystemState {
  STATE_IDLE,
  STATE_AUTH_FINGER,
  STATE_AUTH_PIN,
  STATE_UNLOCKED,
  STATE_ALARM,
  STATE_ADMIN
};

SystemState currentState = STATE_IDLE;

// Variabel Timer non-blocking
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 1000; // Update layar setiap 1 detik jika idle

// ==========================================
// 4. DEKLARASI FUNGSI
// ==========================================
void updateDisplay(String text1, String text2 = "", String text3 = "");
char readKeypad();
void handleIdleState();

// ==========================================
// 5. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n\n===================================");
  Serial.println("  SISTEM BRANGKAS V2 - FASE 1      ");
  Serial.println("===================================");

  // Inisialisasi I2C Bus
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Inisialisasi OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("[ERROR] OLED gagal diinisialisasi!");
  } else {
    Serial.println("[OK] OLED siap.");
    updateDisplay("Sistem Brangkas", "Memulai...");
  }

  // Inisialisasi Keypad
  if (keyPad.begin()) {
    Serial.println("[OK] Keypad siap.");
    keyPad.loadKeyMap(keyMap);
  } else {
    Serial.println("[ERROR] Keypad gagal diinisialisasi!");
  }

  // Inisialisasi Fingerprint
  mySerial.begin(57600, SERIAL_8N1, PIN_FP_RX, PIN_FP_TX);
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("[OK] Fingerprint siap.");
  } else {
    Serial.println("[ERROR] Fingerprint tidak terdeteksi!");
  }

  delay(2000); // Tahan sebentar untuk membaca pesan startup
  
  // Set ke State Awal
  currentState = STATE_IDLE;
  updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
}

// ==========================================
// 6. MAIN LOOP (NON-BLOCKING)
// ==========================================
void loop() {
  // State Machine Switcher
  switch (currentState) {
    case STATE_IDLE:
      handleIdleState();
      break;

    case STATE_AUTH_FINGER:
      // Akan diimplementasikan di Fase 2
      break;

    case STATE_AUTH_PIN:
      // Akan diimplementasikan di Fase 2
      break;

    case STATE_UNLOCKED:
      // Akan diimplementasikan
      break;

    case STATE_ALARM:
      // Akan diimplementasikan di Fase 3
      break;

    case STATE_ADMIN:
      // Akan diimplementasikan di Fase 2
      break;
  }
}

// ==========================================
// 7. FUNGSI-FUNGSI BANTUAN
// ==========================================

// Fungsi Update Layar OLED
void updateDisplay(String text1, String text2, String text3) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println(text1);
  
  display.setCursor(0, 16);
  display.println(text2);
  
  display.setCursor(0, 32);
  display.println(text3);
  
  display.display();
}

// Fungsi Membaca Keypad (Trigger saat dilepas)
char readKeypad() {
  char currentKey = 'N'; 
  char pressedKey = 'N';

  if (keyPad.isPressed()) {
    currentKey = keyPad.getChar();
  }

  // Trigger saat tombol dilepas
  if (lastKey != 'N' && currentKey == 'N') {
    pressedKey = lastKey; // Karakter yang baru saja dilepas
  }

  lastKey = currentKey;
  delay(10); // Debounce kecil
  
  return pressedKey;
}

// Handler untuk STATE_IDLE
void handleIdleState() {
  // 1. Cek Input Keypad
  char key = readKeypad();
  if (key != 'N') {
    Serial.print("Input Keypad di Idle: ");
    Serial.println(key);
    
    // Contoh transisi (Nanti di Fase 2 akan lebih kompleks)
    if (key == '*') {
      updateDisplay("Mode Admin", "Masukkan PIN Master");
      currentState = STATE_ADMIN;
    } else {
      updateDisplay("Key Ditekan", String(key), "Kembali Idle...");
      delay(1000); // Sederhana untuk demo, hindari delay panjang nanti
      updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
    }
  }

  // 2. Polling Fingerprint (Non-Blocking)
  // Untuk fase 1, kita hanya cek apakah ada jari tanpa processing panjang
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_OK) {
    Serial.println("Jari terdeteksi! Pindah ke AUTH_FINGER");
    updateDisplay("Memproses...", "Membaca Sidik Jari");
    currentState = STATE_AUTH_FINGER; 
    // Nanti akan dikembalikan ke IDLE atau lanjut ke AUTH_PIN di Fase 2
    delay(1000); // Simulasi proses, akan diubah nanti
    currentState = STATE_IDLE;
    updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
  }

  // 3. Update Status Berkala (Blink/Heartbeat)
  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = millis();
    // Bisa digunakan untuk menampilkan jam atau status sensor lain di background
  }
}
