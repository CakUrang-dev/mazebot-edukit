#include "settings.h"
#include "config.h"
#include <Preferences.h>

namespace {
Preferences s_pref;
uint8_t s_kiri  = PWM_DEFAULT;
uint8_t s_kanan = PWM_DEFAULT;

// Namespace dan key sama seperti versi .ino,
// jadi kalibrasi lama di flash tetap terbaca.
constexpr const char* NS    = "maze-motor";
constexpr const char* KEY_L = "left";
constexpr const char* KEY_R = "right";
}  // namespace

void settingsLoad() {
  s_pref.begin(NS, true);
  s_kiri  = s_pref.getUChar(KEY_L, PWM_DEFAULT);
  s_kanan = s_pref.getUChar(KEY_R, PWM_DEFAULT);
  s_pref.end();

  Serial.printf("PWM KIRI TERSIMPAN: %d\n", s_kiri);
  Serial.printf("PWM KANAN TERSIMPAN: %d\n", s_kanan);
}

void settingsSave(uint8_t kiri, uint8_t kanan) {
  if (kiri != s_kiri || kanan != s_kanan) {
    s_pref.begin(NS, false);
    s_pref.putUChar(KEY_L, kiri);
    s_pref.putUChar(KEY_R, kanan);
    s_pref.end();

    s_kiri  = kiri;
    s_kanan = kanan;
  }

  Serial.println("KALIBRASI DISIMPAN");
  Serial.printf("PWM KIRI  : %d\n", kiri);
  Serial.printf("PWM KANAN : %d\n", kanan);
}

uint8_t settingsPwmKiri()  { return s_kiri; }
uint8_t settingsPwmKanan() { return s_kanan; }