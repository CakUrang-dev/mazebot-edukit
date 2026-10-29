/**
 * @file main.cpp
 * @brief Firmware Utama MazeBot EduKit - ESP32 Autonomous Maze Solver
 * @author CakUrang-dev
 * @details Arsitektur modular PlatformIO clean code:
 *          - Motor Control DRV8833 PWM 18 kHz dengan fail-safe 650 ms
 *          - Sequential Non-Blocking Ultrasonic HC-SR04 (10.0/9.1 factor)
 *          - ADC1_CH4 GPIO 32 TCRT5000 Analog Line Tracking & Calibration
 *          - Embedded Web Dashboard (PROGMEM) & REST API
 *          - Captive Portal SoftAP (SSID: MazeSolver)
 */

#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "sensor.h"
#include "motor.h"
#include "web_server.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("   MAZEBOT EDUKIT - ESP32 FIRMWARE     "));
    Serial.println(F("   Modular Architecture v2.0           "));
    Serial.println(F("========================================"));

    // 1. Inisialisasi NVS Preferences
    settingsInit();

    // 2. Inisialisasi Sensor (HC-SR04 x3 & TCRT5000)
    sensorInit();

    // 3. Inisialisasi Driver Motor DRV8833
    motorInit();

    // 4. Inisialisasi WiFi SoftAP, DNS, & WebServer REST API
    webServerInit();

    Serial.println(F("[SYSTEM] Boot sequence complete. MazeBot Ready!"));
}

void loop() {
    // 1. Tangani request WebServer dan DNS Captive Portal
    webServerLoop();

    // 2. Pembaruan data sensor (round-robin non-blocking)
    sensorUpdate();

    // 3. Pembaruan state motor & evaluasi fail-safe timer
    motorUpdate();

    // 4. Yield sejenak untuk FreeRTOS & WiFi stack
    delay(2);
}
