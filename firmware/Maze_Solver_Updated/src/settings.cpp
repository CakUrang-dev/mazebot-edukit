#include "settings.h"
#include "config.h"
#include <Preferences.h>

static Preferences prefs;
static MotorSettings currentMotor = { DEFAULT_PWM_KIRI, DEFAULT_PWM_KANAN };
static TCRTSettings currentTCRT = { 0, 0, 0, false };

void settingsInit() {
    // 1. Muat pengaturan Motor
    if (prefs.begin(NVS_MOTOR_NS, false)) {
        currentMotor.pwmKiri = prefs.getUChar("kiri", DEFAULT_PWM_KIRI);
        currentMotor.pwmKanan = prefs.getUChar("kanan", DEFAULT_PWM_KANAN);
        prefs.end();
    }

    // 2. Muat pengaturan TCRT5000
    if (prefs.begin(NVS_TCRT_NS, false)) {
        currentTCRT.putih = prefs.getInt("putih", 0);
        currentTCRT.hitam = prefs.getInt("hitam", 0);
        currentTCRT.ambang = prefs.getInt("ambang", 0);
        currentTCRT.valid = prefs.getBool("valid", false);
        prefs.end();
    }

    Serial.printf("[SETTINGS] Motor L=%d R=%d | TCRT P=%d H=%d Ambang=%d Valid=%d\n",
                  currentMotor.pwmKiri, currentMotor.pwmKanan,
                  currentTCRT.putih, currentTCRT.hitam, currentTCRT.ambang, currentTCRT.valid);
}

MotorSettings settingsGetMotor() {
    return currentMotor;
}

void settingsSaveMotor(uint8_t kiri, uint8_t kanan) {
    currentMotor.pwmKiri = kiri;
    currentMotor.pwmKanan = kanan;
    if (prefs.begin(NVS_MOTOR_NS, false)) {
        prefs.putUChar("kiri", kiri);
        prefs.putUChar("kanan", kanan);
        prefs.end();
        Serial.printf("[SETTINGS] Saved Motor: L=%d R=%d\n", kiri, kanan);
    }
}

TCRTSettings settingsGetTCRT() {
    return currentTCRT;
}

void settingsSaveTCRT(int putih, int hitam, int ambang, bool valid) {
    currentTCRT.putih = putih;
    currentTCRT.hitam = hitam;
    currentTCRT.ambang = ambang;
    currentTCRT.valid = valid;
    if (prefs.begin(NVS_TCRT_NS, false)) {
        prefs.putInt("putih", putih);
        prefs.putInt("hitam", hitam);
        prefs.putInt("ambang", ambang);
        prefs.putBool("valid", valid);
        prefs.end();
        Serial.printf("[SETTINGS] Saved TCRT: P=%d H=%d Amb=%d Valid=%d\n", putih, hitam, ambang, valid);
    }
}

void settingsResetTCRT() {
    currentTCRT.putih = 0;
    currentTCRT.hitam = 0;
    currentTCRT.ambang = 0;
    currentTCRT.valid = false;
    if (prefs.begin(NVS_TCRT_NS, false)) {
        prefs.clear();
        prefs.end();
        Serial.println(F("[SETTINGS] Reset TCRT calibration"));
    }
}
