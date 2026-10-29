#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// KONFIGURASI WIFI SOFTAP & CAPTIVE PORTAL
// =============================================================================
#define AP_SSID             "MazeSolver"
#define AP_PASS             "12345678"
#define AP_CHANNEL          1
#define AP_MAX_CONN         4
#define DNS_PORT            53

// =============================================================================
// PINOUT HARDWARE ESP32
// =============================================================================

// Sensor Ultrasonik HC-SR04 (3 Unit)
#define PIN_TRIG_DEPAN      4
#define PIN_ECHO_DEPAN      17

#define PIN_TRIG_KIRI       19
#define PIN_ECHO_KIRI       18

#define PIN_TRIG_KANAN      14
#define PIN_ECHO_KANAN      13

// Sensor Garis Finish TCRT5000 (Analog 12-bit)
#define PIN_TCRT_ADC        32   // ADC1_CH4 (0 - 4095)

// Driver Motor DRV8833
#define PIN_AIN1            27   // Motor Kiri Maju  (PWM Channel 0)
#define PIN_AIN2            26   // Motor Kiri Mundur (PWM Channel 1)
#define PIN_BIN1            25   // Motor Kanan Maju (PWM Channel 2)
#define PIN_BIN2            33   // Motor Kanan Mundur (PWM Channel 3)
#define PIN_STBY            23   // Driver Standby (Active HIGH)

// =============================================================================
// PARAMETER PWM MOTOR & FAIL-SAFE
// =============================================================================
#define PWM_FREQ            18000   // 18 kHz (Senyap, di luar frekuensi audio)
#define PWM_RES_BITS        8       // 8-bit (0 - 255)
#define PWM_MAX             255

#define CH_AIN1             0
#define CH_AIN2             1
#define CH_BIN1             2
#define CH_BIN2             3

#define DEFAULT_PWM_KIRI    255
#define DEFAULT_PWM_KANAN   244

#define MOTOR_FAILSAFE_MS   650     // Timeout pengaman jika klien terputus
#define MOTOR_RAMP_STEP_MS  35      // Soft-transition interval (anti-slip)

// =============================================================================
// PARAMETER KALIBRASI SENSOR
// =============================================================================
// HC-SR04
#define HCSR_CAL_FACTOR     (10.0f / 9.1f)  // Faktor koreksi fisik 10cm / 9.1cm baca
#define HCSR_TIMEOUT_US     25000           // Timeout pulseIn (~4 meter)
#define HCSR_INTERVAL_MS    80              // Jeda antar-sensor round-robin

// TCRT5000
#define TCRT_OVERSAMPLE     16              // Jumlah sampel oversampling saat idle
#define TCRT_CAL_SAMPLES    48              // Jumlah sampel saat kalibrasi
#define TCRT_MIN_DELTA      20              // Selisih minimal ADC hitam vs putih
#define TCRT_HYSTERESIS     6               // Ambang histeresis anti-flicker

// =============================================================================
// NAMESPACE NVS / PREFERENCES
// =============================================================================
#define NVS_MOTOR_NS        "maze-motor"
#define NVS_TCRT_NS         "maze-tcrt"

#endif // CONFIG_H
