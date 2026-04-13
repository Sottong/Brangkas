#ifndef OTASYS_H
#define OTASYS_H

#include <Arduino.h>

/**
 * Mengecek apakah ada versi firmware baru di server.
 * Mengembalikan true jika update tersedia dan berhasil diunduh.
 * Parameter chatId digunakan untuk mengirim status ke Telegram.
 */
bool checkAndRunOTA(String chatId);

#endif // OTASYS_H
