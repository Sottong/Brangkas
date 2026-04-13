#include "Globals.h"
#include <Wire.h>

// Objek Global
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
I2CKeyPad keyPad(KEYPAD_I2C_ADDR);
char keyMap[] = "123A456B789C*0#D";
char lastKey = 'N'; 

HardwareSerial mySerial(2); 
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Variabel Global
SystemState currentState = STATE_IDLE;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 1000;

unsigned long alarmStartTime = 0;

String inputBuffer = "";
int failedAttempts = 0;
unsigned long cooldownStartTime = 0;
bool inCooldown = false;

int adminMenuState = 0; // 0: Menu utama, 1: Enroll tunggu ID, 2: Enroll tunggu jari, 3: Delete tunggu ID
int targetID = 0;
int enrollStep = 0;
