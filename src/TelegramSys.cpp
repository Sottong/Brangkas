#include "TelegramSys.h"
#include "Globals.h"
#include "Config.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include <time.h>

// Client & Bot
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// State untuk remote unlock via Telegram
bool isWaitingForPin = false;
String lastChatId = "";

// Forward declaration
String sendPhotoTelegram(String chatId);

void initTelegram() {
    // Jalankan task di Core 0 (PRO_CPU)
    // Stack diperbesar (16KB) karena raw HTTP + kamera berat
    xTaskCreatePinnedToCore(
        telegramTask,
        "TelegramTask",
        16384,
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
            welcome += "/foto - Ambil foto dari kamera\n";
            welcome += "/status - Cek status brangkas";
            bot.sendMessage(chat_id, welcome, "");
            lastChatId = chat_id;
        }

        if (text == "/buka") {
            isWaitingForPin = true;
            lastChatId = chat_id;
            bot.sendMessage(chat_id, "🔐 Perintah buka diterima. Silakan masukkan PIN brangkas:", "");
        } else if (isWaitingForPin) {
            if (text == USER_PIN || text == MASTER_PIN) {
                bot.sendMessage(chat_id, "✅ PIN BENAR! Membuka brangkas...", "");
                CommandType cmd = CMD_OPEN_RELAY;
                xQueueSend(commandQueue, &cmd, portMAX_DELAY);
                isWaitingForPin = false;
            } else {
                bot.sendMessage(chat_id, "❌ PIN SALAH. Akses dibatalkan.", "");
                isWaitingForPin = false;
            }
        }

        if (text == "/foto") {
            bot.sendMessage(chat_id, "📸 Mengambil foto...", "");
            sendPhotoTelegram(chat_id);
        }

        if (text == "/status") {
          bot.sendMessage(chat_id, "Sistem Aktif & Terhubung.", "");
        }
    }
}

// =================== KIRIM FOTO (RAW HTTP + CHUNK 1024) ===================
String sendPhotoTelegram(String chatId) {
    if (chatId == "") return "No chat ID";

    const char* myDomain = "api.telegram.org";
    String getAll = "";
    String getBody = "";

    // Buang frame pertama (kualitas jelek)
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (fb) esp_camera_fb_return(fb);

    // Ambil frame kedua (kualitas bagus)
    fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[TELEGRAM] Gagal ambil gambar!");
        return "Camera capture failed";
    }

    Serial.printf("[TELEGRAM] Foto diambil: %d bytes\n", fb->len);

    if (client.connect(myDomain, 443)) {
        Serial.println("[TELEGRAM] Mengirim foto (chunked)...");

        String head = "--ESP32Boundary\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chatId + "\r\n--ESP32Boundary\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
        String tail = "\r\n--ESP32Boundary--\r\n";

        size_t imageLen = fb->len;
        size_t totalLen = imageLen + head.length() + tail.length();

        client.println("POST /bot" + String(BOT_TOKEN) + "/sendPhoto HTTP/1.1");
        client.println("Host: " + String(myDomain));
        client.println("Content-Length: " + String(totalLen));
        client.println("Content-Type: multipart/form-data; boundary=ESP32Boundary");
        client.println();
        client.print(head);

        // Kirim data foto dalam chunk 1024 byte
        uint8_t *fbBuf = fb->buf;
        size_t fbLen = fb->len;
        for (size_t n = 0; n < fbLen; n = n + 1024) {
            if (n + 1024 < fbLen) {
                client.write(fbBuf, 1024);
                fbBuf += 1024;
            } else if (fbLen % 1024 > 0) {
                size_t remainder = fbLen % 1024;
                client.write(fbBuf, remainder);
            }
        }

        client.print(tail);
        esp_camera_fb_return(fb);

        // Baca respons dengan timeout 10 detik
        int waitTime = 10000;
        long startTimer = millis();
        boolean state = false;

        while ((startTimer + waitTime) > millis()) {
            delay(100);
            while (client.available()) {
                char c = client.read();
                if (state == true) getBody += String(c);
                if (c == '\n') {
                    if (getAll.length() == 0) state = true;
                    getAll = "";
                } else if (c != '\r')
                    getAll += String(c);
                startTimer = millis();
            }
            if (getBody.length() > 0) break;
        }
        client.stop();
        Serial.println("[TELEGRAM] Foto terkirim.");
    } else {
        esp_camera_fb_return(fb);
        getBody = "Connection to api.telegram.org failed.";
        Serial.println("[TELEGRAM] Koneksi ke server gagal.");
    }
    return getBody;
}

void telegramTask(void *pvParameters) {
    client.setInsecure();

    // Sinkronisasi Waktu NTP untuk SSL
    Serial.print("[NTP] Sync time: ");
    configTime(0, 0, "pool.ntp.org");
    time_t now = time(nullptr);
    while (now < 24 * 3600) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println(" OK");
    
    unsigned long lastCheckTime = 0;
    SafeEvent event;

    // Kirim pesan online saat boot
    bot.sendMessage(CHAT_ID, "🟢 Brangkas V2 Online.", "");

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
                        bot.sendMessage(targetChatId, "🚨 BAHAYA: LIMIT SWITCH TERPICU!", "");
                        sendPhotoTelegram(targetChatId);
                        break;
                }
            } else {
                Serial.println("[TELEGRAM] Event diterima tapi CHAT_ID belum dikonfigurasi.");
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
