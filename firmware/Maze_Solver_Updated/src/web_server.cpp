#include "web_server.h"
#include "config.h"
#include "motor.h"
#include "sensor.h"
#include "settings.h"
#include "web_page.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

namespace {

const IPAddress IP_AP(192, 168, 4, 1);
const IPAddress GATEWAY(192, 168, 4, 1);
const IPAddress SUBNET(255, 255, 255, 0);

WebServer server(80);
DNSServer dns;

const char JSON_OK[]    = "{\"ok\":true}";
const char JSON_GAGAL[] = "{\"ok\":false}";

// Jarak -> teks JSON ("null" jika tidak valid)
void formatJarak(char* out, size_t n, float jarak) {
  if (jarak < 0) {
    strlcpy(out, "null", n);
  } else {
    snprintf(out, n, "%.1f", jarak);
  }
}

// =========================================
// API
// =========================================
void apiData() {
  char depan[12], kiri[12], kanan[12];
  formatJarak(depan, sizeof(depan), sensorDepan());
  formatJarak(kiri,  sizeof(kiri),  sensorKiri());
  formatJarak(kanan, sizeof(kanan), sensorKanan());

  char json[192];
  snprintf(json, sizeof(json),
           "{\"depan\":%s,\"kiri\":%s,\"kanan\":%s,"
           "\"mode\":\"%s\",\"pwmKiri\":%d,\"pwmKanan\":%d,"
           "\"savedKiri\":%u,\"savedKanan\":%u}",
           depan, kiri, kanan,
           motorModeNama(motorMode()), motorPwmKiri(), motorPwmKanan(),
           settingsPwmKiri(), settingsPwmKanan());

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void apiDrive() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  MotorMode mode;
  if (!motorParseMode(server.arg("mode").c_str(), mode)) {
    server.send(400, "application/json", JSON_GAGAL);
    return;
  }

  if (!server.hasArg("l") || !server.hasArg("r")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"PWM tidak ada\"}");
    return;
  }

  const int l = constrain(server.arg("l").toInt(), 0, PWM_MAX);
  const int r = constrain(server.arg("r").toInt(), 0, PWM_MAX);

  motorDrive(mode, l, r);
  server.send(200, "application/json", JSON_OK);
}

void apiStop() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  motorStop();
  server.send(200, "application/json", JSON_OK);
}

void apiSave() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  motorStop();

  if (!server.hasArg("l") || !server.hasArg("r")) {
    server.send(400, "application/json", JSON_GAGAL);
    return;
  }

  const int l = constrain(server.arg("l").toInt(), 0, PWM_MAX);
  const int r = constrain(server.arg("r").toInt(), 0, PWM_MAX);

  settingsSave(l, r);
  server.send(200, "application/json", JSON_OK);
}

// =========================================
// CAPTIVE PORTAL
// =========================================
void redirectPortal() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

}  // namespace

// =========================================
// PUBLIK
// =========================================
void webBegin() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IP_AP, GATEWAY, SUBNET);
  WiFi.softAP(WIFI_NAMA, WIFI_PASSWORD, 1, 0, WIFI_MAX_KLIEN);

  dns.start(53, "*", IP_AP);

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", WEB);
  });

  server.on("/api/data",  HTTP_GET,  apiData);
  server.on("/api/drive", HTTP_POST, apiDrive);
  server.on("/api/stop",  HTTP_POST, apiStop);
  server.on("/api/save",  HTTP_POST, apiSave);

  // Deteksi captive portal (Android, iOS, Windows)
  static const char* const PATH_PORTAL[] = {
    "/generate_204", "/gen_204", "/hotspot-detect.html", "/connecttest.txt"
  };
  for (const char* path : PATH_PORTAL) {
    server.on(path, HTTP_GET, redirectPortal);
  }

  server.onNotFound(redirectPortal);
  server.begin();
}

void webLoop() {
  dns.processNextRequest();
  server.handleClient();
}