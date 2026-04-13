#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <I2CKeyPad.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include "Config.h"

// Enum State Machine
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

#endif // GLOBALS_H
