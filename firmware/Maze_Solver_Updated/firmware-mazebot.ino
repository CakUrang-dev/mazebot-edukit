#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// Kompatibilitas header versi core ESP32 (Core 2.x & 3.x)
#if defined(__has_include)
  #if __has_include(<esp_arduino_version.h>)
    #include <esp_arduino_version.h>
  #endif
#endif
#ifndef ESP_ARDUINO_VERSION_MAJOR
  #define ESP_ARDUINO_VERSION_MAJOR 2
#endif

// ======================================
// WIFI ACCESS POINT
// ======================================
const char* AP_SSID = "MazeSolver";
const char* AP_PASS = "12345678";

IPAddress apIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);
DNSServer dns;
Preferences prefs;

// ======================================
// PIN HC-SR04
// ======================================
#define TRIG_DEPAN 4
#define ECHO_DEPAN 17
#define TRIG_KIRI 19
#define ECHO_KIRI 18
#define TRIG_KANAN 14
#define ECHO_KANAN 13

const float KALIBRASI_HCSR = 10.0f / 9.1f;

float jarakDepan = -1;
float jarakKiri = -1;
float jarakKanan = -1;

unsigned long lastHCSR = 0;
int indexHCSR = 0;

// ======================================
// TCRT5000 GPIO32 (ADC1_CH4)
// ======================================
#define PIN_TCRT 32

// Hasil kalibrasi pengguna
int adcPutih = 0;
int adcHitam = 56;

int nilaiTCRT = 0;
int ambangTCRT = 28;

const int MIN_SELISIH_TCRT = 20;
const int HYSTERESIS_TCRT = 6;

bool tcrtValid = false;
bool hitamLebihTinggi = true;
bool garisHitam = false;

unsigned long lastTCRT = 0;

// ======================================
// DRV8833 - DUA RODA
// ======================================
#define AIN1 27
#define AIN2 26
#define BIN1 25
#define BIN2 33
#define STBY 23

const bool BALIK_KIRI = false;
const bool BALIK_KANAN = false;

const int PWM_FREQ = 18000;
const int PWM_BITS = 8;

const int CH_A1 = 0;
const int CH_A2 = 1;
const int CH_B1 = 2;
const int CH_B2 = 3;

// Nilai awal berdasarkan hasil uji PWM
int pwmSimpanKiri = 255;
int pwmSimpanKanan = 244;

int pwmAktifKiri = 0;
int pwmAktifKanan = 0;

String modeMotor = "stop";

unsigned long lastMotor = 0;
const unsigned long MOTOR_TIMEOUT = 650;

