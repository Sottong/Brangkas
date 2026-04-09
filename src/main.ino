#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2CKeyPad.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "esp_camera.h"

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
// 2. INISIALISASI OBJEK
// ==========================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
I2CKeyPad keyPad(KEYPAD_I2C_ADDR);
char keyMap[] = "123A456B789C*0#D";
char lastKey = 'N'; 

HardwareSerial mySerial(2); 
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// ==========================================
// 3. DEFINISI STATE MACHINE & VARIABEL
// ==========================================
enum SystemState {
  STATE_IDLE,
  STATE_AUTH_FINGER,
  STATE_AUTH_PIN,
  STATE_UNLOCKED,
  STATE_ALARM,
  STATE_ADMIN,
  STATE_ADMIN_AUTH
};

SystemState currentState = STATE_IDLE;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 1000;

// Variabel Alarm
unsigned long alarmStartTime = 0;

// Variabel Autentikasi
String inputBuffer = "";
const String USER_PIN = "1234";    // PIN User default
const String MASTER_PIN = "9999";  // PIN Master default
int failedAttempts = 0;
unsigned long cooldownStartTime = 0;
bool inCooldown = false;

// Variabel Admin Mode
int adminMenuState = 0; // 0: Menu utama, 1: Enroll tunggu ID, 2: Enroll tunggu jari, 3: Delete tunggu ID
int targetID = 0;
int enrollStep = 0;

// ==========================================
// 4. DEKLARASI FUNGSI
// ==========================================
void updateDisplay(String text1, String text2 = "", String text3 = "", String text4 = "");
char readKeypad();
void handleIdleState();
void handleAuthFingerState();
void handleAuthPinState();
void handleAdminAuthState();
void handleAdminState();
void handleAlarmState();
void initCamera();
void initWiFi();

// ==========================================
// 5. INISIALISASI KAMERA & WIFI
// ==========================================
void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // 1600x1200
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[ERROR] Inisialisasi kamera gagal dengan error 0x%x\n", err);
  } else {
    Serial.println("[DEBUG] Kamera berhasil diinisialisasi.");
  }
}

void initWiFi() {
  WiFiManager wm;
  Serial.println("[WIFI] Menghubungkan ke WiFi...");
  updateDisplay("WIFI SETUP", "Menghubungkan...", "AP: Brangkas_AP", "Pass: 12345678");
  
  bool res = wm.autoConnect("Brangkas_AP", "12345678"); 
  
  if(!res) {
    Serial.println("[ERROR] Gagal terhubung ke WiFi.");
    updateDisplay("WIFI GAGAL", "Silakan Restart");
  } else {
    Serial.println("[WIFI] Berhasil terhubung ke WiFi!");
    updateDisplay("WIFI TERHUBUNG", WiFi.localIP().toString());
    delay(2000);
  }
}

// ==========================================
// 6. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n\n===================================");
  Serial.println("  SISTEM BRANGKAS V2 - FASE 4      ");
  Serial.println("===================================");

  // Inisialisasi Pin Relay, Limit Switch, Buzzer
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Asumsi LOW = Terkunci
  
  pinMode(LIMIT_SWITCH_1_PIN, INPUT_PULLDOWN); // Internal Pull-Down (Aktif High)
  pinMode(LIMIT_SWITCH_2_PIN, INPUT_PULLDOWN); // Internal Pull-Down (Aktif High)
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("[ERROR] OLED gagal!");
  } else {
    Serial.println("[DEBUG] OLED berhasil diinisialisasi.");
    updateDisplay("Sistem Brangkas", "Memulai...");
  }

  if (keyPad.begin()) {
    Serial.println("[DEBUG] Keypad berhasil diinisialisasi.");
    keyPad.loadKeyMap(keyMap);
  } else {
    Serial.println("[ERROR] Keypad gagal!");
  }

  mySerial.begin(57600, SERIAL_8N1, PIN_FP_RX, PIN_FP_TX);
  finger.begin(57600);
  if (!finger.verifyPassword()) {
    Serial.println("[ERROR] Fingerprint tidak terdeteksi!");
  } else {
    Serial.println("[DEBUG] Fingerprint terdeteksi dan siap.");
  }

  // Memberi jeda agar arus stabil sebelum menyalakan komponen berat
  delay(1000); 
  
  initCamera();
  
  // Memberi jeda agar daya stabil setelah kamera aktif sebelum radio WiFi memicu lonjakan arus
  delay(1500); 
  
  initWiFi();

  delay(2000);
  currentState = STATE_IDLE;
  Serial.println("[STATE] Berpindah ke STATE_IDLE");
  updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
}

