#pragma once
#include <Arduino.h>

// =========================================
// WIFI ACCESS POINT
// =========================================
constexpr const char* WIFI_NAMA     = "MazeSolver";
constexpr const char* WIFI_PASSWORD = "12345678";
constexpr uint8_t     WIFI_MAX_KLIEN = 2;

// =========================================
// HC-SR04
// =========================================
constexpr uint8_t TRIG_DEPAN = 4;
constexpr uint8_t ECHO_DEPAN = 17;

constexpr uint8_t TRIG_KIRI = 19;
constexpr uint8_t ECHO_KIRI = 18;

constexpr uint8_t TRIG_KANAN = 14;
constexpr uint8_t ECHO_KANAN = 13;

constexpr float    KALIBRASI_SENSOR   = 10.0f / 9.1f;
constexpr uint32_t SENSOR_INTERVAL_MS = 80;     // jeda antar pembacaan (bergantian)
constexpr uint32_t ECHO_TIMEOUT_US    = 25000;  // batas tunggu echo

// =========================================
// DRV8833
// MOTOR A = KIRI, MOTOR B = KANAN
// =========================================
constexpr uint8_t AIN1 = 27;
constexpr uint8_t AIN2 = 26;
constexpr uint8_t BIN1 = 25;
constexpr uint8_t BIN2 = 33;
constexpr uint8_t STBY = 23;

// Ganti true jika arah roda terbalik
constexpr bool BALIK_KIRI  = false;
constexpr bool BALIK_KANAN = false;

// =========================================
// PWM MOTOR 12V - BATERAI 2S
// =========================================
constexpr int PWM_MAX  = 255;
constexpr int PWM_FREQ = 18000;
constexpr int PWM_BITS = 8;

constexpr uint8_t PWM_DEFAULT = 220;            // nilai awal sebelum kalibrasi

constexpr uint32_t MOTOR_TIMEOUT_MS = 650;      // fail-safe jika perintah berhenti
constexpr uint32_t JEDA_GANTI_ARAH_US = 40000;  // jeda saat berbalik arah
constexpr uint32_t JEDA_BANGUN_US     = 2000;   // jeda setelah STBY dinyalakan