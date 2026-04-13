#include "StateHandlers.h"
#include "Globals.h"
#include "DisplaySys.h"
#include "KeypadSys.h"
#include "FingerprintSys.h"

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
