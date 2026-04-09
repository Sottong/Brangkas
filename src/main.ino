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

// ==========================================
// 5. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n\n===================================");
  Serial.println("  SISTEM BRANGKAS V2 - FASE 2      ");
  Serial.println("===================================");

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

  delay(2000);
  currentState = STATE_IDLE;
  Serial.println("[STATE] Berpindah ke STATE_IDLE");
  updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
}

// ==========================================
// 6. MAIN LOOP (NON-BLOCKING)
// ==========================================
void loop() {
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
      Serial.println("[STATE] Kembali ke STATE_IDLE mengunci kembali.");
      currentState = STATE_IDLE;
      updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
      break;
    case STATE_ALARM:
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
