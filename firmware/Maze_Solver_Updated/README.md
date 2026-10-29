# Firmware MazeBot EduKit (ESP32)

Repositori firmware resmi untuk **MazeBot EduKit** — robot pemecah labirin otonom berbasis ESP32, dirancang dengan kaidah arsitektur perangkat lunak tertanam (*clean code*) menggunakan **PlatformIO**.

Firmware ini menyematkan antarmuka web modern dari repositori [CakUrang-dev/mazebot-edukit](https://github.com/CakUrang-dev/mazebot-edukit) langsung ke dalam memori Flash ESP32 (`PROGMEM`), memungkinkan kontrol nirkabel, kalibrasi sensor dua tahap, dan telemetri langsung secara mandiri melalui WiFi SoftAP.

---

## 🏗️ Arsitektur Proyek (PlatformIO Modular)

Struktur kode diatur dengan prinsip *Separation of Concerns*:

```text
firmware-mazebot/
├── platformio.ini       # Konfigurasi PlatformIO & target ESP32 Dev Module
├── src/
│   ├── config.h         # Definisi pinout, konstanta frekuensi, & AP SoftAP
│   ├── settings.h/.cpp  # Manajemen NVS Preferences (PWM Motor & TCRT5000)
│   ├── motor.h/.cpp     # Driver DRV8833 (PWM 18 kHz, soft-ramp 35 ms, fail-safe 650 ms)
│   ├── sensor.h/.cpp    # HC-SR04 non-blocking round-robin & ADC TCRT5000 (GPIO 32)
│   ├── web_page.h       # Dashboard HTML EduKit (PROGMEM Flash)
│   ├── web_server.h/.cpp# WebServer REST API & DNS Captive Portal
│   └── main.cpp         # Inisialisasi sistem & event loop ringkas (< 50 baris)
└── README.md            # Dokumentasi teknis & panduan instalasi
```

---

## 🔌 Skema Pinout Hardware ESP32

| Komponen | Pin Fungsi | GPIO ESP32 | Keterangan |
| :--- | :--- | :--- | :--- |
| **HC-SR04 Depan** | TRIG / ECHO | **GPIO 4 / 17** | Deteksi rintangan dinding depan |
| **HC-SR04 Kiri** | TRIG / ECHO | **GPIO 19 / 18** | Navigasi dinding kiri |
| **HC-SR04 Kanan** | TRIG / ECHO | **GPIO 14 / 13** | Navigasi dinding kanan |
| **TCRT5000** | Output Analog | **GPIO 32** | ADC1_CH4 (12-bit: 0 - 4095) deteksi garis |
| **DRV8833 Kiri** | AIN1 / AIN2 | **GPIO 27 / 26** | PWM 18 kHz (Ch 0 & 1) |
| **DRV8833 Kanan**| BIN1 / BIN2 | **GPIO 25 / 33** | PWM 18 kHz (Ch 2 & 3) |
| **DRV8833 Standby**| STBY | **GPIO 23** | Active HIGH (Sleep saat robot berhenti) |

---

## ⚡ Fitur Utama & Keunggulan

1. **Dashboard Web EduKit Mandiri (PROGMEM):**
   - Menjalankan antarmuka [mazebot-edukit](https://github.com/CakUrang-dev/mazebot-edukit) tanpa butuh koneksi internet atau server eksternal.
   - Captive Portal otomatis saat tersambung ke WiFi `MazeSolver`.
2. **Kalibrasi Dua Tahap TCRT5000:**
   - Kalibrasi permukaan putih dan garis hitam langsung dari antarmuka web.
   - Ambang batas (`threshold`) dan histeresis dihitung dinamis serta tersimpan permanen di NVS ESP32.
3. **Round-Robin Non-Blocking Ultrasonic:**
   - Pembacaan 3 sensor ultrasonik dilakukan bergantian dengan interval 80 ms untuk mengeliminasi gema silang (*cross-echo*).
   - Faktor koreksi teruji `10.0f / 9.1f` untuk presisi milimeter.
4. **Proteksi & Keamanan Gerak Motor:**
   - Frekuensi PWM 18 kHz senyap tanpa dering frekuensi audio (*audible whine*).
   - *Soft transition* (35 ms per step) mencegah selip ban dan lonjakan arus baterai.
   - *Fail-safe timer* otomatis menghentikan robot jika koneksi klien terputus lebih dari 650 ms.
5. **Dukungan Penuh CORS & REST API:**
   - Kompatibel dengan akses lintas domain (CORS `*`) dan preflight `OPTIONS` untuk integrasi simulator atau kontrol eksternal.

---

## 🌐 Spesifikasi REST API

| Method | Endpoint | Deskripsi | Parameter |
| :--- | :--- | :--- | :--- |
| `GET` | `/` | Sajikan antarmuka dashboard Web EduKit | - |
| `GET` | `/api/data` | Telemetri jarak, ADC TCRT, status kalibrasi, & PWM | - |
| `POST`| `/api/drive`| Menggerakkan robot manual atau otomatis | `mode`, `l`, `r` |
| `POST`| `/api/stop` | Menghentikan seluruh motor seketika | - |
| `POST`| `/api/save` | Menyimpan nilai PWM motor ke NVS | `l`, `r` (0-255) |
| `POST`| `/api/tcrt/putih` | Rekam sampel ADC permukaan putih ke NVS | - |
| `POST`| `/api/tcrt/hitam` | Rekam sampel ADC garis hitam finish ke NVS | - |
| `POST`| `/api/tcrt/reset` | Reset kalibrasi garis ke kondisi default | - |

---

## 🚀 Panduan Kompilasi & Upload (PlatformIO)

Pastikan telah menginstal [VS Code](https://code.visualstudio.com/) dengan ekstensi **PlatformIO IDE** (atau CLI `pio`):

```bash
# 1. Kloning repositori
git clone https://github.com/CakUrang-dev/firmware-mazebot.git
cd firmware-mazebot

# 2. Kompilasi firmware
pio run

# 3. Upload ke ESP32 (hubungkan kabel data USB)
pio run --target upload

# 4. Buka Serial Monitor (115200 baud)
pio device monitor
```

---

## 📶 Menghubungkan ke Robot

1. Hubungkan daya ESP32.
2. Sambungkan laptop/smartphone ke WiFi:
   - **SSID:** `MazeSolver`
   - **Password:** `12345678`
3. Buka browser dan arahkan ke `http://192.168.4.1` (atau pop-up Captive Portal akan terbuka otomatis).