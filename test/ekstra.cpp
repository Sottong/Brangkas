#include "index.h"

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

// ============================ TASK UNTUK KIRIM FOTO ============================
void configInitCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

 // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  // Dapatkan handler sensor kamera
  sensor_t * s = esp_camera_sensor_get();

  // Atur orientasi
  s->set_hmirror(s, 1); // Flip horizontal (1: aktif, 0: nonaktif)
  s->set_vflip(s, 0);   // Flip vertical (1: aktif, 0: nonaktif)
}

void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);
    
    String from_name = bot.messages[i].from_name;
    if (text == "/start") {
      String welcome = "Welcome , " + from_name + "\n";
      welcome += "Use the following commands to interact with the ESP32-CAM \n";
      welcome += "/photo : takes a new photo\n";
      welcome += "/flash : toggles flash LED \n";
      bot.sendMessage(CHAT_ID, welcome, "");
    }
    if (text == "/flash") {
      // flashState = !flashState;
      // digitalWrite(FLASH_LED_PIN, flashState);
      Serial.println("Change flash LED state");
    }
    if (text == "/photo") {
      fl_send_photo = true;
      Serial.println("New photo request");
    }
  }
}

String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  //Dispose first picture because of bad quality
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb); // dispose the buffered image
  
  // Take a new photo
  fb = NULL;  
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  
  Serial.println("Connect to " + String(myDomain));


  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + CHAT_ID + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    size_t imageLen = fb->len;
    size_t extraLen = head.length() + tail.length();
    size_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  return getBody;
}

void setClientTCP(){
    clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
}

void LED_ON(){ digitalWrite(PIN_RELAY_LED, HIGH);}
void LED_OFF(){digitalWrite(PIN_RELAY_LED, LOW);}
void SOLENOID_ON(){digitalWrite(PIN_RELAY_SOLENOID, LOW);}
void SOLENOID_OFF(){digitalWrite(PIN_RELAY_SOLENOID, HIGH);}
void BUZZER_ON(){digitalWrite(PIN_BUZZER, HIGH);}
void BUZZER_OFF(){digitalWrite(PIN_BUZZER, LOW);}

// ============================= EVENT HANDLER =============================
void handle_event(br_event_t event){
  switch(br_state){
    case ST_INIT:
      if(event == EVT_INIT_COMPLETE){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Initialization");
        lcd.setCursor(0, 1);
        lcd.print("Complete!"); 
        delay(2000);
        lcd.clear();
        enter_state(ST_DOOR_CLOSE);
      }
      break;
    case ST_DOOR_CLOSE:
      if(event == EVT_PASSWORD_CORRECT){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Password Correct!");
        lcd.setCursor(0, 1);
        lcd.print("Unlocking...");
        fl_send_photo = true;
        SOLENOID_ON();
        unlock_timer_active = false; // reset timer
        enter_state(ST_SAFE_UNLOCK);
      }
      else if(event == EVT_CTRL_UNLOCK_BLYNK){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Control Unlock");
        lcd.setCursor(0, 1);
        lcd.print("from Blynk");
        SOLENOID_ON();
        unlock_timer_active = false; // reset timer
        enter_state(ST_SAFE_UNLOCK);
      }
      break;
    case ST_DOOR_OPEN:
      if(event == EVT_SAVE_DOOR_CLOSE){
        enter_state(ST_DOOR_CLOSE);
      }
      
      break;
    case ST_ALRM:
      
      break;
    case ST_SAFE_UNLOCK:
      if(event == EVT_SAVE_DOOR_OPEN){
        send_status_pintu(true);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Door Opened!");
        enter_state(ST_DOOR_OPEN);
      }
      else if(event == EVT_SAVE_DOOR_CLOSE){
        enter_state(ST_DOOR_CLOSE);
      }
      break;
    default:
      Serial.println("Unhandled state!");
      break;
  }
}

// // ============================= STATE MACHINE =============================
void enter_state(br_state_t new_state){
  br_state = new_state;

  switch(br_state){
    case ST_INIT:
      Serial.println("Entering INIT state");
      brangkas_init();
      
      break;
    case ST_DOOR_CLOSE:
      Serial.println("Entering DOOR CLOSE state");
      send_status_pintu(false);
      // Door close actions
      lcd.setCursor(0, 0);
      lcd.print("Input Password:");

      break;
    case ST_DOOR_OPEN:
      Serial.println("Entering DOOR OPEN state");
      // Door open actions
      break;

    case ST_ALRM:
      Serial.println("Entering ALARM state");
      // Alarm actions
      break;

    case ST_SAFE_UNLOCK:
      Serial.println("Entering UNLOCK state");
      // Unlock actions
      break;

    default:
      Serial.println("Unknown state!");
      break;
  }
}



