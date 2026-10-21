#include "sensor.h"
#include "config.h"

namespace {

struct Sensor {
  uint8_t trig;
  uint8_t echo;
  volatile uint32_t tNaik;    // waktu echo naik (us)
  volatile uint32_t tTurun;   // waktu echo turun (us)
  volatile bool     selesai;
  float             jarak;    // cm, -1 jika tidak valid
};

Sensor s_sensor[3] = {
  {TRIG_DEPAN, ECHO_DEPAN, 0, 0, false, -1.0f},
  {TRIG_KIRI,  ECHO_KIRI,  0, 0, false, -1.0f},
  {TRIG_KANAN, ECHO_KANAN, 0, 0, false, -1.0f},
};

uint8_t  s_idx      = 0;       // sensor yang sedang aktif
bool     s_menunggu = false;   // sedang menunggu echo
uint32_t s_mulaiMs  = 0;
uint32_t s_trigUs   = 0;

void IRAM_ATTR echoISR(void* arg) {
  Sensor* sn = static_cast<Sensor*>(arg);
  const uint32_t now = micros();

  if (digitalRead(sn->echo)) {
    sn->tNaik = now;
  } else {
    sn->tTurun = now;
    sn->selesai = true;
  }
}

float hitungJarak(uint32_t durasiUs) {
  float jarak = durasiUs * 0.0343f / 2.0f;
  jarak *= KALIBRASI_SENSOR;

  if (jarak < 2 || jarak > 400) return -1.0f;
  return jarak;
}

void kirimTrigger(const Sensor& sn) {
  digitalWrite(sn.trig, LOW);
  delayMicroseconds(3);
  digitalWrite(sn.trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(sn.trig, LOW);
}

}  // namespace

void sensorBegin() {
  for (Sensor& sn : s_sensor) {
    pinMode(sn.trig, OUTPUT);
    pinMode(sn.echo, INPUT);
    digitalWrite(sn.trig, LOW);

    attachInterruptArg(digitalPinToInterrupt(sn.echo), echoISR, &sn, CHANGE);
  }
}

void sensorUpdate() {
  Sensor& sn = s_sensor[s_idx];

  if (!s_menunggu) {
    if (millis() - s_mulaiMs < SENSOR_INTERVAL_MS) return;

    s_mulaiMs = millis();
    sn.tNaik   = 0;
    sn.selesai = false;

    kirimTrigger(sn);

    s_trigUs   = micros();
    s_menunggu = true;
    return;
  }

  if (sn.selesai) {
    if (sn.tNaik != 0) {
      sn.jarak = hitungJarak(sn.tTurun - sn.tNaik);
    } else {
      sn.jarak = -1.0f;
    }
  } else if (micros() - s_trigUs > ECHO_TIMEOUT_US) {
    sn.jarak = -1.0f;
  } else {
    return;  // masih menunggu
  }

  s_menunggu = false;
  s_idx = (s_idx + 1) % 3;
}

float sensorDepan() { return s_sensor[0].jarak; }
float sensorKiri()  { return s_sensor[1].jarak; }
float sensorKanan() { return s_sensor[2].jarak; }