// ======================================
// HTML DASHBOARD
// ======================================
const char WEB[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Maze Solver ESP32</title>

<style>
*{box-sizing:border-box}

body{
 margin:0;
 padding:16px;
 background:#0f172a;
 color:#f8fafc;
 font-family:Arial,sans-serif;
}

.wrap{
 max-width:560px;
 margin:auto;
}

h2{
 text-align:center;
 margin:8px 0;
}

.subtitle{
 text-align:center;
 color:#94a3b8;
 font-size:13px;
}

.card{
 background:#1e293b;
 padding:17px;
 border-radius:14px;
 margin:14px 0;
}

h3{
 margin:0 0 15px;
 font-size:17px;
}

.grid{
 display:grid;
 grid-template-columns:repeat(3,1fr);
 gap:8px;
}

.sensor{
 background:#0f172a;
 text-align:center;
 padding:14px 5px;
 border-radius:10px;
}

.label{
 font-size:12px;
 color:#cbd5e1;
}

.num{
 font-size:27px;
 font-weight:bold;
 color:#38bdf8;
 margin:10px 0;
}

.small{
 font-size:12px;
 color:#94a3b8;
 text-align:center;
 line-height:1.6;
}

.info{
 background:#0f172a;
 border-radius:10px;
 padding:13px;
 font-size:13px;
 line-height:2;
}

.status{
 text-align:center;
 font-weight:bold;
 padding:12px;
 border-radius:10px;
 background:#334155;
}

.row{
 display:flex;
 justify-content:space-between;
 gap:8px;
 align-items:center;
}

.value{
 color:#38bdf8;
 font-weight:bold;
}

.sliderBox{
 background:#0f172a;
 padding:14px;
 border-radius:10px;
 margin-bottom:12px;
}

input[type=range]{
 width:100%;
 margin:16px 0;
 accent-color:#38bdf8;
}

button{
 border:0;
 border-radius:10px;
 color:white;
 padding:14px;
 font-weight:bold;
 cursor:pointer;
 touch-action:none;
 user-select:none;
 -webkit-user-select:none;
}

button.pressed{
 filter:brightness(1.3);
}

.blue{background:#2563eb}
.red{background:#dc2626}
.green{background:#16a34a}

.btnGrid{
 display:grid;
 grid-template-columns:1fr 1fr;
 gap:10px;
}

.full{
 width:100%;
 margin-top:12px;
}

.controls{
 display:grid;
 grid-template-columns:repeat(3,1fr);
 gap:10px;
 max-width:350px;
 margin:20px auto;
}

.drive{
 background:#2563eb;
 height:75px;
}

.maju{grid-column:2;grid-row:1}
.kiri{grid-column:1;grid-row:2}
.kanan{grid-column:3;grid-row:2}
.mundur{grid-column:2;grid-row:3}

.stop{
 grid-column:2;
 grid-row:2;
 background:#dc2626;
}

.tcrtValue{
 font-size:45px;
 color:#38bdf8;
 text-align:center;
 font-weight:bold;
}

.finish{
 background:#854d0e;
 color:#fef08a;
}

.white{
 background:#166534;
 color:#dcfce7;
}
</style>
</head>

<body>
<div class="wrap">

<h2>MAZE SOLVER</h2>
<p class="subtitle">
ESP32 - HC-SR04 - TCRT5000 - DRV8833
</p>

<div class="card">
 <div class="status" id="connection">
 Menghubungkan ESP32...
 </div>
</div>

<!-- SENSOR ULTRASONIK -->
<div class="card">
<h3>Monitoring HC-SR04</h3>

<div class="grid">
 <div class="sensor">
  <div class="label">DEPAN</div>
  <div class="num" id="depan">--</div>
  <div class="small">cm</div>
 </div>

 <div class="sensor">
  <div class="label">KIRI</div>
  <div class="num" id="kiri">--</div>
  <div class="small">cm</div>
 </div>

 <div class="sensor">
  <div class="label">KANAN</div>
  <div class="num" id="kanan">--</div>
  <div class="small">cm</div>
 </div>
</div>
</div>

<!-- TCRT5000 -->
<div class="card">

<h3>Kalibrasi TCRT5000</h3>

<p class="small">Nilai ADC real-time</p>

<div class="tcrtValue" id="adc">--</div>

<div class="status" id="tcrtStatus">
BELUM TERKALIBRASI
</div>

<div class="info" style="margin-top:12px">
 Sampel Putih:
 <b id="putih">--</b><br>

 Sampel Hitam:
 <b id="hitam">--</b><br>

 Ambang:
 <b id="ambang">--</b><br>

 Selisih ADC:
 <b id="selisih">--</b>
</div>

<div class="btnGrid" style="margin-top:12px">

 <button class="blue"
 onclick="kalibrasiTCRT('putih')">
  KALIBRASI PUTIH
 </button>

 <button class="blue"
 onclick="kalibrasiTCRT('hitam')">
  KALIBRASI HITAM
 </button>

</div>

<p class="small" id="tcrtMessage">
Kalibrasi awal: putih 0, hitam 56.
</p>

</div>

<!-- MOTOR -->
<div class="card">

<h3>Kalibrasi Dua Roda</h3>

<div class="sliderBox">
 <div class="row">
  <b>MOTOR KIRI</b>
  <span class="value" id="leftValue">255</span>
 </div>

 <input type="range" id="left"
 min="0" max="255" value="255"
 oninput="updatePWM()">
</div>

<div class="sliderBox">
 <div class="row">
  <b>MOTOR KANAN</b>
  <span class="value" id="rightValue">244</span>
 </div>

 <input type="range" id="right"
 min="0" max="255" value="244"
 oninput="updatePWM()">
</div>

<div class="btnGrid">

 <button class="blue" data-mode="uji_kiri">
 TES RODA KIRI
 </button>

 <button class="blue" data-mode="uji_kanan">
 TES RODA KANAN
 </button>

 <button class="blue" data-mode="maju">
 TES BERSAMA
 </button>

 <button class="red" onclick="stopMotor()">
 STOP
 </button>

</div>

<button class="green full"
onclick="simpanPWM()">
 SIMPAN KALIBRASI MOTOR
</button>

<p class="small" id="saveMessage"></p>

</div>

<!-- KONTROL ROBOT -->
<div class="card">

<h3>Kontrol Robot Manual</h3>

<div class="controls">

 <button class="drive maju"
 data-mode="maju">
 ▲ MAJU
 </button>

 <button class="drive kiri"
 data-mode="belok_kiri">
 ◀ KIRI
 </button>

 <button class="stop"
 onclick="stopMotor()">
 STOP
 </button>

 <button class="drive kanan"
 data-mode="belok_kanan">
 KANAN ▶
 </button>

 <button class="drive mundur"
 data-mode="mundur">
 ▼ MUNDUR
 </button>

</div>

<p class="small">
Tekan dan tahan tombol.
Lepaskan untuk menghentikan motor.
</p>

<div class="info">
 Status:
 <b id="motorStatus">STOP</b><br>
 PWM Kiri:
 <b id="pwmAktifKiri">0</b><br>
 PWM Kanan:
 <b id="pwmAktifKanan">0</b>
</div>

</div>

<p class="small">
WiFi MazeSolver - 192.168.4.1
</p>

</div>

<script>
let activeMode=null;
let activeButton=null;
let requestBusy=false;
let stopPending=false;
let initialized=false;

const left=document.getElementById('left');
const right=document.getElementById('right');

function updatePWM(){
 document.getElementById('leftValue').textContent=left.value;
 document.getElementById('rightValue').textContent=right.value;
}

async function sendMotor(){
 if(requestBusy)return;
 requestBusy=true;

 try{
  if(stopPending){
   stopPending=false;
   await fetch('/api/stop',{
    method:'POST',
    cache:'no-store'
   });
  }

  if(activeMode){
   let url='/api/drive?mode='+
   encodeURIComponent(activeMode)+
   '&l='+left.value+
   '&r='+right.value;

   await fetch(url,{
    method:'POST',
    cache:'no-store'
   });
  }
 }catch(e){
  document.getElementById('connection').textContent='Koneksi terganggu';
 }finally{
  requestBusy=false;
  if(stopPending)sendMotor();
 }
}

function startMotor(e){
 e.preventDefault();
 if(activeMode)stopMotor();

 activeButton=e.currentTarget;
 activeMode=activeButton.dataset.mode;
 activeButton.classList.add('pressed');

 try{
  activeButton.setPointerCapture(e.pointerId);
 }catch(e){}

 sendMotor();
}

function stopMotor(){
 activeMode=null;
 if(activeButton) activeButton.classList.remove('pressed');
 activeButton=null;
 stopPending=true;
 sendMotor();
}

document.querySelectorAll('[data-mode]').forEach(b=>{
 b.addEventListener('pointerdown',startMotor);
 b.addEventListener('pointerup',stopMotor);
 b.addEventListener('pointercancel',stopMotor);
 b.addEventListener('lostpointercapture',()=>{
  if(activeMode)stopMotor();
 });
});

setInterval(()=>{
 if(activeMode)sendMotor();
},180);

window.addEventListener('blur',()=>{
 if(activeMode)stopMotor();
});

document.addEventListener('visibilitychange',()=>{
 if(document.hidden && activeMode)stopMotor();
});

async function simpanPWM(){
 stopMotor();
 try{
  let r=await fetch(
   '/api/save?l='+left.value+'&r='+right.value,
   {method:'POST'}
  );
  if(!r.ok)throw new Error();
  document.getElementById('saveMessage').textContent='Kalibrasi PWM tersimpan';
 }catch(e){
  document.getElementById('saveMessage').textContent='Gagal menyimpan PWM';
 }
}

async function kalibrasiTCRT(warna){
 stopMotor();
 try{
  let r=await fetch('/api/tcrt/'+warna,{method:'POST'});
  let d=await r.json();
  document.getElementById('tcrtMessage').textContent=d.pesan;
  updateData();
 }catch(e){
  document.getElementById('tcrtMessage').textContent='Kalibrasi gagal';
 }
}

function jarak(n){
 return (n===null || n===undefined)?'--':Number(n).toFixed(1);
}

async function updateData(){
 try{
  const response=await fetch('/api/data',{cache:'no-store'});
  if(!response.ok)throw new Error();
  let d=await response.json();

  document.getElementById('depan').textContent=jarak(d.depan);
  document.getElementById('kiri').textContent=jarak(d.kiri);
  document.getElementById('kanan').textContent=jarak(d.kanan);

  document.getElementById('adc').textContent=(d.adc!==undefined)?d.adc:'--';
  document.getElementById('putih').textContent=(d.putih!==undefined)?d.putih:'--';
  document.getElementById('hitam').textContent=(d.hitam!==undefined)?d.hitam:'--';
  document.getElementById('ambang').textContent=(d.ambang!==undefined)?d.ambang:'--';
  document.getElementById('selisih').textContent=Math.abs((d.hitam||0)-(d.putih||0));

  let status=document.getElementById('tcrtStatus');
  if(!d.tcrtValid){
   status.textContent='KALIBRASI BELUM VALID';
   status.className='status';
  }else if(d.garisHitam){
   status.textContent='GARIS HITAM / FINISH';
   status.className='status finish';
  }else{
   status.textContent='PERMUKAAN PUTIH';
   status.className='status white';
  }

  document.getElementById('motorStatus').textContent=(d.mode || 'STOP').toUpperCase();
  document.getElementById('pwmAktifKiri').textContent=(d.pwmKiri!==undefined)?d.pwmKiri:0;
  document.getElementById('pwmAktifKanan').textContent=(d.pwmKanan!==undefined)?d.pwmKanan:0;
  document.getElementById('connection').textContent='ESP32 TERHUBUNG';

  if(!initialized){
   left.value=d.savedKiri;
   right.value=d.savedKanan;
   updatePWM();
   initialized=true;
  }
 }catch(e){
  document.getElementById('connection').textContent='ESP32 TERPUTUS';
 }
}

updateData();
setInterval(updateData,500);
</script>

</body>
</html>
)HTML";

// ======================================
// FUNGSI PWM
// ======================================
void setupPWM(){
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(AIN1, PWM_FREQ, PWM_BITS);
  ledcAttach(AIN2, PWM_FREQ, PWM_BITS);
  ledcAttach(BIN1, PWM_FREQ, PWM_BITS);
  ledcAttach(BIN2, PWM_FREQ, PWM_BITS);
#else
  ledcSetup(CH_A1, PWM_FREQ, PWM_BITS);
  ledcSetup(CH_A2, PWM_FREQ, PWM_BITS);
  ledcSetup(CH_B1, PWM_FREQ, PWM_BITS);
  ledcSetup(CH_B2, PWM_FREQ, PWM_BITS);

  ledcAttachPin(AIN1, CH_A1);
  ledcAttachPin(AIN2, CH_A2);
  ledcAttachPin(BIN1, CH_B1);
  ledcAttachPin(BIN2, CH_B2);
#endif
}

void tulisPWM(int pin, int ch, int duty){
  duty = constrain(duty, 0, 255);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  ledcWrite(ch, duty);
#endif
}

void pwmOff(){
  tulisPWM(AIN1, CH_A1, 0);
  tulisPWM(AIN2, CH_A2, 0);
  tulisPWM(BIN1, CH_B1, 0);
  tulisPWM(BIN2, CH_B2, 0);
}

void motorStop(){
  pwmOff();
  digitalWrite(STBY, LOW);

  modeMotor = "stop";
  pwmAktifKiri = 0;
  pwmAktifKanan = 0;
}

// ======================================
// KONTROL SATU RODA
// ======================================
void setRoda(int p1, int ch1,
             int p2, int ch2,
             int pwm){
  pwm = constrain(pwm, -255, 255);

  if(pwm > 0){
    tulisPWM(p2, ch2, 0);
    tulisPWM(p1, ch1, pwm);
  }else if(pwm < 0){
    tulisPWM(p1, ch1, 0);
    tulisPWM(p2, ch2, -pwm);
  }else{
    tulisPWM(p1, ch1, 0);
    tulisPWM(p2, ch2, 0);
  }
}

// ======================================
// KONTROL MOTOR DUA RODA
// ======================================
void motorDrive(String mode, int l, int r){
  l = constrain(l, 0, 255);
  r = constrain(r, 0, 255);

  int kiri = 0;
  int kanan = 0;

  if(mode == "uji_kiri"){
    kiri = l;
  }
  else if(mode == "uji_kanan"){
    kanan = r;
  }
  else if(mode == "maju"){
    kiri = l;
    kanan = r;
  }
  else if(mode == "mundur"){
    kiri = -l;
    kanan = -r;
  }
  else if(mode == "belok_kiri"){
    kiri = -l;
    kanan = r;
  }
  else if(mode == "belok_kanan"){
    kiri = l;
    kanan = -r;
  }
  else{
    motorStop();
    return;
  }

  if(BALIK_KIRI) kiri = -kiri;
  if(BALIK_KANAN) kanan = -kanan;

  if(kiri == 0 && kanan == 0){
    motorStop();
    return;
  }

  if(modeMotor != "stop" && modeMotor != mode){
    pwmOff();
    digitalWrite(STBY, LOW);
    delay(35);
  }

  digitalWrite(STBY, HIGH);

  setRoda(AIN1, CH_A1, AIN2, CH_A2, kiri);
  setRoda(BIN1, CH_B1, BIN2, CH_B2, kanan);

  modeMotor = mode;
  pwmAktifKiri = abs(kiri);
  pwmAktifKanan = abs(kanan);

  lastMotor = millis();
}

// ======================================
// BACA HC-SR04
// ======================================
float bacaJarak(int trig, int echo){
  digitalWrite(trig, LOW);
  delayMicroseconds(3);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  unsigned long durasi = pulseIn(
    echo, HIGH, 25000UL
  );

  if(durasi == 0) return -1;

  float cm = durasi * 0.0343f / 2.0f;
  cm *= KALIBRASI_HCSR;

  if(cm < 2 || cm > 400) return -1;

  return cm;
}

// ======================================
// BACA ADC TCRT5000
// ======================================
int bacaTCRT(int jumlah = 16){
  long total = 0;
  for(int i = 0; i < jumlah; i++){
    total += analogRead(PIN_TCRT);
    delayMicroseconds(200);
  }
  return total / jumlah;
}

// ======================================
// HITUNG KALIBRASI TCRT
// ======================================
void hitungKalibrasiTCRT(){
  int selisih = abs(adcHitam - adcPutih);
  ambangTCRT = (adcHitam + adcPutih) / 2;
  hitamLebihTinggi = (adcHitam > adcPutih);
  tcrtValid = (selisih >= MIN_SELISIH_TCRT);

  if(!tcrtValid){
    garisHitam = false;
  }
}

// ======================================
// DETEKSI GARIS DENGAN HYSTERESIS
// ======================================
void updateTCRT(){
  nilaiTCRT = bacaTCRT();

  if(!tcrtValid){
    garisHitam = false;
    return;
  }

  int batasAtas = ambangTCRT + HYSTERESIS_TCRT;
  int batasBawah = ambangTCRT - HYSTERESIS_TCRT;

  if(hitamLebihTinggi){
    if(!garisHitam && nilaiTCRT >= batasAtas){
      garisHitam = true;
    }
    if(garisHitam && nilaiTCRT <= batasBawah){
      garisHitam = false;
    }
  }else{
    if(!garisHitam && nilaiTCRT <= batasBawah){
      garisHitam = true;
    }
    if(garisHitam && nilaiTCRT >= batasAtas){
      garisHitam = false;
    }
  }
}

// ======================================
// HELPER CORS & JSON
// ======================================
void sendCORS(){
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

void handleOptions(){
  sendCORS();
  server.send(204);
}

String jsonJarak(float x){
  if(x < 0) return "null";
  return String(x, 1);
}

// ======================================
// API DATA JSON
// ======================================
void apiData(){
  String json = "{";
  json += "\"depan\":" + jsonJarak(jarakDepan) + ",";
  json += "\"kiri\":" + jsonJarak(jarakKiri) + ",";
  json += "\"kanan\":" + jsonJarak(jarakKanan) + ",";
  json += "\"adc\":" + String(nilaiTCRT) + ",";
  json += "\"putih\":" + String(adcPutih) + ",";
  json += "\"hitam\":" + String(adcHitam) + ",";
  json += "\"ambang\":" + String(ambangTCRT) + ",";
  json += "\"tcrtValid\":" + String(tcrtValid ? "true" : "false") + ",";
  json += "\"garisHitam\":" + String(garisHitam ? "true" : "false") + ",";
  json += "\"mode\":\"" + modeMotor + "\",";
  json += "\"pwmKiri\":" + String(pwmAktifKiri) + ",";
  json += "\"pwmKanan\":" + String(pwmAktifKanan) + ",";
  json += "\"savedKiri\":" + String(pwmSimpanKiri) + ",";
  json += "\"savedKanan\":" + String(pwmSimpanKanan);
  json += "}";

  sendCORS();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// ======================================
// API DRIVE
// ======================================
void apiDrive(){
  sendCORS();
  String mode = server.arg("mode");
  mode.toLowerCase();

  if(
    mode != "maju" &&
    mode != "mundur" &&
    mode != "belok_kiri" &&
    mode != "belok_kanan" &&
    mode != "uji_kiri" &&
    mode != "uji_kanan"
  ){
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  if(!server.hasArg("l") || !server.hasArg("r")){
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  int l = constrain(server.arg("l").toInt(), 0, 255);
  int r = constrain(server.arg("r").toInt(), 0, 255);

  motorDrive(mode, l, r);
  server.send(200, "application/json", "{\"ok\":true}");
}

// ======================================
// API STOP
// ======================================
void apiStop(){
  sendCORS();
  motorStop();
  server.send(200, "application/json", "{\"ok\":true}");
}

// ======================================
// SIMPAN PWM MOTOR
// ======================================
void apiSave(){
  sendCORS();
  motorStop();

  if(!server.hasArg("l") || !server.hasArg("r")){
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  int l = constrain(server.arg("l").toInt(), 0, 255);
  int r = constrain(server.arg("r").toInt(), 0, 255);

  prefs.begin("maze-motor", false);
  prefs.putUChar("left", l);
  prefs.putUChar("right", r);
  prefs.end();

  pwmSimpanKiri = l;
  pwmSimpanKanan = r;

  server.send(200, "application/json", "{\"ok\":true}");
}

// ======================================
// API KALIBRASI TCRT5000
// ======================================
void apiKalibrasiTCRT(bool hitam){
  sendCORS();
  motorStop();

  int sampel = bacaTCRT(48);
  nilaiTCRT = sampel;

  // Simpan sampel yang dibaca ke memory dan Preferences secara atomik
  prefs.begin("maze-tcrt", false);
  if(hitam){
    adcHitam = sampel;
    prefs.putInt("hitam", adcHitam);
  }else{
    adcPutih = sampel;
    prefs.putInt("putih", adcPutih);
  }
  prefs.end();

  hitungKalibrasiTCRT();
  updateTCRT();

  String pesan;
  if(tcrtValid){
    pesan = "Kalibrasi berhasil dan tersimpan.";
  }else{
    pesan = "Sampel tersimpan, namun selisih ADC < 20. Silakan kalibrasi warna lainnya.";
  }

  Serial.printf(
    "TCRT Putih=%d Hitam=%d Ambang=%d Valid=%d\n",
    adcPutih, adcHitam, ambangTCRT, tcrtValid
  );

  String response = "{\"ok\":";
  response += (tcrtValid ? "true" : "false");
  response += ",\"pesan\":\"" + pesan + "\"}";

  server.send(200, "application/json", response);
}

// ======================================
// CAPTIVE PORTAL
// ======================================
void redirectPortal(){
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

// ======================================
// SETUP
// ======================================
void setup(){
  Serial.begin(115200);
  delay(500);

  // Motor OFF saat boot
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

  // HC-SR04
  pinMode(TRIG_DEPAN, OUTPUT);
  pinMode(ECHO_DEPAN, INPUT);

  pinMode(TRIG_KIRI, OUTPUT);
  pinMode(ECHO_KIRI, INPUT);

  pinMode(TRIG_KANAN, OUTPUT);
  pinMode(ECHO_KANAN, INPUT);

  digitalWrite(TRIG_DEPAN, LOW);
  digitalWrite(TRIG_KIRI, LOW);
  digitalWrite(TRIG_KANAN, LOW);

  // TCRT5000
  pinMode(PIN_TCRT, INPUT);
  analogReadResolution(12);

  // Load hasil kalibrasi TCRT
  prefs.begin("maze-tcrt", true);
  adcPutih = prefs.getInt("putih", 0);
  adcHitam = prefs.getInt("hitam", 56);
  prefs.end();

  hitungKalibrasiTCRT();
  updateTCRT();

  // Load PWM motor
  prefs.begin("maze-motor", true);
  pwmSimpanKiri = prefs.getUChar("left", 255);
  pwmSimpanKanan = prefs.getUChar("right", 244);
  prefs.end();

  // WIFI ACCESS POINT
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASS);

  dns.start(53, "*", apIP);

  // Web dashboard & API
  server.enableCORS(true);

  server.on("/", HTTP_GET, [](){
    server.send_P(200, "text/html", WEB);
  });

  server.on("/api/data", HTTP_GET, apiData);
  server.on("/api/data", HTTP_OPTIONS, handleOptions);

  server.on("/api/drive", HTTP_POST, apiDrive);
  server.on("/api/drive", HTTP_OPTIONS, handleOptions);

  server.on("/api/stop", HTTP_POST, apiStop);
  server.on("/api/stop", HTTP_OPTIONS, handleOptions);

  server.on("/api/save", HTTP_POST, apiSave);
  server.on("/api/save", HTTP_OPTIONS, handleOptions);

  server.on("/api/tcrt/putih", HTTP_POST, [](){
    apiKalibrasiTCRT(false);
  });
  server.on("/api/tcrt/putih", HTTP_OPTIONS, handleOptions);

  server.on("/api/tcrt/hitam", HTTP_POST, [](){
    apiKalibrasiTCRT(true);
  });
  server.on("/api/tcrt/hitam", HTTP_OPTIONS, handleOptions);

  // Captive Portal
  server.on("/generate_204", HTTP_GET, redirectPortal);
  server.on("/gen_204", HTTP_GET, redirectPortal);
  server.on("/hotspot-detect.html", HTTP_GET, redirectPortal);
  server.on("/connecttest.txt", HTTP_GET, redirectPortal);
  server.on("/ncsi.txt", HTTP_GET, redirectPortal);

  server.onNotFound(redirectPortal);

  server.begin();

  Serial.println();
  Serial.println("MAZE SOLVER READY");

  Serial.print("WiFi: ");
  Serial.println(AP_SSID);

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.printf(
    "TCRT PUTIH=%d HITAM=%d AMBANG=%d\n",
    adcPutih, adcHitam, ambangTCRT
  );

  Serial.printf(
    "PWM KIRI=%d KANAN=%d\n",
    pwmSimpanKiri, pwmSimpanKanan
  );
}

// ======================================
// LOOP
// ======================================
void loop(){
  dns.processNextRequest();
  server.handleClient();

  // Pembacaan TCRT
  if(millis() - lastTCRT >= 70){
    lastTCRT = millis();
    updateTCRT();
  }

  // Motor fail-safe
  if(modeMotor != "stop"){
    if(
      WiFi.softAPgetStationNum() == 0 ||
      millis() - lastMotor > MOTOR_TIMEOUT
    ){
      motorStop();
    }
  }

  // Pembacaan HC-SR04 bergantian
  if(millis() - lastHCSR >= 80){
    lastHCSR = millis();

    if(indexHCSR == 0){
      jarakDepan = bacaJarak(TRIG_DEPAN, ECHO_DEPAN);
    }else if(indexHCSR == 1){
      jarakKiri = bacaJarak(TRIG_KIRI, ECHO_KIRI);
    }else{
      jarakKanan = bacaJarak(TRIG_KANAN, ECHO_KANAN);
    }

    indexHCSR = (indexHCSR + 1) % 3;
  }

  delay(1);
}