// ==========================================
// 6. MAIN LOOP (NON-BLOCKING)
// ==========================================
void loop() {
  // Cek limit switch untuk alarm (jika sedang tidak unlocked dan tidak alarm)
  if (currentState != STATE_UNLOCKED && currentState != STATE_ALARM) {
    if (digitalRead(LIMIT_SWITCH_1_PIN) == LOW || digitalRead(LIMIT_SWITCH_2_PIN) == LOW) {
      Serial.println("[ALARM] LIMIT SWITCH TERPICU!");
      currentState = STATE_ALARM;
      alarmStartTime = millis();
      digitalWrite(BUZZER_PIN, HIGH);
      updateDisplay("PERINGATAN!", "Limit Switch", "Terpicu!");
    }
  }

  if (inCooldown) {
    if (millis() - cooldownStartTime > 60000) {
      Serial.println("[COOLDOWN] Waktu habis. Kembali ke STATE_IDLE.");
      inCooldown = false;
      failedAttempts = 0;
      currentState = STATE_IDLE;
      updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
    } else {
      if (millis() - lastDisplayUpdate >= 1000) {
         lastDisplayUpdate = millis();
         int timeLeft = 60 - ((millis() - cooldownStartTime) / 1000);
         Serial.printf("[COOLDOWN] Sisa waktu: %d detik\n", timeLeft);
         updateDisplay("COOLDOWN", "Tunggu:", String(timeLeft) + " detik");
      }
      return; // Skip the rest of the loop
    }
  }

  switch (currentState) {
    case STATE_IDLE:
      handleIdleState();
      break;
    case STATE_AUTH_FINGER:
      handleAuthFingerState();
      break;
    case STATE_AUTH_PIN:
      handleAuthPinState();
      break;
    case STATE_UNLOCKED:
      Serial.println("[STATE] Berpindah ke STATE_UNLOCKED. Pintu terbuka.");
      updateDisplay("TERBUKA", "Silakan Buka", "Pintu");
      delay(3000);
      digitalWrite(RELAY_PIN, LOW); // Kunci kembali relay
      Serial.println("[STATE] Kembali ke STATE_IDLE mengunci kembali.");
      currentState = STATE_IDLE;
      updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
      break;
    case STATE_ALARM:
      handleAlarmState();
      break;
    case STATE_ADMIN_AUTH:
      handleAdminAuthState();
      break;
    case STATE_ADMIN:
      handleAdminState();
      break;
  }
}

// ==========================================
// 7. IMPLEMENTASI STATE HANDLER
// ==========================================

void handleAlarmState() {
  // Alarm berbunyi selama 20 detik
  if (millis() - alarmStartTime >= 20000) {
    if (digitalRead(LIMIT_SWITCH_1_PIN) == HIGH && digitalRead(LIMIT_SWITCH_2_PIN) == HIGH) {
      Serial.println("[ALARM] Kondisi aman. Mematikan alarm.");
      digitalWrite(BUZZER_PIN, LOW);
      currentState = STATE_IDLE;
      updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
    } else {
      Serial.println("[ALARM] Limit switch masih terpicu. Reset timer alarm.");
      alarmStartTime = millis(); // Perpanjang alarm 20 detik
    }
  }
  
  // Bisa tambahkan override admin di sini jika ingin matikan manual
}

