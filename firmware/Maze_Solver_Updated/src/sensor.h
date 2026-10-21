#pragma once
#include <Arduino.h>

void sensorBegin();

// Panggil setiap loop. Non-blocking: memakai interrupt pada pin ECHO.
void sensorUpdate();

// Jarak dalam cm, atau -1 jika tidak valid.
float sensorDepan();
float sensorKiri();
float sensorKanan();