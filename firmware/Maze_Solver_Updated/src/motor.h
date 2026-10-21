#pragma once
#include <Arduino.h>

enum class MotorMode : uint8_t {
  STOP,
  UJI_KIRI,
  UJI_KANAN,
  MAJU,
  MUNDUR,
  BELOK_KIRI,
  BELOK_KANAN
};

void motorBegin();
void motorStop();

// Mengatur arah dan kecepatan (PWM 0-255 per roda).
void motorDrive(MotorMode mode, int pwmKiri, int pwmKanan);

// Panggil setiap loop: menjalankan jeda non-blocking dan fail-safe.
void motorUpdate();

MotorMode   motorMode();
const char* motorModeNama(MotorMode mode);
bool        motorParseMode(const char* teks, MotorMode& hasil);

int motorPwmKiri();
int motorPwmKanan();