void handleIdleState() {
  char key = readKeypad();
  if (key != 'N') {
    Serial.print("[KEYPAD] Tombol ditekan di IDLE: "); Serial.println(key);
  }
  
  if (key == '*') {
    Serial.println("[STATE] Berpindah ke STATE_ADMIN_AUTH");
    inputBuffer = "";
    currentState = STATE_ADMIN_AUTH;
    updateDisplay("MODE ADMIN", "Masukkan PIN Master:", "");
    return;
  }

  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_OK) {
    Serial.println("[FINGERPRINT] Jari terdeteksi di sensor.");
    Serial.println("[STATE] Berpindah ke STATE_AUTH_FINGER");
    currentState = STATE_AUTH_FINGER;
    updateDisplay("Memproses...", "Membaca Sidik Jari");
  }
}

void handleAuthFingerState() {
  uint8_t p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.println("[FINGERPRINT] Gagal mengkonversi gambar jari.");
    updateDisplay("GAGAL", "Sidik Jari", "Tidak Terbaca");
    delay(1000);
    currentState = STATE_IDLE;
    updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
    return;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.printf("[FINGERPRINT] Cocok! ID: %d, Confidence: %d\n", finger.fingerID, finger.confidence);
    Serial.println("[STATE] Berpindah ke STATE_AUTH_PIN");
    updateDisplay("FP COCOK!", "ID: " + String(finger.fingerID), "Masukkan PIN:");
    inputBuffer = "";
    currentState = STATE_AUTH_PIN;
  } else {
    failedAttempts++;
    Serial.printf("[FINGERPRINT] Tidak dikenal. Percobaan gagal ke-%d\n", failedAttempts);
    updateDisplay("GAGAL", "Sidik Jari", "Tidak Dikenal");
    delay(1500);
    if (failedAttempts >= 3) {
      Serial.println("[COOLDOWN] Terlalu banyak percobaan gagal. Memulai cooldown.");
      inCooldown = true;
      cooldownStartTime = millis();
    } else {
      currentState = STATE_IDLE;
      updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
    }
  }
}

void handleAuthPinState() {
  char key = readKeypad();
  if (key != 'N') {
    Serial.print("[KEYPAD] Tombol ditekan di AUTH_PIN: "); Serial.println(key);
    if (key == '#') {
      Serial.print("[AUTH] Mengecek PIN User: "); Serial.println(inputBuffer);
      if (inputBuffer == USER_PIN) {
        Serial.println("[AUTH] PIN User BENAR!");
        failedAttempts = 0;
        currentState = STATE_UNLOCKED;
        digitalWrite(RELAY_PIN, HIGH); // Buka kunci relay
      } else {
        failedAttempts++;
        Serial.printf("[AUTH] PIN User SALAH! Percobaan gagal ke-%d\n", failedAttempts);
        updateDisplay("PIN SALAH!", "Coba Lagi");
        delay(1500);
        if (failedAttempts >= 3) {
          Serial.println("[COOLDOWN] Terlalu banyak percobaan gagal. Memulai cooldown.");
          inCooldown = true;
          cooldownStartTime = millis();
        } else {
          currentState = STATE_IDLE;
          updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
        }
      }
    } else if (key == '*') {
      Serial.println("[STATE] Batal masuk PIN, kembali ke IDLE");
      currentState = STATE_IDLE;
      updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
    } else {
      inputBuffer += key;
      String stars = "";
      for(int i=0; i<inputBuffer.length(); i++) stars += "*";
      updateDisplay("Masukkan PIN:", stars);
    }
  }
}

