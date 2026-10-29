#include "web_server.h"
#include "config.h"
#include "web_page.h"
#include "sensor.h"
#include "motor.h"
#include "settings.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

static WebServer server(80);
static DNSServer dnsServer;

static void sendCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

static void handleOptions() {
    sendCorsHeaders();
    server.send(204, "text/plain", "");
}

static void handleRoot() {
    sendCorsHeaders();
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send(200, "text/html; charset=utf-8", FPSTR(WEB_PAGE));
}

static void handleApiData() {
    motorFeedWatchdog();
    sendCorsHeaders();

    SensorData s = sensorGetData();
    MotorSettings mSaved = settingsGetMotor();

    char buf[384];
    snprintf(buf, sizeof(buf),
        "{\"depan\":%.1f,\"kiri\":%.1f,\"kanan\":%.1f,"
        "\"adc\":%d,\"putih\":%d,\"hitam\":%d,\"ambang\":%d,"
        "\"tcrtValid\":%s,\"garisHitam\":%s,"
        "\"mode\":\"%s\",\"pwmKiri\":%d,\"pwmKanan\":%d,"
        "\"savedKiri\":%d,\"savedKanan\":%d}",
        s.depan, s.kiri, s.kanan,
        s.tcrtAdc, s.tcrtPutih, s.tcrtHitam, s.tcrtAmbang,
        s.tcrtValid ? "true" : "false", s.garisHitam ? "true" : "false",
        motorGetMode().c_str(), motorGetPwmLeft(), motorGetPwmRight(),
        mSaved.pwmKiri, mSaved.pwmKanan
    );

    server.send(200, "application/json", buf);
}

static void handleApiDrive() {
    sendCorsHeaders();
    String mode = "stop";
    if (server.hasArg("mode")) mode = server.arg("mode");

    int l = -1;
    if (server.hasArg("l")) l = server.arg("l").toInt();
    else if (server.hasArg("pwmKiri")) l = server.arg("pwmKiri").toInt();

    int r = -1;
    if (server.hasArg("r")) r = server.arg("r").toInt();
    else if (server.hasArg("pwmKanan")) r = server.arg("pwmKanan").toInt();

    motorDrive(mode, l, r);
    server.send(200, "application/json", "{\"ok\":true,\"mode\":\"" + mode + "\"}");
}

static void handleApiStop() {
    sendCorsHeaders();
    motorStop();
    server.send(200, "application/json", "{\"ok\":true,\"mode\":\"stop\"}");
}

static void handleApiSave() {
    sendCorsHeaders();
    MotorSettings cur = settingsGetMotor();
    int l = cur.pwmKiri;
    int r = cur.pwmKanan;

    if (server.hasArg("l")) l = server.arg("l").toInt();
    else if (server.hasArg("pwmKiri")) l = server.arg("pwmKiri").toInt();

    if (server.hasArg("r")) r = server.arg("r").toInt();
    else if (server.hasArg("pwmKanan")) r = server.arg("pwmKanan").toInt();

    l = constrain(l, 0, PWM_MAX);
    r = constrain(r, 0, PWM_MAX);

    settingsSaveMotor(l, r);

    char buf[128];
    snprintf(buf, sizeof(buf), "{\"ok\":true,\"savedKiri\":%d,\"savedKanan\":%d}", l, r);
    server.send(200, "application/json", buf);
}

static void handleApiTcrtPutih() {
    sendCorsHeaders();
    String msg;
    bool valid = sensorCalibrateWhite(msg);

    char buf[256];
    snprintf(buf, sizeof(buf), "{\"ok\":true,\"pesan\":\"%s\",\"tcrtValid\":%s}", msg.c_str(), valid ? "true" : "false");
    server.send(200, "application/json", buf);
}

static void handleApiTcrtHitam() {
    sendCorsHeaders();
    String msg;
    bool valid = sensorCalibrateBlack(msg);

    char buf[256];
    snprintf(buf, sizeof(buf), "{\"ok\":true,\"pesan\":\"%s\",\"tcrtValid\":%s}", msg.c_str(), valid ? "true" : "false");
    server.send(200, "application/json", buf);
}

static void handleApiTcrtReset() {
    sendCorsHeaders();
    sensorResetCalibration();
    server.send(200, "application/json", "{\"ok\":true,\"pesan\":\"Kalibrasi TCRT direset.\"}");
}

static void handleCaptivePortal() {
    sendCorsHeaders();
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
}

static void handleNotFound() {
    String uri = server.uri();
    if (server.method() == HTTP_OPTIONS) {
        handleOptions();
        return;
    }
    // Captive portal probes
    if (uri.endsWith("/generate_204") || uri.endsWith("/hotspot-detect.html") ||
        uri.endsWith("/canonical.html") || uri.endsWith("/connecttest.txt")) {
        handleCaptivePortal();
        return;
    }
    // Fallback: layani dashboard jika path tidak dikenal
    handleRoot();
}

void webServerInit() {
    // 1. Inisialisasi SoftAP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, 0, AP_MAX_CONN);

    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WIFI] SoftAP '%s' aktif di IP %s\n", AP_SSID, apIP.toString().c_str());

    // 2. DNS Server untuk Captive Portal
    dnsServer.start(DNS_PORT, "*", apIP);

    // 3. Daftarkan URL & API Handlers
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleApiData);

    server.on("/api/drive", HTTP_GET, handleApiDrive);
    server.on("/api/drive", HTTP_POST, handleApiDrive);

    server.on("/api/stop", HTTP_GET, handleApiStop);
    server.on("/api/stop", HTTP_POST, handleApiStop);

    server.on("/api/save", HTTP_POST, handleApiSave);
    server.on("/api/save", HTTP_GET, handleApiSave);

    server.on("/api/tcrt/putih", HTTP_POST, handleApiTcrtPutih);
    server.on("/api/tcrt/hitam", HTTP_POST, handleApiTcrtHitam);
    server.on("/api/tcrt/reset", HTTP_POST, handleApiTcrtReset);

    // Preflight CORS handler
    server.on("/api/data", HTTP_OPTIONS, handleOptions);
    server.on("/api/drive", HTTP_OPTIONS, handleOptions);
    server.on("/api/stop", HTTP_OPTIONS, handleOptions);
    server.on("/api/save", HTTP_OPTIONS, handleOptions);
    server.on("/api/tcrt/putih", HTTP_OPTIONS, handleOptions);
    server.on("/api/tcrt/hitam", HTTP_OPTIONS, handleOptions);
    server.on("/api/tcrt/reset", HTTP_OPTIONS, handleOptions);

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println(F("[HTTP] WebServer & REST API started on port 80"));
}

void webServerLoop() {
    dnsServer.processNextRequest();
    server.handleClient();
}
