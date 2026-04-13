#ifndef TELEGRAM_SYS_H
#define TELEGRAM_SYS_H

#include <Arduino.h>

/**
 * Inisialisasi Task Telegram di Core 0.
 */
void initTelegram();

/**
 * Task internal FreeRTOS (Core 0).
 */
void telegramTask(void *pvParameters);

#endif // TELEGRAM_SYS_H
