# Rencana Pembuatan Firmware Utama - Brangkas V2

## 1. Tujuan dan Ruang Lingkup (Scope & Objective)
Dokumen ini berisi rencana arsitektur dan implementasi firmware untuk sistem brangkas pintar berbasis ESP32. Firmware ini akan menggabungkan kontrol akses ganda (Sidik Jari + Keypad), sistem alarm anti-maling, antarmuka pengguna OLED, integrasi kamera untuk bukti foto, dan notifikasi serta kontrol jarak jauh via Telegram.

## 2. Pemetaan Perangkat Keras (Hardware Mapping)
Berdasarkan kode awal yang ada di `main.ino`, berikut adalah pemetaan pin yang akan digunakan:

*   **ESP32 Core**: WROVER-KIT (memiliki PSRAM, penting untuk buffer kamera).
*   **Sensor Sidik Jari**: UART 2 (RX: 32, TX: 33)
*   **Keypad 4x4**: I2C (SDA: 13, SCL: 14) via modul PCF8574 (Alamat: 0x27)
*   **OLED 128x64**: I2C (SDA: 13, SCL: 14) (Alamat: 0x3C)
*   **Aktuator Utama**: Door Lock Relay / Solenoid (Pin: 0)
*   **Alarm System**:
    *   Limit Switch 1 (Deteksi Brangkas Diangkat): Pin 15 (Input Pullup)
    *   Limit Switch 2 (Deteksi Pintu Paksa Buka): Pin 2 (Input Pullup)
    *   Buzzer: Pin 12
*   **Kamera**: Menggunakan library ESP32 Camera (koneksi pin paralel standar ESP32-CAM/WROVER). *Catatan: Pinout spesifik kamera perlu disesuaikan.*
*   **Konektivitas**: WiFi + UniversalTelegramBot.

## 3. Arsitektur Firmware (State Machine)
Untuk mengakomodasi multi-tasking tanpa `delay()` yang menghambat sistem (blocking), firmware akan menggunakan **State Machine**.

**Daftar State Sistem:**
1.  **`STATE_IDLE`**: Menunggu input dari Fingerprint, Keypad, atau pesan Telegram. Layar menampilkan status / jam. Terus memantau Limit Switch.
2.  **`STATE_AUTH_FINGER`**: Menunggu jari ditempelkan (Step 1 dari 2FA).
3.  **`STATE_AUTH_PIN`**: Menunggu PIN Keypad dimasukkan setelah Fingerprint valid (Step 2 dari 2FA).
4.  **`STATE_UNLOCKED`**: Door lock terbuka. Menunggu pintu ditutup kembali atau timeout otomatis mengunci. Mengirim foto "Akses Berhasil" ke Telegram.
5.  **`STATE_ALARM`**: Buzzer menyala selama 20 detik. Mengambil foto dan mengirim peringatan "PERINGATAN! BRANGKAS DIANGKAT/DIBUKA PAKSA" ke Telegram. Mengunci semua akses fisik sementara sampai di-reset.
6.  **`STATE_ADMIN`**: Mode khusus untuk mendaftarkan (Enroll) atau menghapus (Delete) sidik jari. Diakses melalui master PIN khusus dari Keypad.

## 4. Rincian Fitur & Logika

### A. Alur Akses (Buka Brangkas)
1.  **Gagal Akses**: Jika sidik jari salah ATAU PIN salah sebanyak 3 kali, sistem mengambil foto pengguna, mengirimkannya ke Telegram dengan peringatan "Upaya Akses Ilegal", dan masuk ke mode *Cooldown* selama 1 menit (tidak bisa scan FP/Keypad).
2.  **Sukses Akses**: Jika Fingerprint COCOK dilanjutkan PIN BENAR, Door Lock aktif (LOW/HIGH sesuai relay), ambil foto, kirim ke Telegram "Brangkas Dibuka oleh ID #...", dan buka pintu.

### B. Sistem Alarm (Limit Switch)
*   Pemantauan (Polling) limit switch dilakukan setiap siklus `loop()`.
*   Jika **Switch 1 (Angkat)** terbuka dari posisi normalnya, trigger Alarm.
*   Jika **Switch 2 (Pintu Paksa)** terbuka sedangkan status sistem bukan `STATE_UNLOCKED`, trigger Alarm.
*   **Aksi Alarm**: Buzzer menyala 20 detik (menggunakan timer `millis()`, bukan `delay()` agar WiFi/Telegram tidak putus). Kirim notifikasi + foto ke Telegram. Setelah 20 detik, cek ulang limit switch. Jika masih terbuka, alarm bunyi lagi 20 detik.

### C. Manajemen Sidik Jari
*   **Aktivasi**: Pengguna menekan kombinasi khusus (misal: `*123456#`) di keypad.
*   **Menu OLED**:
    *   1: Tambah Sidik Jari
    *   2: Hapus Sidik Jari
    *   3: Keluar
*   **Tambah**: Meminta ID baru (lewat keypad), meminta jari ditempelkan 2x (sesuai standar Enroll).
*   **Hapus**: Meminta ID yang ingin dihapus (lewat keypad), lalu mengonfirmasi penghapusan.

### D. Kontrol Jarak Jauh (Telegram)
*   Library: `UniversalTelegramBot`.
*   Perintah (Commands) yang diizinkan untuk Admin:
    *   `/status` : Mengecek kondisi brangkas (Terkunci/Terbuka, status limit switch).
    *   `/buka` : Membuka paksa door lock (override) & mengambil foto.
    *   `/foto` : Meminta foto kondisi saat ini (surveillance).
    *   `/reset_alarm` : Mematikan bunyi buzzer jika sedang alarm.

## 5. Rencana Implementasi Bertahap (Phases)

*   **Fase 1: Framework & State Machine Dasar**
    Menyatukan komponen I2C (Keypad & OLED) dan UART (Fingerprint) ke dalam satu struktur `loop()` yang non-blocking menggunakan `millis()`.
*   **Fase 2: Logika Akses & Admin Mode**
    Membangun alur 2FA (Fingerprint -> PIN). Menambahkan menu Enroll/Delete Fingerprint.
*   **Fase 3: Alarm System**
    Mengintegrasikan pembacaan GPIO untuk Limit Switch dan menyalakan/mematikan Buzzer berbasis timer.
*   **Fase 4: Kamera & Konektivitas WiFi**
    Menginisialisasi modul kamera ESP32. Menambahkan `WiFiManager` (agar SSID/Pass bisa diubah tanpa flash ulang) dan inisialisasi koneksi jaringan.
*   **Fase 5: Integrasi Telegram API**
    Menggabungkan fungsi kirim foto (`bot.sendPhotoByBinary()`) dan bot polling (menerima pesan jarak jauh). Menghubungkan trigger dari Fase 2 & Fase 3 ke fungsi Telegram ini.

## 6. Kebutuhan Library & Dependensi
*   `Wire.h`, `HardwareSerial.h`, `WiFi.h`
*   `Adafruit_GFX`, `Adafruit_SSD1306`
*   `I2CKeyPad`
*   `Adafruit_Fingerprint`
*   `esp_camera.h` (Built-in di platform Espressif32)
*   `UniversalTelegramBot` (Serta `ArduinoJson` untuk parsing)
*   `WiFiManager`

---
Rencana ini dirancang untuk memastikan kestabilan dan keamanan Brangkas V2. Pendekatan State Machine dipilih agar fitur alarm, jaringan, dan antarmuka pengguna dapat berjalan secara bersamaan tanpa lag.