#include "motor.h"
#include "config.h"
#include "settings.h"

#if defined(__has_include)
  #if __has_include(<esp_arduino_version.h>)
    #include <esp_arduino_version.h>
  #endif
#endif

#ifndef ESP_ARDUINO_VERSION_MAJOR
  #define ESP_ARDUINO_VERSION_MAJOR 2
#endif

static String currentMode = "stop";
static int targetPwmL = 0;
static int targetPwmR = 0;
static int actualPwmL = 0;
static int actualPwmR = 0;

static unsigned long lastDriveTime = 0;
static unsigned long lastRampTime = 0;

static void writePwmRaw(uint8_t pin, uint8_t channel, uint32_t val) {
#if (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcWrite(pin, val);
#else
    ledcWrite(channel, val);
#endif
}

static void applyRawHBridge(const String& mode, int l, int r) {
    if (mode == "stop" || (l == 0 && r == 0)) {
        writePwmRaw(PIN_AIN1, CH_AIN1, 0);
        writePwmRaw(PIN_AIN2, CH_AIN2, 0);
        writePwmRaw(PIN_BIN1, CH_BIN1, 0);
        writePwmRaw(PIN_BIN2, CH_BIN2, 0);
        digitalWrite(PIN_STBY, LOW); // Sleep DRV8833
        return;
    }

    digitalWrite(PIN_STBY, HIGH); // Bangunkan DRV8833

    if (mode == "maju") {
        writePwmRaw(PIN_AIN1, CH_AIN1, l);
        writePwmRaw(PIN_AIN2, CH_AIN2, 0);
        writePwmRaw(PIN_BIN1, CH_BIN1, r);
        writePwmRaw(PIN_BIN2, CH_BIN2, 0);
    } else if (mode == "mundur") {
        writePwmRaw(PIN_AIN1, CH_AIN1, 0);
        writePwmRaw(PIN_AIN2, CH_AIN2, l);
        writePwmRaw(PIN_BIN1, CH_BIN1, 0);
        writePwmRaw(PIN_BIN2, CH_BIN2, r);
    } else if (mode == "belok_kiri" || mode == "kiri") {
        writePwmRaw(PIN_AIN1, CH_AIN1, 0);
        writePwmRaw(PIN_AIN2, CH_AIN2, l);
        writePwmRaw(PIN_BIN1, CH_BIN1, r);
        writePwmRaw(PIN_BIN2, CH_BIN2, 0);
    } else if (mode == "belok_kanan" || mode == "kanan") {
        writePwmRaw(PIN_AIN1, CH_AIN1, l);
        writePwmRaw(PIN_AIN2, CH_AIN2, 0);
        writePwmRaw(PIN_BIN1, CH_BIN1, 0);
        writePwmRaw(PIN_BIN2, CH_BIN2, r);
    } else if (mode == "uji_kiri") {
        writePwmRaw(PIN_AIN1, CH_AIN1, l);
        writePwmRaw(PIN_AIN2, CH_AIN2, 0);
        writePwmRaw(PIN_BIN1, CH_BIN1, 0);
        writePwmRaw(PIN_BIN2, CH_BIN2, 0);
    } else if (mode == "uji_kanan") {
        writePwmRaw(PIN_AIN1, CH_AIN1, 0);
        writePwmRaw(PIN_AIN2, CH_AIN2, 0);
        writePwmRaw(PIN_BIN1, CH_BIN1, r);
        writePwmRaw(PIN_BIN2, CH_BIN2, 0);
    }
}

void motorInit() {
    pinMode(PIN_STBY, OUTPUT);
    digitalWrite(PIN_STBY, LOW);

#if (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcAttach(PIN_AIN1, PWM_FREQ, PWM_RES_BITS);
    ledcAttach(PIN_AIN2, PWM_FREQ, PWM_RES_BITS);
    ledcAttach(PIN_BIN1, PWM_FREQ, PWM_RES_BITS);
    ledcAttach(PIN_BIN2, PWM_FREQ, PWM_RES_BITS);
#else
    ledcSetup(CH_AIN1, PWM_FREQ, PWM_RES_BITS);
    ledcSetup(CH_AIN2, PWM_FREQ, PWM_RES_BITS);
    ledcSetup(CH_BIN1, PWM_FREQ, PWM_RES_BITS);
    ledcSetup(CH_BIN2, PWM_FREQ, PWM_RES_BITS);

    ledcAttachPin(PIN_AIN1, CH_AIN1);
    ledcAttachPin(PIN_AIN2, CH_AIN2);
    ledcAttachPin(PIN_BIN1, CH_BIN1);
    ledcAttachPin(PIN_BIN2, CH_BIN2);
#endif

    motorStop();
    Serial.println(F("[MOTOR] DRV8833 initialized (18kHz, Soft-transition 35ms, Failsafe 650ms)"));
}

void motorDrive(const String& mode, int targetL, int targetR) {
    MotorSettings saved = settingsGetMotor();
    if (targetL < 0) targetL = saved.pwmKiri;
    if (targetR < 0) targetR = saved.pwmKanan;

    targetPwmL = constrain(targetL, 0, PWM_MAX);
    targetPwmR = constrain(targetR, 0, PWM_MAX);

    currentMode = mode;
    lastDriveTime = millis();

    if (mode == "stop") {
        targetPwmL = 0;
        targetPwmR = 0;
        actualPwmL = 0;
        actualPwmR = 0;
        applyRawHBridge("stop", 0, 0);
    }
}

void motorStop() {
    motorDrive("stop", 0, 0);
}

void motorFeedWatchdog() {
    lastDriveTime = millis();
}

void motorUpdate() {
    unsigned long now = millis();

    // 1. Fail-safe timeout check
    if (currentMode != "stop" && (now - lastDriveTime > MOTOR_FAILSAFE_MS)) {
        motorStop();
        Serial.println(F("[MOTOR] Fail-safe triggered: client timeout (>650ms)"));
        return;
    }

    // 2. Soft-ramp transition (smooth acceleration/deceleration)
    if (now - lastRampTime >= MOTOR_RAMP_STEP_MS) {
        lastRampTime = now;
        bool changed = false;

        if (actualPwmL < targetPwmL) {
            actualPwmL = min(actualPwmL + 30, targetPwmL);
            changed = true;
        } else if (actualPwmL > targetPwmL) {
            actualPwmL = max(actualPwmL - 40, targetPwmL);
            changed = true;
        }

        if (actualPwmR < targetPwmR) {
            actualPwmR = min(actualPwmR + 30, targetPwmR);
            changed = true;
        } else if (actualPwmR > targetPwmR) {
            actualPwmR = max(actualPwmR - 40, targetPwmR);
            changed = true;
        }

        if (changed && currentMode != "stop") {
            applyRawHBridge(currentMode, actualPwmL, actualPwmR);
        }
    }
}

String motorGetMode() {
    return currentMode;
}

int motorGetPwmLeft() {
    return actualPwmL;
}

int motorGetPwmRight() {
    return actualPwmR;
}

bool motorIsMoving() {
    return (currentMode != "stop");
}
