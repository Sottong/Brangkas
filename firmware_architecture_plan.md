# Arsitektur OTA (Over-The-Air) Firmware Update ESP32

Dokumen ini merangkum rencana arsitektur dan alur kerja (workflow) untuk mengimplementasikan fitur pembaruan firmware (OTA) secara online pada sistem Brangkas menggunakan ESP32.

## 1. Konsep Dasar
ESP32 akan mengambil file firmware (`.bin`) langsung dari server web (HTTP/HTTPS) yang sudah Anda siapkan. Proses ini memanfaatkan partisi OTA pada flash memory ESP32, di mana firmware baru akan diunduh ke partisi pasif (OTA_1), divalidasi, dan jika sukses, ESP32 akan di-reboot untuk menggunakan firmware baru tersebut.

## 2. Kebutuhan Server (Hosting Firmware)
Agar sistem berjalan optimal dan tidak boros bandwidth, server harus menyediakan dua path/endpoint:
1. **Endpoint Versi (`/version.json` atau `/version.txt`)**: 
   Mengembalikan informasi versi rilis terbaru.
   *Contoh respon:* `{"version": "1.0.1", "firmware_url": "http://domainanda.com/firmware/brangkas_v1.0.1.bin"}`
2. **Endpoint File Firmware**: 
   Menyediakan file `.bin` yang di-compile dari PlatformIO (.pio/build/esp-wrover-kit/firmware.bin) melalui jalur HTTP/HTTPS.

*Catatan: Pastikan server mensupport HTTP Header `Content-Length` agar ESP32 bisa mengetahui ukuran file secara pasti sebelum memulai update.*

## 3. Alur Kerja (Workflow) pada ESP32

### Trigger Update
Terdapat dua cara untuk memicu (trigger) pengecekan dan update OTA:
1. **Manual via Telegram (Direkomendasikan)**: Menambahkan _command_ `/update` di Telegram bot. Sistem brangkas hanya akan mengecek dan mengunduh firmware ketika pemilik memerintahkan.
2. **Otomatis pada Boot**: Setiap ESP32 dinyalakan ulang, ia akan mengecek versi di server.

### Proses Eksekusi Update
1. ESP32 membuat HTTP GET ke `http://domainanda.com/version.json`.
2. ESP32 membandingkan versi server dengan versi yang *hardcoded* di source code (misal: `const String FIRMWARE_VERSION = "1.0.0";`).
3. Jika versi server lebih baru, ESP32 mendownload firmware (`.bin`) menggunakan library `Update.h` atau `HTTPUpdate.h`.
4. Selama proses update, tampilan LCD/OLED menampilkan "Updating... X%".
5. Setelah unduhan selesai 100% dan lolos verifikasi _checksum_, sistem memanggil `ESP.restart()`.

## 4. Modifikasi Kode yang Diperlukan (Implementation Plan)

### A. Modifikasi `platformio.ini`
Kita perlu mengubah skema partisi (Partition Scheme) agar memiliki ruang untuk 2 aplikasi (App0 dan App1 untuk proses OTA). 
Tambahkan baris berikut di konfigurasi platformio environment Anda (jika belum ada):
```ini
; Menggunakan skema partition yang mendukung OTA (App = 2 x 1.9MB)
board_build.partitions = min_spiffs.csv 
```

### B. Membuat Modul Baru `src/OTASys.h` & `src/OTASys.cpp`
Modul ini akan bertanggung jawab spesifik terhadap:
- Mengecek versi firmware dari eksternal.
- Mengeksekusi proses HTTP OTA Update.
- Memberi callback progress (untuk di-update ke display).

### C. Modifikasi `src/TelegramSys.cpp`
Menambahkan handler untuk perintah `/update`:
```cpp
if (text == "/update") {
    bot.sendMessage(chat_id, "Memulai pengecekan update firmware...", "");
    // Taruh logic enqueue perintah ke Main Update Task
}
```

### D. Modifikasi `src/Globals.h` / `src/Config.h`
Menambahkan konstanta versi dan URL server:
```cpp
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_VERSION_URL "http://serveranda.com/brangkas/version.json"
```

## 5. Keamanan (Security Consideration)
1. **Gunakan koneksi HTTPS** jika memungkinkan (membutuhkan Root CA Certificate ditaruh di dalam code).
2. Jika menggunakan HTTP biasa, ada risiko Man-in-the-Middle namun secara operasional lebih ringan terhadap resource ESP32. Setidaknya pastikan server tidak bisa ditebak mudah strukturnya, atau tambahkan simple token otentikasi di HTTP Headers (contoh: `Authorization: Bearer <TOKEN>`).

## Kesimpulan Langkah Selanjutnya
1. Pastikan server sudah siap menampung HTTP request untuk file statis `.bin` dan `.json`.
2. Jika Anda Setuju dengan plan ini, kita akan mulai mengimplementasikan `OTASys.cpp` dan mengintegrasikannya ke proses `TelegramSys`.