void handleAdminAuthState() {
  char key = readKeypad();
  if (key != 'N') {
    Serial.print("[KEYPAD] Tombol ditekan di ADMIN_AUTH: "); Serial.println(key);
    if (key == '#') {
      Serial.print("[ADMIN_AUTH] Mengecek PIN Master: "); Serial.println(inputBuffer);
      if (inputBuffer == MASTER_PIN) {
        Serial.println("[ADMIN_AUTH] PIN Master BENAR. Masuk ke ADMIN MENU.");
        adminMenuState = 0;
        currentState = STATE_ADMIN;
        updateDisplay("ADMIN MENU", "1: Tambah FP", "2: Hapus FP", "3: Keluar");
      } else {
        Serial.println("[ADMIN_AUTH] PIN Master SALAH.");
        updateDisplay("PIN SALAH!", "Akses Ditolak");
        delay(1500);
        currentState = STATE_IDLE;
        updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
      }
    } else if (key == '*') {
      Serial.println("[STATE] Batal ADMIN_AUTH, kembali ke IDLE");
      currentState = STATE_IDLE;
      updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
    } else {
      inputBuffer += key;
      String stars = "";
      for(int i=0; i<inputBuffer.length(); i++) stars += "*";
      updateDisplay("MODE ADMIN", "Masukkan PIN Master:", stars);
    }
  }
}

