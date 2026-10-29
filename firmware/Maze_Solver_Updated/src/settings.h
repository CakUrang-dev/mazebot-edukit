#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

struct MotorSettings {
    uint8_t pwmKiri;
    uint8_t pwmKanan;
};

struct TCRTSettings {
    int putih;
    int hitam;
    int ambang;
    bool valid;
};

void settingsInit();
MotorSettings settingsGetMotor();
void settingsSaveMotor(uint8_t kiri, uint8_t kanan);

TCRTSettings settingsGetTCRT();
void settingsSaveTCRT(int putih, int hitam, int ambang, bool valid);
void settingsResetTCRT();

#endif // SETTINGS_H
