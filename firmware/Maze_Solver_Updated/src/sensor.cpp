#include "sensor.h"
#include "config.h"
#include "settings.h"

static SensorData currentData = { 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, false, false };

// Round-robin scheduling HC-SR04
enum UltrasonicStep { STEP_DEPAN, STEP_KIRI, STEP_KANAN };
static UltrasonicStep currentStep = STEP_DEPAN;
static unsigned long lastUltrasonicTime = 0;

static float readSingleHCSR(uint8_t trigPin, uint8_t echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    unsigned long duration = pulseIn(echoPin, HIGH, HCSR_TIMEOUT_US);
    if (duration == 0) return 999.0f; // Tidak ada pantulan / out of range

    // Hitung jarak (cm) dengan faktor kalibrasi fisik 10.0 / 9.1
    float dist = (duration * 0.0343f / 2.0f) * HCSR_CAL_FACTOR;
    return constrain(dist, 1.0f, 400.0f);
}

static int sampleTCRTRaw(int count) {
    long sum = 0;
    for (int i = 0; i < count; i++) {
        sum += analogRead(PIN_TCRT_ADC);
        delayMicroseconds(50);
    }
    return (int)(sum / count);
}

void sensorInit() {
    // 1. Setup Pin HC-SR04
    pinMode(PIN_TRIG_DEPAN, OUTPUT);
    pinMode(PIN_ECHO_DEPAN, INPUT);
    pinMode(PIN_TRIG_KIRI, OUTPUT);
    pinMode(PIN_ECHO_KIRI, INPUT);
    pinMode(PIN_TRIG_KANAN, OUTPUT);
    pinMode(PIN_ECHO_KANAN, INPUT);

    digitalWrite(PIN_TRIG_DEPAN, LOW);
    digitalWrite(PIN_TRIG_KIRI, LOW);
    digitalWrite(PIN_TRIG_KANAN, LOW);

    // 2. Setup Pin TCRT5000 (12-bit ADC)
    pinMode(PIN_TCRT_ADC, INPUT);
    analogReadResolution(12);
#if defined(ARDUINO_ARCH_ESP32)
    analogSetAttenuation(ADC_11db);
#endif

    // 3. Muat kalibrasi tersimpan
    TCRTSettings tcrt = settingsGetTCRT();
    currentData.tcrtPutih = tcrt.putih;
    currentData.tcrtHitam = tcrt.hitam;
    currentData.tcrtAmbang = tcrt.ambang;
    currentData.tcrtValid = tcrt.valid;

    Serial.println(F("[SENSOR] Ultrasonic 3x HC-SR04 & TCRT5000 (GPIO 32) initialized"));
}

void sensorUpdate() {
    unsigned long now = millis();

    // 1. Pembacaan Ultrasonik Berurutan (Round-Robin non-blocking)
    if (now - lastUltrasonicTime >= HCSR_INTERVAL_MS) {
        lastUltrasonicTime = now;

        switch (currentStep) {
            case STEP_DEPAN:
                currentData.depan = readSingleHCSR(PIN_TRIG_DEPAN, PIN_ECHO_DEPAN);
                currentStep = STEP_KIRI;
                break;
            case STEP_KIRI:
                currentData.kiri = readSingleHCSR(PIN_TRIG_KIRI, PIN_ECHO_KIRI);
                currentStep = STEP_KANAN;
                break;
            case STEP_KANAN:
                currentData.kanan = readSingleHCSR(PIN_TRIG_KANAN, PIN_ECHO_KANAN);
                currentStep = STEP_DEPAN;
                break;
        }
    }

    // 2. Pembacaan Sensor TCRT5000 (Oversampling 16x)
    currentData.tcrtAdc = sampleTCRTRaw(TCRT_OVERSAMPLE);

    // 3. Deteksi Garis Hitam dengan Hysteresis
    if (currentData.tcrtValid) {
        if (!currentData.garisHitam) {
            if (currentData.tcrtAdc >= (currentData.tcrtAmbang + TCRT_HYSTERESIS)) {
                currentData.garisHitam = true;
            }
        } else {
            if (currentData.tcrtAdc <= (currentData.tcrtAmbang - TCRT_HYSTERESIS)) {
                currentData.garisHitam = false;
            }
        }
    } else {
        currentData.garisHitam = false;
    }
}

bool sensorCalibrateWhite(String& outMsg) {
    int sample = sampleTCRTRaw(TCRT_CAL_SAMPLES);
    currentData.tcrtPutih = sample;
    currentData.tcrtAmbang = (currentData.tcrtPutih + currentData.tcrtHitam) / 2;
    int delta = abs(currentData.tcrtHitam - currentData.tcrtPutih);
    currentData.tcrtValid = (delta >= TCRT_MIN_DELTA);

    settingsSaveTCRT(currentData.tcrtPutih, currentData.tcrtHitam, currentData.tcrtAmbang, currentData.tcrtValid);

    if (currentData.tcrtValid) {
        outMsg = "Kalibrasi PUTIH sukses (" + String(sample) + "). Ambang: " + String(currentData.tcrtAmbang);
        return true;
    } else {
        outMsg = "Sampel PUTIH dicatat (" + String(sample) + "). Lakukan kalibrasi HITAM untuk validasi.";
        return false;
    }
}

bool sensorCalibrateBlack(String& outMsg) {
    int sample = sampleTCRTRaw(TCRT_CAL_SAMPLES);
    currentData.tcrtHitam = sample;
    currentData.tcrtAmbang = (currentData.tcrtPutih + currentData.tcrtHitam) / 2;
    int delta = abs(currentData.tcrtHitam - currentData.tcrtPutih);
    currentData.tcrtValid = (delta >= TCRT_MIN_DELTA);

    settingsSaveTCRT(currentData.tcrtPutih, currentData.tcrtHitam, currentData.tcrtAmbang, currentData.tcrtValid);

    if (currentData.tcrtValid) {
        outMsg = "Kalibrasi HITAM sukses (" + String(sample) + "). Ambang: " + String(currentData.tcrtAmbang);
        return true;
    } else {
        outMsg = "Sampel HITAM dicatat (" + String(sample) + "). Lakukan kalibrasi PUTIH untuk validasi.";
        return false;
    }
}

void sensorResetCalibration() {
    settingsResetTCRT();
    currentData.tcrtPutih = 0;
    currentData.tcrtHitam = 0;
    currentData.tcrtAmbang = 0;
    currentData.tcrtValid = false;
    currentData.garisHitam = false;
}

SensorData sensorGetData() {
    return currentData;
}