void handleAdminState() {
  char key = readKeypad();
  
  if (key != 'N') {
    Serial.printf("[ADMIN] MenuState: %d, Key Pressed: %c\n", adminMenuState, key);
  }

  if (adminMenuState == 0) { // Main Admin Menu
    if (key == '1') {
      Serial.println("[ADMIN] Memilih menu Tambah FP");
      adminMenuState = 1;
      inputBuffer = "";
      updateDisplay("TAMBAH FP", "Masukkan ID (1-127)", "Lalu tekan #");
    } else if (key == '2') {
      Serial.println("[ADMIN] Memilih menu Hapus FP");
      adminMenuState = 3;
      inputBuffer = "";
      updateDisplay("HAPUS FP", "Masukkan ID (1-127)", "Lalu tekan #");
    } else if (key == '3' || key == '*') {
      Serial.println("[ADMIN] Keluar dari Admin Menu");
      currentState = STATE_IDLE;
      updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
    }
  } 
  else if (adminMenuState == 1) { // Input ID to Enroll
    if (key != 'N') {
      if (key == '#') {
        targetID = inputBuffer.toInt();
        Serial.printf("[ENROLL] Target ID yang dimasukkan: %d\n", targetID);
        if (targetID > 0 && targetID < 128) {
          adminMenuState = 2;
          enrollStep = 1;
          updateDisplay("ENROLL ID " + String(targetID), "Tempelkan Jari", "Sekarang...");
        } else {
          Serial.println("[ENROLL] ID tidak valid!");
          updateDisplay("ID TIDAK VALID!", "Harus 1-127");
          delay(1500);
          adminMenuState = 0;
          updateDisplay("ADMIN MENU", "1: Tambah FP", "2: Hapus FP", "3: Keluar");
        }
      } else if (key == '*') {
        adminMenuState = 0;
        updateDisplay("ADMIN MENU", "1: Tambah FP", "2: Hapus FP", "3: Keluar");
      } else {
        inputBuffer += key;
        updateDisplay("TAMBAH FP", "ID: " + inputBuffer, "Tekan # utk Lanjut");
      }
    }
  }
  else if (adminMenuState == 2) { // Process Enroll
    if (enrollStep == 1) {
      if (finger.getImage() == FINGERPRINT_OK) {
        Serial.println("[ENROLL] Step 1: Jari terdeteksi. Konversi gambar...");
        if (finger.image2Tz(1) == FINGERPRINT_OK) {
          Serial.println("[ENROLL] Step 1: Berhasil konversi ke Buffer 1. Angkat jari.");
          updateDisplay("Angkat Jari Anda");
          enrollStep = 2;
          delay(1000); // beri waktu pengguna untuk mengangkat jari
        }
      }
    } else if (enrollStep == 2) {
      if (finger.getImage() == FINGERPRINT_NOFINGER) {
        Serial.println("[ENROLL] Step 2: Jari telah diangkat. Minta tempel lagi.");
        updateDisplay("Tempelkan Lagi", "Jari Yang Sama");
        enrollStep = 3;
      }
    } else if (enrollStep == 3) {
      if (finger.getImage() == FINGERPRINT_OK) {
        Serial.println("[ENROLL] Step 3: Jari terdeteksi lagi. Konversi gambar...");
        if (finger.image2Tz(2) == FINGERPRINT_OK) {
          Serial.println("[ENROLL] Step 3: Berhasil konversi ke Buffer 2. Membuat model...");
          if (finger.createModel() == FINGERPRINT_OK) {
            Serial.println("[ENROLL] Step 3: Model berhasil dibuat. Menyimpan ke sensor...");
            if (finger.storeModel(targetID) == FINGERPRINT_OK) {
              Serial.printf("[ENROLL] SUKSES! ID %d berhasil disimpan.\n", targetID);
              updateDisplay("BERHASIL!", "ID " + String(targetID) + " Tersimpan");
            } else {
              Serial.println("[ENROLL] ERROR: Gagal menyimpan model ke flash memori sensor.");
              updateDisplay("GAGAL!", "Menyimpan ke Flash");
            }
          } else {
             Serial.println("[ENROLL] ERROR: Jari tidak cocok antara percobaan 1 dan 2.");
             updateDisplay("GAGAL!", "Jari Tidak Cocok");
          }
          delay(2000);
          adminMenuState = 0;
          updateDisplay("ADMIN MENU", "1: Tambah FP", "2: Hapus FP", "3: Keluar");
        }
      }
    }
    
    if (key == '*') {
      Serial.println("[ENROLL] Dibatalkan oleh pengguna.");
      adminMenuState = 0;
      updateDisplay("ADMIN MENU", "1: Tambah FP", "2: Hapus FP", "3: Keluar");
    }
  }
  else if (adminMenuState == 3) { // Input ID to Delete
    if (key != 'N') {
      if (key == '#') {
        targetID = inputBuffer.toInt();
        Serial.printf("[DELETE] Menghapus ID: %d\n", targetID);
        if (finger.deleteModel(targetID) == FINGERPRINT_OK) {
          Serial.println("[DELETE] SUKSES! ID berhasil dihapus.");
          updateDisplay("BERHASIL!", "ID " + String(targetID) + " Dihapus");
        } else {
          Serial.println("[DELETE] ERROR: ID tidak ditemukan di memori.");
          updateDisplay("GAGAL!", "ID Tidak Ditemukan");
        }
        delay(2000);
        adminMenuState = 0;
        updateDisplay("ADMIN MENU", "1: Tambah FP", "2: Hapus FP", "3: Keluar");
      } else if (key == '*') {
        adminMenuState = 0;
        updateDisplay("ADMIN MENU", "1: Tambah FP", "2: Hapus FP", "3: Keluar");
      } else {
        inputBuffer += key;
        updateDisplay("HAPUS FP", "ID: " + inputBuffer, "Tekan # utk Lanjut");
      }
    }
  }
}

// ==========================================
// 8. FUNGSI-FUNGSI BANTUAN
// ==========================================

void updateDisplay(String text1, String text2, String text3, String text4) {
  // Uncomment the line below if you want OLED debug on serial
  // Serial.println("[OLED] " + text1 + " | " + text2 + " | " + text3 + " | " + text4);
  
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

char readKeypad() {
  char currentKey = 'N'; 
  char pressedKey = 'N';

  if (keyPad.isPressed()) {
    currentKey = keyPad.getChar();
  }

  // Deteksi trigger saat tombol dilepas
  if (lastKey != 'N' && currentKey == 'N') {
    pressedKey = lastKey; 
  }

  lastKey = currentKey;
  delay(10); 
  
  return pressedKey;
}
