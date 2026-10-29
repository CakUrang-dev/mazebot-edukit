# MazeBot EduKit — ESP32 Firmware (Single-File)

Firmware robot navigasi labirin (*maze-solving autonomous robot*) berbasis ESP32 DevKit V1 dengan kontroler dual motor DRV8833 dan 3× sensor jarak ultrasonik HC-SR04 berbasis polling bergantian (round-robin). Firmware ini dilengkapi server web tertanam (*embedded web dashboard* di memori flash PROGMEM ~25 KB) dengan antarmuka dark minimalis, telemetri real-time, kontrol manual D-pad, kalibrasi TCRT5000 (line tracker/finish), dan kalibrasi motor PWM dengan penyimpanan NVS.

---

## ⚡ Fitur Utama

- **Embedded Web Dashboard (Zero CDN / 100% Offline):** Antarmuka web modern tersimpan langsung di flash ESP32 (`PROGMEM`), siap diakses saat perangkat terhubung ke WiFi robot.
- **Dual-Transport Telemetry:** Mendukung REST API HTTP (`/api/data`, `/api/drive`, `/api/stop`, `/api/save`, `/api/tcrt/*`) dengan header CORS.
- **Driver Motor DRV8833:**
  - Modulasi PWM frekuensi 18 kHz (bebas dengung audiosonik).
  - *Soft-transition* (delay 35 ms) saat pergantian mode untuk kestabilan traksi roda.
  - Fail-safe timeout 650 ms (motor otomatis berhenti jika komunikasi terputus atau tidak ada klien WiFi).
  - Manajemen daya via pin `STBY` (sleep otomatis saat mode STOP).
- **Sensor Ultrasonik Round-Robin:** Pembacaan bergantian 3 sensor HC-SR04 (Depan → Kiri → Kanan) setiap ~80 ms untuk menghindari interferensi gelombang suara.
- **Line Tracker TCRT5000 (GPIO 32):**
  - Kalibrasi putih/hitam dengan hysteresis (±6) untuk deteksi garis stabil.
  - Deteksi finish line (garis hitam) dengan status visual di dashboard.
- **Penyimpanan Kalibrasi Permanen (NVS/Preferences):**
  - Parameter PWM roda kiri dan kanan (`maze-motor` namespace)
  - Kalibrasi TCRT5000 putih/hitam (`maze-tcrt` namespace)

---

## 🔌 Konfigurasi Pinout Perangkat Keras

| Komponen | Pin Modul | GPIO ESP32 | Tipe / Keterangan |
|---|---|---|---|
| **HC-SR04 Depan** | TRIG | `GPIO 4` | Output (Pulsa 10 µs) |
| | ECHO | `GPIO 17` | Input (pulseIn) |
| **HC-SR04 Kiri** | TRIG | `GPIO 19` | Output (Pulsa 10 µs) |
| | ECHO | `GPIO 18` | Input (pulseIn) |
| **HC-SR04 Kanan** | TRIG | `GPIO 14` | Output (Pulsa 10 µs) |
| | ECHO | `GPIO 13` | Input (pulseIn) |
| **TCRT5000 Line Tracker** | DO / A0 | `GPIO 32` | Input ADC1_CH4 (12-bit) |
| **DRV8833 Motor Kiri** | AIN1 | `GPIO 27` | PWM Channel (18 kHz, 8-bit) |
| | AIN2 | `GPIO 26` | PWM Channel (18 kHz, 8-bit) |
| **DRV8833 Motor Kanan** | BIN1 | `GPIO 25` | PWM Channel (18 kHz, 8-bit) |
| | BIN2 | `GPIO 33` | PWM Channel (18 kHz, 8-bit) |
| **DRV8833 Standby** | STBY | `GPIO 23` | Output (Active HIGH) |

---

## 📶 Koneksi WiFi & Akses Dashboard

ESP32 beroperasi dalam mode **SoftAP (Access Point)** mandiri:

- **SSID WiFi:** `MazeSolver`
- **Password:** `12345678`
- **IP Address:** `192.168.4.1`
- **Captive Portal:** Otomatis membuka peramban pada perangkat Android, iOS, dan Windows setelah terkoneksi ke WiFi.
- **URL Dashboard:** `http://192.168.4.1/`

---

## 📡 Dokumentasi Endpoint REST API

Semua endpoint menyertakan header `Access-Control-Allow-Origin: *` untuk pengujian lintas domain dan mendukung preflight `OPTIONS`.

