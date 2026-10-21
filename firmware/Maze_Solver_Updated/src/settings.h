#pragma once
#include <Arduino.h>

// Memuat kalibrasi PWM terakhir dari flash (NVS).
void settingsLoad();

// Menyimpan kalibrasi; menulis flash hanya jika nilainya berubah.
void settingsSave(uint8_t kiri, uint8_t kanan);

uint8_t settingsPwmKiri();
uint8_t settingsPwmKanan();