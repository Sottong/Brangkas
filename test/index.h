#ifndef INDEX_H
#define INDEX_H

// Core ESP32 & System
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "FS.h"
#include "SPIFFS.h"

// Hardware-related
#include <Ultrasonic.h>
#include "Wire.h"
#include "I2CKeyPad.h"
#include <LiquidCrystal_I2C.h>

// Connectivity & Web
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

#include <WiFiManager.h> 
#include "BasicOTA.hpp"

// ESP32-CAM (OV2640)
#define PWDN_GPIO_NUM        -1
#define RESET_GPIO_NUM       -1
#define XCLK_GPIO_NUM        21
#define SIOD_GPIO_NUM        26
#define SIOC_GPIO_NUM        27
#define Y9_GPIO_NUM          35
#define Y8_GPIO_NUM          34
#define Y7_GPIO_NUM          39
#define Y6_GPIO_NUM          36
#define Y5_GPIO_NUM          19
#define Y4_GPIO_NUM          18
#define Y3_GPIO_NUM          5
#define Y2_GPIO_NUM          4
#define VSYNC_GPIO_NUM       25
#define HREF_GPIO_NUM        23
#define PCLK_GPIO_NUM        22

// I2C Devices (LCD & Keypad)
#define PIN_I2C_SDA          32
#define PIN_I2C_SCL          15

// Sensors and Actuators
#define PIN_US_TRIG          0
#define PIN_US_ECHO          33
#define PIN_BUZZER           2
#define PIN_RELAY_LED        13
#define PIN_RELAY_SOLENOID   14
#define PIN_LIMIT_SWITCH     12

// =================================================================
//                      CONFIGURATION CONSTANTS
// =================================================================
extern int door_open_treshold ;
extern int door_close_treshold ;
extern unsigned long readInterval;

// =================================================================
//                      TELEGRAM & BLYNK CONFIG
// =================================================================
extern String BOTtoken;
extern String CHAT_ID;
#define BLYNK_PRINT  Serial
#define BLYNK_TEMPLATE_ID "TMPL6TuB3NE1b"
#define BLYNK_TEMPLATE_NAME "Brangkas"
#define BLYNK_AUTH_TOKEN "nUBqSsrL_ekJyr_f36N_MMNLBk5PW9wQ"

extern uint8_t KEYPAD_ADDRESS;
extern char keymap[19];
extern String password;
extern String resetconfig;


typedef enum {
  EVT_NONE,
  EVT_INIT_COMPLETE,
  EVT_ALRM_SWITCH_TRIGGERED,
  EVT_PASSWORD_CORRECT,
  EVT_SAVE_DOOR_OPEN,
  EVT_SAVE_DOOR_CLOSE,
  EVT_CTRL_UNLOCK_BLYNK
} br_event_t;

typedef enum {
  ST_INIT,
  ST_DOOR_CLOSE,
  ST_DOOR_OPEN,
  ST_SAFE_UNLOCK,
  ST_ALRM
} br_state_t;

extern br_state_t br_state;
extern Ultrasonic ultrasonic;
extern I2CKeyPad keyPad;
extern br_state_t br_state;
extern WiFiManager wm;
extern WiFiManagerParameter custom_field;
extern TaskHandle_t Task1;

// =================================================================
//                      SYSTEM FLAGS & VARIABLES
// =================================================================


extern bool fl_is_unlock;
extern bool fl_is_alarm;
extern bool fl_init_door_check;
extern bool fl_init_limit_switch;
extern bool unlock_timer_active;
extern bool fl_wrong_password;
extern unsigned long wrong_password_start;
extern bool fl_send_photo;

extern String input_password;
extern int distance;

extern bool fl_is_release;
extern char ch_now;
extern char on_release;

extern LiquidCrystal_I2C lcd;

extern String ssid;
extern String pass;

// =================================================================
//                      FUNCTION PROTOTYPES
// =================================================================

// --- Initialization Functions ---
void brangkas_init();
void configInitCamera();
void gpio_init(); // This function was defined but not prototyped. Added for consistency.
void lcd_init();
void keypad_init();
void wifi_init();
void blynk_init();

// --- Actuator & Sensor Control ---
void LED_ON();
void LED_OFF();
void SOLENOID_ON();
void SOLENOID_OFF();
void BUZZER_ON();
void BUZZER_OFF();

// --- Main Logic & Handlers ---
void keypad_run();
void distance_loop();
void buzzer_alrm();

// --- Communication (Blynk & Telegram) ---
void send_status_pintu(bool status);
void handleNewMessages(int numNewMessages);
String sendPhotoTelegram();
void saveParamCallback();

// --- RTOS Task ---
void Task1code( void * pvParameters );

// --- State Machine ---
void handle_event(br_event_t event);
void enter_state(br_state_t new_state);

void setClientTCP();

// lcd functions
void lcd1();
void lcd2();
void lcd3();
void lcd4();
void lcd5();
void lcd6();
void lcd7();
void lcd8();

//
void state_init();
void state_door_close();
void state_door_open();
void state_safe_unlock();


#endif // INDEX_H