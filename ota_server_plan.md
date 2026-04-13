# Planning: OTA Firmware Server

Server sederhana untuk hosting file firmware `.bin` dan metadata versi (`version.json`) yang akan diakses oleh ESP32 Brangkas saat melakukan OTA update.

## Arsitektur

```
ESP32 Brangkas                         Server (Node.js / Express)
─────────────                         ───────────────────────────
                                       /api/version    → GET version.json
GET /api/version ──────────────────▶   /api/firmware   → GET firmware .bin
  ↓ bandingkan versi                   /api/upload     → POST upload firmware baru
GET /api/firmware ─────────────────▶   /firmware/      → folder penyimpanan file .bin
  ↓ update & restart
```

## Teknologi yang Digunakan

| Komponen | Pilihan | Alasan |
|---|---|---|
| Runtime | **Node.js** | Ringan, cepup deploy |
| Framework | **Express.js** | Minimalis, cukup untuk serving file statis + API |
| Upload | **Multer** | Middleware upload file untuk Express |
| Database | **Tidak perlu** | Versi disimpan di `version.json` (file statis) |
| Deploy | VPS / Cloud yang sudah ada | Sesuai server online milik user |

## Struktur Folder Project

```
ota-server/
├── server.js              # Entry point Express server
├── package.json
├── firmware/              # Folder penyimpanan file .bin
│   └── (kosong, diisi saat upload)
├── version.json           # Metadata versi aktif
└── .env                   # Konfigurasi (PORT, AUTH_TOKEN)
```

## API Endpoints

### 1. `GET /api/version`
ESP32 memanggil endpoint ini untuk cek versi terbaru.

**Response:**
```json
{
  "version": "2.2.0",
  "url": "http://domainanda.com/api/firmware"
}
```

### 2. `GET /api/firmware`
ESP32 mengunduh file `.bin` firmware terbaru. Server mengirim file binary dengan header `Content-Length` (wajib untuk ESP32 `HTTPUpdate`).

**Response:** Binary stream file `.bin` dengan header:
```
Content-Type: application/octet-stream
Content-Length: 1234567
```

### 3. `POST /api/upload`
Endpoint untuk mengupload firmware baru dari PC developer. Menerima file `.bin` dan parameter `version`, lalu auto-update `version.json`.

**Request:** `multipart/form-data`
- `firmware` — file `.bin`
- `version` — string versi baru (misal `"2.2.0"`)
- Header `Authorization: Bearer <TOKEN>` — otorisasi sederhana

**Response:**
```json
{ "success": true, "message": "Firmware v2.2.0 uploaded.", "size": 1234567 }
```

## Keamanan

1. **Auth Token** — Endpoint `/api/upload` dilindungi token di header `Authorization`. Token disimpan di `.env`.
2. **Validasi file** — Hanya menerima file berekstensi `.bin` dan ukuran maksimal 2MB (sesuai slot partisi ESP32).
3. **HTTPS** — Disarankan pasang SSL (via reverse proxy Nginx/Caddy). Jika tidak, ESP32 sudah dikonfigurasi HTTP biasa.

## Alur Kerja Developer

```
1. Edit firmware di PlatformIO
2. Naikkan FIRMWARE_VERSION di Config.h (misal "2.1.0" → "2.2.0")
3. Jalankan: ./build_ota.sh
4. Upload ke server:
   curl -X POST http://domainanda.com/api/upload \
     -H "Authorization: Bearer TOKEN_ANDA" \
     -F "firmware=@ota_output/brangkas_v2.2.0.bin" \
     -F "version=2.2.0"
5. Kirim /update di Telegram → ESP32 auto-update
```

## Implementasi `server.js` (Gambaran)

```javascript
const express = require('express');
const multer = require('multer');
const fs = require('fs');
const path = require('path');
require('dotenv').config();

const app = express();
const PORT = process.env.PORT || 3000;
const AUTH_TOKEN = process.env.AUTH_TOKEN || 'ganti-dengan-token-rahasia';
const BASE_URL = process.env.BASE_URL || `http://localhost:${PORT}`;

const FIRMWARE_DIR = path.join(__dirname, 'firmware');
const VERSION_FILE = path.join(__dirname, 'version.json');

// GET /api/version — ESP32 cek versi
app.get('/api/version', (req, res) => {
    if (!fs.existsSync(VERSION_FILE)) {
        return res.status(404).json({ error: 'Belum ada firmware' });
    }
    res.sendFile(VERSION_FILE);
});

// GET /api/firmware — ESP32 download .bin
app.get('/api/firmware', (req, res) => {
    const version = JSON.parse(fs.readFileSync(VERSION_FILE));
    const filePath = path.join(FIRMWARE_DIR, version.filename);
    if (!fs.existsSync(filePath)) {
        return res.status(404).json({ error: 'File firmware tidak ditemukan' });
    }
    res.download(filePath);
});

// POST /api/upload — Developer upload firmware baru
const upload = multer({ dest: FIRMWARE_DIR, limits: { fileSize: 2 * 1024 * 1024 } });
app.post('/api/upload', (req, res) => {
    // Cek auth token
    if (req.headers.authorization !== `Bearer ${AUTH_TOKEN}`) {
        return res.status(401).json({ error: 'Unauthorized' });
    }
    // Handle upload via multer, simpan file, update version.json
    // ...
});

app.listen(PORT, () => console.log(`OTA Server running on port ${PORT}`));
```

## Langkah Implementasi

1. `mkdir ota-server && cd ota-server`
2. `npm init -y`
3. `npm install express multer dotenv`
4. Buat `server.js`, `version.json`, folder `firmware/`, dan `.env`
5. Jalankan: `node server.js`
6. Test dengan `curl`
7. Deploy ke VPS / server online

## Integrasi dengan ESP32

Setelah server jalan, update `Config.h` di project Brangkas:
```cpp
#define OTA_VERSION_URL "http://ip-server-anda:3000/api/version"
```
Dan update `BASE_URL` di `build_ota.sh`:
```bash
BASE_URL="http://ip-server-anda:3000/api"
```
