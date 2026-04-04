#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>
#include <I2CKeyPad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==========================================
// 1. DEFINISI PIN HARDWARE
// ==========================================
#define PIN_LIMIT_SWITCH_1  0
#define PIN_LIMIT_SWITCH_23 2
#define PIN_DOOR_LOCK       15
#define PIN_BUZZER          12

// Pin UART Fingerprint (Serial2)
#define PIN_FP_RX           32
#define PIN_FP_TX           33

// Pin I2C (Display, Keypad)
#define PIN_I2C_SDA         13
#define PIN_I2C_SCL         14

// ==========================================
// 2. ALAMAT I2C & OBJEK PERIPHERAL
// ==========================================
#define KEYPAD_I2C_ADDR     0x27
#define OLED_I2C_ADDR       0x3C

// --- Objek Fingerprint ---
HardwareSerial mySerial(2); 
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// --- Objek Keypad (0x27) ---
I2CKeyPad keyPad(KEYPAD_I2C_ADDR);
char keyMap[] = "123A456B789C*0#D"; // Layout Keypad 4x4 Standar

// --- Objek OLED (0x3C) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==========================================
// 3. SETUP: INISIALISASI SISTEM
// ==========================================
void setup() {
  // A. Inisialisasi Serial Monitor
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n===================================");
  Serial.println("  SISTEM BRANGKAS V2 - STARTINGUP  ");
  Serial.println("===================================");

  // B. Inisialisasi Pin I/O Dasar (Limit Switch, Door Lock, Buzzer)
  pinMode(PIN_LIMIT_SWITCH_1, INPUT_PULLUP);
  pinMode(PIN_LIMIT_SWITCH_23, INPUT_PULLUP);
  
  pinMode(PIN_DOOR_LOCK, OUTPUT);
  digitalWrite(PIN_DOOR_LOCK, LOW); // Door Lock terkunci di awal
  
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW); // Buzzer mati di awal
  Serial.println("[OK] GPIO Inisialisasi Selesai.");

  // C. Inisialisasi Komunikasi I2C (Custom Pins)
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Serial.println("[OK] I2C Bus dimulai (SDA:13, SCL:14).");

  // D. Inisialisasi Layar OLED (0x3C)
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println("[ERROR] Gagal inisialisasi OLED (0x3C) - Abaikan jika rusak.");
  } else {
    Serial.println("[OK] OLED SSD1306 siap.");
    // Kembalikan pin I2C setelah dipanggil oleh library OLED
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); 
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("BRANGKAS V2");
    display.setCursor(0, 16);
    display.println("Sistem Siap...");
    display.display();
  }

  delay(2000);

  // E. Inisialisasi I2C Keypad (0x27)
  if(keyPad.begin()) {
    Serial.println("[OK] I2C Keypad siap (0x27).");
    keyPad.loadKeyMap(keyMap);
    // Kembalikan pin I2C setelah dipanggil oleh library Keypad
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  } else {
    Serial.println("[ERROR] Gagal inisialisasi I2C Keypad (0x27).");
  }

  // F. Inisialisasi Sensor Sidik Jari (UART2)
  mySerial.begin(57600, SERIAL_8N1, PIN_FP_RX, PIN_FP_TX);
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("[OK] Sensor Fingerprint siap.");
  } else {
    Serial.println("[ERROR] Sensor Fingerprint tidak terdeteksi.");
  }

  // G. Indikator Selesai Setup (Buzzer Beep)
  digitalWrite(PIN_BUZZER, HIGH);
  delay(100);
  digitalWrite(PIN_BUZZER, LOW);
  
  Serial.println("===================================");
  Serial.println("       SETUP SELESAI               ");
  Serial.println("===================================\n");
}

// ==========================================
// 4. LOOP: LOGIKA UTAMA APLIKASI
// ==========================================
void loop() {
  // TODO: Tambahkan logika utama di sini
  // 1. Baca Keypad untuk PIN
  // 2. Baca Fingerprint untuk sidik jari
  // 3. Update status ke OLED (jika sudah diperbaiki)
  // 4. Buka Door Lock jika autentikasi berhasil
  // 5. Cek Limit Switch untuk mendeteksi pintu tertutup otomatis
  
}
