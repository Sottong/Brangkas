#include <Arduino.h>
#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>

// Definisi Pin UART Fingerprint (Serial2 ESP32)
#define PIN_FP_RX 32
#define PIN_FP_TX 33

// Objek Komunikasi Serial untuk Fingerprint
HardwareSerial mySerial(2); 

// Objek Fingerprint Sensor
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(115200);
  while (!Serial); // Tunggu sampai Serial Monitor siap
  delay(100);
  
  Serial.println("\n\nMemulai Test Fingerprint Sensor...");

  // Inisialisasi Serial2 untuk Fingerprint (Baudrate default modul biasanya 57600)
  mySerial.begin(57600, SERIAL_8N1, PIN_FP_RX, PIN_FP_TX);

  // Inisialisasi library fingerprint
  finger.begin(57600);

  // Cek apakah sensor terdeteksi
  if (finger.verifyPassword()) {
    Serial.println("Sensor Fingerprint DITEMUKAN!");
  } else {
    Serial.println("Sensor Fingerprint TIDAK terdeteksi :(");
    Serial.println("Periksa koneksi kabel (RX->33, TX->32) atau power sensor.");
    while (1) { delay(1); } // Berhenti jika sensor tidak ditemukan
  }

  // Baca parameter sensor
  Serial.println("\nMembaca parameter sensor...");
  finger.getParameters();
  Serial.print("Status: 0x"); Serial.println(finger.status_reg, HEX);
  Serial.print("Sys ID: 0x"); Serial.println(finger.system_id, HEX);
  Serial.print("Kapasitas: "); Serial.println(finger.capacity);
  Serial.print("Security level: "); Serial.println(finger.security_level);
  Serial.print("Device address: "); Serial.println(finger.device_addr, HEX);
  Serial.print("Packet len: "); Serial.println(finger.packet_len);
  Serial.print("Baud rate: "); Serial.println(finger.baud_rate);

  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    Serial.print("Sensor tidak memiliki data sidik jari. Silakan enroll terlebih dahulu.");
  } else {
    Serial.println("Terdapat " + String(finger.templateCount) + " data sidik jari.");
  }
  
  Serial.println("\n--- Sistem Siap ---");
  Serial.println("Silakan tempelkan jari Anda ke sensor...");
}

// Fungsi untuk membaca sidik jari
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    Serial.println("Sidik jari tidak cocok / belum terdaftar!");
    return -1;
  }
  
  // Jika cocok, print ID dan confidence
  Serial.print("Ditemukan ID #"); Serial.print(finger.fingerID); 
  Serial.print(" dengan tingkat kecocokan (confidence) "); Serial.println(finger.confidence);
  
  return finger.fingerID; 
}

void loop() {
  getFingerprintIDez();
  delay(50); // Delay kecil agar pembacaan tidak terlalu cepat
}
