# Rencana Implementasi Fitur Telegram & Arsitektur Dual-Core (ESP32)

Dokumen ini adalah cetak biru (blueprint) untuk menambahkan integrasi Telegram pada sistem Brangkas V2, sekaligus memanfaatkan arsitektur dual-core ESP32 agar performa UI dan sensor tidak terhambat saat mengirim gambar.

## 1. Deskripsi Fitur Baru
- **Notifikasi Foto:** Bot Telegram mengirimkan foto wajah pengguna ketika ada upaya membuka pintu (baik berhasil maupun gagal).
- **Remote Unlock:** Bot Telegram menerima perintah `/buka`. ESP membalas menanyakan PIN. Jika PIN yang dimasukkan benar, brangkas akan langsung terbuka via jarak jauh.
- **Notifikasi Alarm:** Jika limit switch terpicu (brangkas dibuka paksa/diangkat), bot Telegram segera mengirimkan peringatan darurat beserta foto situasi saat itu.

## 2. Arsitektur Pembagian Core ESP32
Operasi kamera dan pengunggahan file ke server API Telegram (HTTPS) memakan waktu cukup lama (blocking 1-5 detik). Mengingat sistem ini berbasis *real-time* event (membaca Keypad & Limit Switch tak boleh tertunda), beban harus dipisah.

### Core 1 (APP_CPU) - *Hardware & Real-time State Machine*
Secara default, Arduino `loop()` berjalan di atas Core 1.
- **Tugas:** Menjalankan State Machine (`handleIdleState`, `handleAlarmState`, dll), membaca respon Keypad, memindai Fingerprint, memicu Buzzer, dan menampilkan teks ke layar OLED.
- **Komunikasi Keluar:** Jika mendeteksi Alarm atau Auth Selesai (Cocok/Tidak), Core 1 memicu / mengirimkan *event* pesan ke Core 0.
- **Komunikasi Masuk:** Mengawasi antrean pesan. Jika Core 0 mengirim sinyal "BUKA_PINTU", Core 1 mengaktifkan relay.

### Core 0 (PRO_CPU) - *Network, Camera Capture & Heavy Processing*
Core ini digunakan untuk menangani urusan internet menggunakan **FreeRTOS`.
- **Tugas:** Dijalankan dengan `xTaskCreatePinnedToCore`.
- **Polling Bot:** Rutin mengecek (polling) jika ada pesan obrolan baru masuk dari Telegram (misal pengguna mengetik `/buka`).
- **Kamera & Upload:** Menerima perintah dari Core 1 untuk mengirim foto. Prosesnya: `esp_camera_fb_get()` -> `HTTP POST Telegram` -> `esp_camera_fb_return()`. Karena berada di Core 0, `loop()` utama OLED di Core 1 tidak akan mengalami jeda/lag.

## 3. Komunikasi Antar Proses dengan FreeRTOS Queues
Sistem komunikasi yang aman antar-core adalah dengan **FreeRTOS Queue** (`xQueue`).
- `EventQueue` (Arah Core 1 => Core 0): Membawa struct sederhana untuk mengeksekusi Telegram bot.
  - Event tipe: `NOTIFY_AUTH_SUCCESS`, `NOTIFY_AUTH_FAILED`, `NOTIFY_ALARM`.
- `CommandQueue` (Arah Core 0 => Core 1): Mengirim instruksi dari internet ke hardware.
  - Tipe command: `CMD_OPEN_RELAY`.

## 4. Mekanisme "Remote Unlock" via Telegram
1. **User (Aplikasi Telegram):** Mengetik dan mengirim perintah `/buka`.
2. **Core 0 (ESP32):** Menerima pesan, kemudian membalas: `"Perintah diterima. Silakan masukkan PIN Master/User:"`. (Set variabel semacam `bool waitingForPin = true;`).
3. **User (Aplikasi Telegram):** Mengetik `1234` atau PIN lainnya.
4. **Core 0 (ESP32):** Mengambil pesan teks, membandingkannya dengan konstan `USER_PIN` atau `MASTER_PIN`.
   - **Jika Salah:** Membalas `"PIN Salah. Akses Ditolak."` dan me-reset `waitingForPin`.
   - **Jika Benar:** Membalas `"PIN Benar! Membuka brangkas..."`, mengirim sinyal `CMD_OPEN_RELAY` ke `CommandQueue`, dan me-reset `waitingForPin`.
5. **Core 1 (ESP32):** Menangkap antrean dari `CommandQueue` di awal fungsi `loop()`, merespons dengan mengubah status State Machine ke `STATE_UNLOCKED`. Pintu terbuka.

## 5. Instruksi Bertahap Untuk Developer / Model AI

### Tahap 1: Setup Bot & Dependensi
- Tambahkan `UniversalTelegramBot` dan `ArduinoJson` (v6) pada konfigurasi `platformio.ini`.
- Minta USER mendaftarkan **BotToken** dan **ChatID** Telegram untuk di letakkan dalam variabel global / `Config.h`.

### Tahap 2: Manajemen FreeRTOS di `Globals`
- Deklarasikan dua Queue global: `extern QueueHandle_t eventQueue` dan `extern QueueHandle_t commandQueue`.
- Daftarkan `struct` tipe data yang mudah dibaca sebagai format pesan payload antrean.

### Tahap 3: Pembuatan `TelegramSys` (Core 0 Task)
- Buat file `TelegramSys.h` dan `TelegramSys.cpp`.
- Dalam modul ini, siapkan task FreeRTOS dengan *infinite loop* (jangan lupa beri `vTaskDelay(10)` agar tidak memicu watchdog).
- Programkan logika Telegram API `getUpdates()` untuk fitur chat. Sertakan fungsi `sendPhoto` HTTP Client untuk menangkap *frame buffer* kamera tanpa meyimpannya ke SD Card (harus `multipart/form-data` stream).

### Tahap 4: Inisialisasi Task di `main.ino`
- Panggil `xQueueCreate(10, sizeof(YourEventStruct))` di `setup()`.
- Panggil `xTaskCreatePinnedToCore(...)` untuk menjalankan Task utama `TelegramSys` dan sandarkan di prosesor Core 0.

### Tahap 5: Pemanggilan Antrean di `StateHandlers`
- Perbarui `handleAuthFingerState()`, `handleAuthPinState()`, dan `handleAlarmState()`. Sisipkan rutin `xQueueSend(eventQueue, ...)` agar Core 1 segera mensinyalkan notifikasi ke aplikasi Telegram ketika terpicu.
- Begitu pula, tangkap `xQueueReceive(commandQueue, ...)` dari `loop()` utama di `main.ino` jika Telegram mengirim perintah rahasia pembuka gembok.
