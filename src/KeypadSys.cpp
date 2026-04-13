#include "KeypadSys.h"
#include "Globals.h"

void initKeypad() {
  if (keyPad.begin()) {
    Serial.println("[DEBUG] Keypad berhasil diinisialisasi.");
    keyPad.loadKeyMap(keyMap);
  } else {
    Serial.println("[ERROR] Keypad gagal!");
  }
}

char readKeypad() {
  char currentKey = 'N'; 
  char pressedKey = 'N';

  if (keyPad.isPressed()) {
    currentKey = keyPad.getChar();
  }

  // Deteksi trigger saat tombol dilepas
  if (lastKey != 'N' && currentKey == 'N') {
    pressedKey = lastKey; 
  }

  lastKey = currentKey;
  delay(10); 
  
  return pressedKey;
}
