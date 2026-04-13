/**
 * SISTEM BRANGKAS V2 - MAIN ENTRY POINT
 * Deskripsi: Mengelola siklus setup dan perulangan utama sistem.
 * Modul Terpisah: Config, Globals, Peripherals, StateHandlers.
 */

#include "Config.h"
#include "Globals.h"
#include "DisplaySys.h"
#include "KeypadSys.h"
#include "FingerprintSys.h"
#include "CameraSys.h"
#include "NetworkSys.h"
#include "StateHandlers.h"
#include "TelegramSys.h"

// ==========================================
// 1. SETUP (Inisialisasi Sistem)
// ==========================================
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n\n===================================");
  Serial.println("  SISTEM BRANGKAS V2 - REFACTORED  ");
  Serial.println("===================================");

  // 0. Inisialisasi Antrean (Dual-Core)
  eventQueue = xQueueCreate(10, sizeof(SafeEvent));
  commandQueue = xQueueCreate(10, sizeof(CommandType));

  // Inisialisasi Pin Dasar
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Asumsi LOW = Terkunci
  
  pinMode(LIMIT_SWITCH_1_PIN, INPUT_PULLDOWN); 
  pinMode(LIMIT_SWITCH_2_PIN, INPUT_PULLDOWN);
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Inisialisasi Modul Periferal
  initDisplay();
  initKeypad();
  initFingerprint();

  delay(1000); // Stabilisasi arus
  
  initCamera();
  
  delay(1500); // Stabilisasi daya sebelum WiFi
  
  initWiFi();

  // Inisialisasi Task Telegram di Core 0
  initTelegram();

  delay(2000);
  currentState = STATE_IDLE;
  Serial.println("[STATE] Berpindah ke STATE_IDLE");
  updateDisplay("BRANGKAS V2", "Status: TERKUNCI", "Tempelkan Jari");
}

// ==========================================
// 2. MAIN LOOP (Perulangan Utama)
// ==========================================
void loop() {
  // --- Antrean Perintah Jarak Jauh (Telegram -> Core 1) ---
  CommandType cmd;
  if (xQueueReceive(commandQueue, &cmd, 0) == pdTRUE) {
    if (cmd == CMD_OPEN_RELAY) {
      Serial.println("[REMOTE] Perintah Buka dari Telegram.");
      currentState = STATE_UNLOCKED;
      digitalWrite(RELAY_PIN, HIGH);
    }
  }

  // --- A. PROTEKSI & ALARM ---
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

  // --- B. MANAJEMEN COOLDOWN ---
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
      return; // Tunggu cooldown selesai
    }
  }

  // --- C. STATE MACHINE HANDLERS ---
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


