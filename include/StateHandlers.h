#ifndef STATE_HANDLERS_H
#define STATE_HANDLERS_H

#include <Arduino.h>

void handleIdleState();
void handleAuthFingerState();
void handleAuthPinState();
void handleAdminAuthState();
void handleAdminState();
void handleAlarmState();

#endif
