#include "NetworkSys.h"
#include "Globals.h"
#include "DisplaySys.h"
#include <WiFiManager.h>

void initWiFi() {
  WiFiManager wm;
  Serial.println("[WIFI] Menghubungkan ke WiFi...");
  updateDisplay("WIFI SETUP", "Menghubungkan...", "AP: Brangkas_AP", "Pass: 12345678");
  
  bool res = wm.autoConnect("Brangkas_AP", "12345678"); 
  
  if(!res) {
    Serial.println("[ERROR] Gagal terhubung ke WiFi.");
    updateDisplay("WIFI GAGAL", "Silakan Restart");
  } else {
    Serial.println("[WIFI] Berhasil terhubung ke WiFi!");
    updateDisplay("WIFI TERHUBUNG", WiFi.localIP().toString());
    delay(2000);
  }
}
