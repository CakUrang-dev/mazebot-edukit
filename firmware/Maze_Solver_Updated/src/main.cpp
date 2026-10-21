#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "motor.h"
#include "sensor.h"
#include "settings.h"
#include "web_server.h"

void setup() {
  Serial.begin(115200);
  delay(500);

  // Driver motor harus OFF lebih dulu
  motorBegin();
  sensorBegin();
  settingsLoad();
  webBegin();

  Serial.println();
  Serial.println("================================");
  Serial.println("ESP32 MAZE SOLVER");
  Serial.println("MOTOR CALIBRATION READY");
  Serial.println("================================");

  Serial.print("SSID: ");
  Serial.println(WIFI_NAMA);

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.println("PWM RANGE: 0 - 255");
  Serial.println("MOTOR: STOP");
}

void loop() {
  webLoop();
  motorUpdate();
  sensorUpdate();

  delay(1);
}