String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
}

void gpio_init(){
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RELAY_LED, OUTPUT);
  pinMode(PIN_RELAY_SOLENOID, OUTPUT);
  pinMode(PIN_LIMIT_SWITCH, INPUT);

  LED_OFF();
  SOLENOID_OFF();
  BUZZER_OFF();
}


void keypad_init(){
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  if (keyPad.begin() == false)
  {
    Serial.println("\nERROR: cannot communicate to keypad.\n");

    // MASUK STATE INIT FAIL
  }
  keyPad.loadKeyMap(keymap);
}

void wifi_init(){
  unsigned long start_time = millis();
  const unsigned long timeout = 60000; // 1 menit

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if (millis() - start_time > timeout) {
      Serial.println("\nWiFi connection timeout!");
      break;
    }
    delay(500);
  }
}

void brangkas_init(){
  // Tambahkan inisialisasi lain jika diperlukan
  
  blynk_init();
  lcd_init();

  if (psramFound()) {
      Serial.println("PSRAM ditemukan!");
      Serial.printf("Ukuran PSRAM: %u bytes\n", ESP.getPsramSize());
  } else {
      Serial.println("PSRAM tidak terdeteksi!");
  }
  configInitCamera();

  lcd6();
  setClientTCP();
  delay(2000);
  keypad_init();
  gpio_init();
  distance = ultrasonic.read();

  lcd.setCursor(0, 0);
  lcd.print("Door state check");
  delay(2000);
  if(distance >= door_open_treshold){
    Serial.println("Door is open during init!");
    lcd7();
    return;
  }
}

// ==================== STATE FUNCTIONS ====================

void state_init() {
  if(!fl_init_door_check){
    if(distance >= door_open_treshold){
      return;
    } else {
      lcd1();
      fl_init_door_check = true;
    }
  }

  if(fl_init_limit_switch == false){
    bool limit_switch_state = digitalRead(PIN_LIMIT_SWITCH);
    if(limit_switch_state == LOW){
      lcd2();
      return;
    } else {
      lcd3();
      delay(2000);
      fl_init_limit_switch = true;
    }
  }
  if(fl_init_door_check && fl_init_limit_switch){
    handle_event(EVT_INIT_COMPLETE);
  }
}

void state_door_close() {
  static size_t last_len = 0;
  if(!fl_is_alarm){
    if (input_password.length() < last_len || input_password.length() == 0) {
      lcd.setCursor(0, 1);
      lcd.print("                "); // Clear previous input
      lcd.setCursor(0, 1);
    }
    if (input_password.length() != last_len) {
      lcd.setCursor(0, 1);
      for(size_t i = 0; i < input_password.length(); i++){
        lcd.print("*");
      }
      last_len = input_password.length();
    }
  }
  // Cek kondisi switch alrm
  if(digitalRead(PIN_LIMIT_SWITCH) == LOW || distance >= door_open_treshold){
    Serial.println("Limit switch triggered! Entering ALARM state.");
    fl_is_alarm = true;
    fl_send_photo = true;
  }

  // wrong password handler
  if (fl_wrong_password) {
    if (millis() - wrong_password_start >= 3000) {
      BUZZER_OFF();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Input Password:");
      fl_wrong_password = false;
    }
  }
}

void state_safe_unlock() {
  static unsigned long unlock_start_time = 0;
  if (!unlock_timer_active) {
    Serial.println("Starting unlock timer...");
    unlock_start_time = millis();
    unlock_timer_active = true;
  }
  if (distance > door_open_treshold) {
    Serial.println("Door opened detected!");
    LED_ON();
    handle_event(EVT_SAVE_DOOR_OPEN);
  } else if(millis() - unlock_start_time >= 10000) {
    Serial.printf("millis(): %lu, unlock_start_time: %lu\n", millis(), unlock_start_time);
    Serial.printf("unlock_timer_active: %d\n", unlock_timer_active);
    unlock_timer_active = false;
    SOLENOID_OFF();
    LED_OFF();
    Serial.println("Unlock timeout, returning to DOOR CLOSE state.");
    lcd4();
    delay(2000);
    handle_event(EVT_SAVE_DOOR_CLOSE);
  }
}

void state_door_open() {
  if (distance <= door_close_treshold) {
    SOLENOID_OFF();
    LED_OFF();
    lcd5();
    delay(2000);
    handle_event(EVT_SAVE_DOOR_CLOSE);
  }
}