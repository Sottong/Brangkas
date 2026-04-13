# Rencana Refactoring Kode Sistem Brangkas V2

Untuk mempermudah proses debugging dan modifikasi, kode tunggal yang panjang di `main.ino` akan dipecah menjadi beberapa modul terpisah berdasarkan fitur dan tanggung jawab masing-masing komponen. Pendekatan ini menggunakan file header (`.h`) dan source (`.cpp`).

## Struktur Modul yang Direncanakan

### 1. Modul Konfigurasi
* **File:** `Config.h`
* **Tanggung Jawab:** Menyimpan seluruh definisi hardware (pin, alamat I2C, resolusi layar), definisi konstanta sistem (interval cooldown, waktu alarm), dan PIN default.
* **Isi:** 
  - `#define` pin relay, limit switch, buzzer, pin kamera, SDA/SCL.
  - Alamat OLED dan Keypad.
  - Konstanta seperti `USER_PIN` dan `MASTER_PIN`.

### 2. Modul Variabel Global & State Machine
* **File:** `Globals.h` & `Globals.cpp`
* **Tanggung Jawab:** Menyimpan status (state) sistem saat ini dan variabel-variabel global yang perlu dibagikan antar modul.
* **Isi:** 
  - `enum SystemState`
  - Variabel eksternal seperti `currentState`, `failedAttempts`, `inCooldown`, dll.

### 3. Modul Komponen Periferal (Hardware Abstraction)
* **`Display.h` & `Display.cpp`**
  - Mengelola objek `Adafruit_SSD1306`.
  - Fungsi `initDisplay()`.
  - Fungsi `updateDisplay(...)`.
* **`KeypadSys.h` & `KeypadSys.cpp`**
  - Mengelola objek `I2CKeyPad`.
  - Fungsi `initKeypad()`.
  - Fungsi `readKeypad()`.
* **`FingerprintSys.h` & `FingerprintSys.cpp`**
  - Mengelola komunikasi Serial2 dan objek `Adafruit_Fingerprint`.
  - Fungsi `initFingerprint()`.
* **`CameraSys.h` & `CameraSys.cpp`**
  - Mengelola konfigurasi dan inisialisasi `esp_camera`.
  - Fungsi `initCamera()`.
* **`NetworkSys.h` & `NetworkSys.cpp`**
  - Mengelola koneksi WiFi menggunakan `WiFiManager`.
  - Fungsi `initWiFi()`.

### 4. Modul Logika Sistem / State Handlers
* **File:** `StateHandlers.h` & `StateHandlers.cpp`
* **Tanggung Jawab:** Berisi implementasi dari setiap *State* sistem agar fungsi main terbebas dari logika rumit.
* **Isi:**
  - `handleIdleState()`
  - `handleAuthFingerState()`
  - `handleAuthPinState()`
  - `handleAdminAuthState()`
  - `handleAdminState()`
  - `handleAlarmState()`

### 5. Main (Entry Point)
* **File:** `main.ino` (atau bisa diubah ke `main.cpp` jika menggunakan pure C++)
* **Tanggung Jawab:** Menggabungkan semua modul, mengatur urutan inisialisasi di `setup()`, dan menjalankan perulangan *state machine* di `loop()`.
* **Isi:**
  - `#include` ke semua modul `.h`.
  - Inisialisasi Relay, Limit Switch, dan Buzzer di `setup()`.
  - Memanggil fungsi inisialisasi masing-masing modul (`initDisplay()`, `initCamera()`, dll).
  - Looping deteksi limit switch dan *switch-case* state machine.

## Langkah-Langkah Eksekusi
1. Membuat file `Config.h` dan memindahkan semua makro `#define`.
2. Membuat `Globals.h/.cpp` untuk variabel dan objek global agar bisa diakses modul lain secara aman menggunakan `extern`.
3. Memecah fungsi komponen hardware ke modul-modul periferal.
4. Memindahkan fungsi logika *handler* ke `StateHandlers.cpp`.
5. Merapikan `main.ino` menjadi bersih dan minimalis.
6. Melakukan proses *build* / *verify* untuk memastikan semua modul terhubung dengan benar dan tidak ada *multiple definition error*.

Jika Anda setuju dengan rencana ini, kita dapat mulai mengimplementasikan pemecahan ke modul-modul tersebut!