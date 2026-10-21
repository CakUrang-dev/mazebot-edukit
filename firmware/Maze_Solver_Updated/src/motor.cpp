#include "motor.h"
#include "config.h"
#include <WiFi.h>
#include <strings.h>
#include <esp_arduino_version.h>

namespace {

struct Roda {
  uint8_t pin1;
  uint8_t pin2;
  uint8_t ch1;  // dipakai hanya pada Arduino core 2.x
  uint8_t ch2;
};

const Roda RODA_KIRI  = {AIN1, AIN2, 0, 1};
const Roda RODA_KANAN = {BIN1, BIN2, 2, 3};

MotorMode s_mode = MotorMode::STOP;

int s_pwmKiri  = 0;   // PWM aktif (nilai absolut)
int s_pwmKanan = 0;
int s_cmdKiri  = 0;   // perintah bertanda (+maju / -mundur)
int s_cmdKanan = 0;

uint32_t s_lastCmd = 0;

// Jeda non-blocking sebelum output diterapkan
bool     s_menunggu     = false;
uint32_t s_mulaiUs      = 0;
uint32_t s_jedaUs       = 0;

void tulisPWM(uint8_t pin, uint8_t ch, int duty) {
  duty = constrain(duty, 0, PWM_MAX);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)ch;
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(ch, duty);
#endif
}

void setupPWM() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(AIN1, PWM_FREQ, PWM_BITS);
  ledcAttach(AIN2, PWM_FREQ, PWM_BITS);
  ledcAttach(BIN1, PWM_FREQ, PWM_BITS);
  ledcAttach(BIN2, PWM_FREQ, PWM_BITS);
#else
  const Roda* roda[2] = {&RODA_KIRI, &RODA_KANAN};
  for (const Roda* r : roda) {
    ledcSetup(r->ch1, PWM_FREQ, PWM_BITS);
    ledcSetup(r->ch2, PWM_FREQ, PWM_BITS);
    ledcAttachPin(r->pin1, r->ch1);
    ledcAttachPin(r->pin2, r->ch2);
  }
#endif
}

void semuaPWMOff() {
  tulisPWM(RODA_KIRI.pin1,  RODA_KIRI.ch1,  0);
  tulisPWM(RODA_KIRI.pin2,  RODA_KIRI.ch2,  0);
  tulisPWM(RODA_KANAN.pin1, RODA_KANAN.ch1, 0);
  tulisPWM(RODA_KANAN.pin2, RODA_KANAN.ch2, 0);
}

void setRoda(const Roda& r, int pwm) {
  pwm = constrain(pwm, -PWM_MAX, PWM_MAX);

  if (pwm > 0) {
    tulisPWM(r.pin2, r.ch2, 0);
    tulisPWM(r.pin1, r.ch1, pwm);
  } else if (pwm < 0) {
    tulisPWM(r.pin1, r.ch1, 0);
    tulisPWM(r.pin2, r.ch2, -pwm);
  } else {
    tulisPWM(r.pin1, r.ch1, 0);
    tulisPWM(r.pin2, r.ch2, 0);
  }
}

void terapkanOutput() {
  digitalWrite(STBY, HIGH);
  setRoda(RODA_KIRI,  s_cmdKiri);
  setRoda(RODA_KANAN, s_cmdKanan);
}

}  // namespace

// =========================================
// PUBLIK
// =========================================
void motorBegin() {
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, LOW);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);

  setupPWM();
  motorStop();
}

void motorStop() {
  semuaPWMOff();
  digitalWrite(STBY, LOW);

  s_mode     = MotorMode::STOP;
  s_pwmKiri  = 0;
  s_pwmKanan = 0;
  s_cmdKiri  = 0;
  s_cmdKanan = 0;
  s_menunggu = false;
}

void motorDrive(MotorMode mode, int pwmKiri, int pwmKanan) {
  pwmKiri  = constrain(pwmKiri,  0, PWM_MAX);
  pwmKanan = constrain(pwmKanan, 0, PWM_MAX);

  int kiri  = 0;
  int kanan = 0;

  switch (mode) {
    case MotorMode::UJI_KIRI:    kiri =  pwmKiri;                    break;
    case MotorMode::UJI_KANAN:   kanan = pwmKanan;                   break;
    case MotorMode::MAJU:        kiri =  pwmKiri; kanan =  pwmKanan; break;
    case MotorMode::MUNDUR:      kiri = -pwmKiri; kanan = -pwmKanan; break;
    case MotorMode::BELOK_KIRI:  kiri = -pwmKiri; kanan =  pwmKanan; break;
    case MotorMode::BELOK_KANAN: kiri =  pwmKiri; kanan = -pwmKanan; break;
    default:
      motorStop();
      return;
  }

  if (BALIK_KIRI)  kiri  = -kiri;
  if (BALIK_KANAN) kanan = -kanan;

  if (kiri == 0 && kanan == 0) {
    motorStop();
    return;
  }

  const bool gantiArah = (mode != s_mode);
  const bool dariStop  = (s_mode == MotorMode::STOP);

  s_lastCmd  = millis();
  s_cmdKiri  = kiri;
  s_cmdKanan = kanan;
  s_pwmKiri  = abs(kiri);
  s_pwmKanan = abs(kanan);

  if (gantiArah) {
    // Kurangi hentakan: matikan dulu, terapkan setelah jeda (tanpa delay())
    if (!dariStop) {
      semuaPWMOff();
      digitalWrite(STBY, LOW);
      s_jedaUs = JEDA_GANTI_ARAH_US;
    } else {
      s_jedaUs = JEDA_BANGUN_US;
      digitalWrite(STBY, HIGH);
    }

    s_mode     = mode;
    s_menunggu = true;
    s_mulaiUs  = micros();
    return;
  }

  // Arah sama (heartbeat / perubahan PWM): langsung terapkan
  if (!s_menunggu) {
    terapkanOutput();
  }
}

void motorUpdate() {
  if (s_mode == MotorMode::STOP) return;

  if (s_menunggu && (micros() - s_mulaiUs) >= s_jedaUs) {
    terapkanOutput();
    s_menunggu = false;
  }

  // Fail-safe
  if (WiFi.softAPgetStationNum() == 0 ||
      (millis() - s_lastCmd) > MOTOR_TIMEOUT_MS) {
    motorStop();
  }
}

MotorMode motorMode()   { return s_mode; }
int motorPwmKiri()      { return s_pwmKiri; }
int motorPwmKanan()     { return s_pwmKanan; }

const char* motorModeNama(MotorMode mode) {
  switch (mode) {
    case MotorMode::UJI_KIRI:    return "uji_kiri";
    case MotorMode::UJI_KANAN:   return "uji_kanan";
    case MotorMode::MAJU:        return "maju";
    case MotorMode::MUNDUR:      return "mundur";
    case MotorMode::BELOK_KIRI:  return "belok_kiri";
    case MotorMode::BELOK_KANAN: return "belok_kanan";
    default:                     return "stop";
  }
}

bool motorParseMode(const char* teks, MotorMode& hasil) {
  static const MotorMode DAFTAR[] = {
    MotorMode::UJI_KIRI, MotorMode::UJI_KANAN, MotorMode::MAJU,
    MotorMode::MUNDUR,   MotorMode::BELOK_KIRI, MotorMode::BELOK_KANAN
  };

  for (MotorMode m : DAFTAR) {
    if (strcasecmp(teks, motorModeNama(m)) == 0) {
      hasil = m;
      return true;
    }
  }
  return false;
}