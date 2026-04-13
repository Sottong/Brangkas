#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2CKeyPad.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "Config.h"

// Enum Komunikasi Antar Core
enum EventType { 
  EVENT_AUTH_SUCCESS, 
  EVENT_AUTH_FAILED, 
  EVENT_ALARM 
};

struct SafeEvent {
  EventType type;
  int data; // ID sidik jari atau data tambahan
};

enum CommandType {
  CMD_NONE,
  CMD_OPEN_RELAY
};
enum SystemState {
  STATE_IDLE,
  STATE_AUTH_FINGER,
  STATE_AUTH_PIN,
  STATE_UNLOCKED,
  STATE_ALARM,
  STATE_ADMIN,
  STATE_ADMIN_AUTH
};

// Objek Global
extern Adafruit_SSD1306 display;
extern I2CKeyPad keyPad;
extern char keyMap[];
extern char lastKey;

extern HardwareSerial mySerial;
extern Adafruit_Fingerprint finger;

// Variabel Global
extern SystemState currentState;
extern unsigned long lastDisplayUpdate;
extern const unsigned long DISPLAY_INTERVAL;

extern unsigned long alarmStartTime;

extern String inputBuffer;
extern int failedAttempts;
extern unsigned long cooldownStartTime;
extern bool inCooldown;

extern int adminMenuState;
extern int targetID;
extern int enrollStep;

// Queue Handles (Komunikasi Dual-Core)
extern QueueHandle_t eventQueue;      // Core 1 -> Core 0 (Notify Telegram)
extern QueueHandle_t commandQueue;    // Core 0 -> Core 1 (Remote Action)

#endif // GLOBALS_H
