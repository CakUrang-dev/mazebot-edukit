#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

struct SensorData {
    float depan;        // cm
    float kiri;         // cm
    float kanan;        // cm
    int tcrtAdc;        // Raw ADC 0-4095
    int tcrtPutih;      // Nilai kalibrasi putih
    int tcrtHitam;      // Nilai kalibrasi hitam
    int tcrtAmbang;     // Ambang batas tengah
    bool tcrtValid;     // Apakah kalibrasi valid (selisih >= 20)
    bool garisHitam;    // Status deteksi garis hitam (finish/stop)
};

void sensorInit();
void sensorUpdate();

// Kalibrasi TCRT5000
bool sensorCalibrateWhite(String& outMsg);
bool sensorCalibrateBlack(String& outMsg);
void sensorResetCalibration();

// Akses Data
SensorData sensorGetData();

#endif // SENSOR_H