### 1. `GET /api/data`
Mengambil data telemetri jarak sensor, TCRT5000, dan status motor.
- **Respons (JSON):**
  ```json
  {
    "depan": 18.5,
    "kiri": 5.2,
    "kanan": 22.0,
    "adc": 1245,
    "putih": 0,
    "hitam": 56,
    "ambang": 28,
    "tcrtValid": false,
    "garisHitam": false,
    "mode": "STOP",
    "pwmKiri": 0,
    "pwmKanan": 0,
    "savedKiri": 255,
    "savedKanan": 244
  }
  ```
  - Nilai jarak `-1` / out-of-range dikirim sebagai `null`.
  - `adc` = nilai ADC real-time TCRT5000 (0-4095).
  - `tcrtValid` = `true` jika selisih putih/hitam ≥ 20.

### 2. `POST /api/drive`
Menjalankan motor dengan mode dan nilai PWM tertentu.
- **Format:** `application/x-www-form-urlencoded`
- **Parameter:**
  - `mode`: `maju` | `mundur` | `belok_kiri` | `belok_kanan` | `uji_kiri` | `uji_kanan`
  - `l`: Nilai PWM motor kiri (`0` - `255`)
  - `r`: Nilai PWM motor kanan (`0` - `255`)
- **Contoh Payload:** `mode=maju&l=220&r=220`

### 3. `POST /api/stop`
Menghentikan seluruh motor seketika (mengaktifkan mode standby pada DRV8833).
- **Respons:** `{"ok": true}`

### 4. `POST /api/save`
Menyimpan kalibrasi nilai PWM default ke NVS flash memory ESP32.
- **Parameter:** `l=<pwm_kiri>&r=<pwm_kanan>`
- **Contoh Payload:** `l=210&r=225`

### 5. `POST /api/tcrt/putih` & `POST /api/tcrt/hitam`
Mengambil sampel ADC TCRT5000 (48× rata-rata) dan menyimpannya sebagai referensi kalibrasi putih/hitam.
- **Respons:**
  ```json
  {"ok": true, "pesan": "Kalibrasi berhasil dan tersimpan."}
  ```
  atau jika selisih < 20:
  ```json
  {"ok": false, "pesan": "Sampel tersimpan, namun selisih ADC < 20. Silakan kalibrasi warna lainnya."}
  ```

---

## 🛠️ Panduan Kompilasi & Flash Firmware

### Menggunakan PlatformIO (Disarankan)
1. Clone repositori ini:
   ```bash
   git clone git@github.com:CakUrang-dev/firmware-mazebot.git
   cd firmware-mazebot
   ```
2. Buka folder proyek di **VS Code** yang sudah terpasang ekstensi **PlatformIO IDE**.
3. Hubungkan ESP32 DevKit V1 ke port USB komputer.
4. Klik tombol **PlatformIO: Build** (`✓`) dan **PlatformIO: Upload** (`→`) pada status bar bawah.

### Konfigurasi `platformio.ini`
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder
upload_speed = 921600

; Matikan log debug core agar firmware lebih kecil
build_flags =
    -DCORE_DEBUG_LEVEL=0
```

### Menggunakan Arduino IDE
1. Buka file `src/main.cpp` langsung di Arduino IDE (atau salin ke `firmware-mazebot.ino`).
2. Pilih Board: **ESP32 Dev Module**.
3. Pastikan library bawaan: `WiFi`, `WebServer`, `DNSServer`, `Preferences` (sudah included di core ESP32).
4. Kompilasi & Upload.

---

## 📁 Struktur Berkas (Single-File Architecture)

```
firmware-mazebot/
├── src/
│   └── main.cpp         # Semua logika: WiFi, WebServer, Motor, Sensor, TCRT, NVS, HTML
├── platformio.ini       # Konfigurasi environment PlatformIO ESP32
└── README.md            # Dokumentasi teknis proyek
```

**Catatan:** Arsitektur single-file (`main.cpp` ~26 KB) dipilih untuk kemudahan deploy, review, dan distribusi tanpa manajemen multi-file. Semua dependensi menggunakan library core ESP32 Arduino (tidak butuh library eksternal).

---

## 🎓 Capstone Project Context
Proyek ini merupakan bagian dari ekosistem **MazeBot EduKit** — Platform Robotika & IoT Edukatif untuk pembelajaran siswa jenjang SD, SMP, dan SMA berbasis Kurikulum Merdeka dan Capstone Design Teknik Elektro Universitas Negeri Malang.
