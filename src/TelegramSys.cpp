#include "TelegramSys.h"
#include "Globals.h"
#include "Config.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

// Client & Bot
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// State untuk remote unlock via Telegram
bool isWaitingForPin = false;
String lastChatId = "";

// Forward declaration fungsi pembantu kirim foto
bool sendPhotoTelegram(String chatId);

void initTelegram() {
    // Jalankan task di Core 0 (PRO_CPU)
    // Stack diperbesar (12KB) karena kamera & HTTPS berat
    xTaskCreatePinnedToCore(
        telegramTask,
        "TelegramTask",
        12288,
        NULL,
        1,
        NULL,
        0
    );
    Serial.println("[TELEGRAM] Task diinisialisasi di Core 0.");
}

void handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = String(bot.messages[i].chat_id);
        String text = bot.messages[i].text;
        String from_name = bot.messages[i].from_name;

        Serial.printf("[TELEGRAM] Pesan dari %s: %s\n", from_name.c_str(), text.c_str());

        if (text == "/start") {
            String welcome = "Halo " + from_name + "!\n";
            welcome += "Selamat datang di Bot Kendali Brangkas V2.\n\n";
            welcome += "/buka - Buka brangkas jarak jauh\n";
            welcome += "/status - Cek status brangkas";
            bot.sendMessage(chat_id, welcome, "");
            lastChatId = chat_id; // Simpan chat ID untuk notifikasi nanti
        }

        if (text == "/buka") {
            isWaitingForPin = true;
            lastChatId = chat_id;
            bot.sendMessage(chat_id, "🔐 Perintah buka diterima. Silakan masukkan PIN brangkas:", "");
        } else if (isWaitingForPin) {
            if (text == USER_PIN || text == MASTER_PIN) {
                bot.sendMessage(chat_id, "✅ PIN BENAR! Membuka brangkas...", "");
                
                // Kirim perintah ke Core 1
                CommandType cmd = CMD_OPEN_RELAY;
                xQueueSend(commandQueue, &cmd, portMAX_DELAY);
                
                isWaitingForPin = false;
            } else {
                bot.sendMessage(chat_id, "❌ PIN SALAH. Akses dibatalkan.", "");
                isWaitingForPin = false;
            }
        }

        if (text == "/status") {
          bot.sendMessage(chat_id, "Sistem Aktif & Terhubung.", "");
        }
    }
}

// State untuk pengiriman binary (kamera)
static camera_fb_t * _current_fb = NULL;
static size_t _current_fb_index = 0;

bool _tgMoreDataAvailable() {
    return (_current_fb != NULL && _current_fb_index < _current_fb->len);
}

byte _tgGetNextByte() {
    if (_current_fb != NULL && _current_fb_index < _current_fb->len) {
        return _current_fb->buf[_current_fb_index++];
    }
    return 0;
}

bool sendPhotoTelegram(String chatId) {
    if (chatId == "") return false;

    _current_fb = esp_camera_fb_get();
    if(!_current_fb) {
        Serial.println("[TELEGRAM] Gagal ambil gambar!");
        return false;
    }

    _current_fb_index = 0;
    Serial.println("[TELEGRAM] Mengirim foto...");
    
    // Kirim binary menggunakan callback
    String response = bot.sendPhotoByBinary(chatId, "image/jpeg", _current_fb->len,
        _tgMoreDataAvailable,
        _tgGetNextByte,
        nullptr,
        nullptr
    );

    esp_camera_fb_return(_current_fb);
    _current_fb = NULL;

    if (response != "") {
        Serial.println("[TELEGRAM] Foto terkirim.");
        return true;
    } else {
        Serial.println("[TELEGRAM] Gagal kirim foto!");
        return false;
    }
}

void telegramTask(void *pvParameters) {
    client.setInsecure(); // Hindari masalah sertifikat Root CA
    
    unsigned long lastCheckTime = 0;
    SafeEvent event;

    while (true) {
        // 1. Cek Pesan Baru (Polling setiap 1 detik)
        if (millis() - lastCheckTime > 1000) {
            int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
            while (numNewMessages) {
                handleNewMessages(numNewMessages);
                numNewMessages = bot.getUpdates(bot.last_message_received + 1);
            }
            lastCheckTime = millis();
        }

        // 2. Cek Antrean Event dari Core 1
        if (xQueueReceive(eventQueue, &event, 0) == pdTRUE) {
            String targetChatId = CHAT_ID;
            
            if (targetChatId != "") {
                switch (event.type) {
                    case EVENT_AUTH_SUCCESS:
                        bot.sendMessage(targetChatId, "🔓 Brangkas Dibuka di Lokasi.", "");
                        sendPhotoTelegram(targetChatId);
                        break;
                    case EVENT_AUTH_FAILED:
                        bot.sendMessage(targetChatId, "⚠️ Peringatan: Upaya buka brangkas GAGAL!", "");
                        sendPhotoTelegram(targetChatId);
                        break;
                    case EVENT_ALARM:
                        bot.sendMessage(targetChatId, "🚨 BAHAYA: LIMIT SWITCH TERPICU (Brangkas dibuka paksa/diangkat)!", "");
                        sendPhotoTelegram(targetChatId);
                        break;
                }
            } else {
                Serial.println("[TELEGRAM] Event diterima tapi CHAT_ID belum dikonfigurasi.");
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // Yield ke OS
    }
}
