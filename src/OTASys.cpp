#include "OTASys.h"
#include "Config.h"
#include "DisplaySys.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Referensi ke bot Telegram (didefinisikan di TelegramSys.cpp)
extern WiFiClientSecure client;
extern UniversalTelegramBot bot;

/**
 * Mengecek versi firmware dari server, dan jika lebih baru,
 * langsung mengunduh dan menerapkan update.
 */
bool checkAndRunOTA(String chatId) {
    Serial.println("[OTA] Memulai pengecekan update...");
    updateDisplay("OTA UPDATE", "Cek versi...");

    // 1. Ambil version.json dari server
    HTTPClient http;
    http.begin(OTA_VERSION_URL);
    int httpCode = http.GET();

    if (httpCode != 200) {
        Serial.printf("[OTA] Gagal cek versi, HTTP code: %d\n", httpCode);
        bot.sendMessage(chatId, "❌ Gagal menghubungi server update (HTTP " + String(httpCode) + ")", "");
        updateDisplay("OTA GAGAL", "Server error");
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // 2. Parse JSON: {"version": "x.y.z", "url": "http://...firmware.bin"}
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[OTA] JSON parse error: %s\n", err.c_str());
        bot.sendMessage(chatId, "❌ Format version.json tidak valid.", "");
        updateDisplay("OTA GAGAL", "JSON error");
        return false;
    }

    String serverVersion = doc["version"].as<String>();
    String firmwareUrl   = doc["url"].as<String>();

    Serial.printf("[OTA] Versi lokal: %s | Versi server: %s\n", FIRMWARE_VERSION, serverVersion.c_str());

    // 3. Bandingkan versi
    if (serverVersion == FIRMWARE_VERSION) {
        Serial.println("[OTA] Firmware sudah versi terbaru.");
        bot.sendMessage(chatId, "✅ Firmware sudah versi terbaru (v" + String(FIRMWARE_VERSION) + ").", "");
        updateDisplay("OTA", "Sudah terbaru", "v" + String(FIRMWARE_VERSION));
        return false;
    }

    // 4. Versi baru tersedia, mulai update
    Serial.printf("[OTA] Update tersedia! Mengunduh dari: %s\n", firmwareUrl.c_str());
    bot.sendMessage(chatId, 
        "🔄 Update tersedia!\n"
        "Versi lokal: v" + String(FIRMWARE_VERSION) + "\n"
        "Versi baru : v" + serverVersion + "\n\n"
        "Mengunduh firmware... Jangan matikan brangkas!", "");
    
    updateDisplay("OTA UPDATE", "Download...", "v" + serverVersion);

    // 5. Jalankan HTTP OTA Update
    WiFiClient updateClient;

    // Callback progress (opsional, untuk serial monitor)
    httpUpdate.onProgress([&chatId](int cur, int total) {
        int pct = (cur * 100) / total;
        if (pct % 25 == 0) { // Update display setiap 25%
            Serial.printf("[OTA] Progress: %d%%\n", pct);
            updateDisplay("OTA UPDATE", "Progress:", String(pct) + "%");
        }
    });

    t_httpUpdate_return ret = httpUpdate.update(updateClient, firmwareUrl);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("[OTA] Update GAGAL! Error (%d): %s\n",
                httpUpdate.getLastError(),
                httpUpdate.getLastErrorString().c_str());
            bot.sendMessage(chatId, "❌ Update gagal: " + httpUpdate.getLastErrorString(), "");
            updateDisplay("OTA GAGAL", httpUpdate.getLastErrorString());
            return false;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[OTA] Tidak ada update.");
            bot.sendMessage(chatId, "ℹ️ Server mengatakan tidak ada update.", "");
            return false;

        case HTTP_UPDATE_OK:
            Serial.println("[OTA] Update berhasil! Restarting...");
            bot.sendMessage(chatId, "✅ Update berhasil ke v" + serverVersion + "! Brangkas akan restart...", "");
            updateDisplay("OTA SUKSES!", "Restarting...");
            delay(1000);
            ESP.restart();
            return true; // Tidak akan sampai sini
    }

    return false;
}
