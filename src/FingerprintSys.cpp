#include "FingerprintSys.h"
#include "Globals.h"

void initFingerprint() {
  mySerial.begin(57600, SERIAL_8N1, PIN_FP_RX, PIN_FP_TX);
  finger.begin(57600);
  if (!finger.verifyPassword()) {
    Serial.println("[ERROR] Fingerprint tidak terdeteksi!");
  } else {
    Serial.println("[DEBUG] Fingerprint terdeteksi dan siap.");
  }
}
