# CODING RULES & ARCHITECTURAL GUIDELINES
**Project**: ESP32 Power Meter & Server Monitor  
**Last Updated**: September 2026

Dokumen ini adalah pedoman baku dan aturan arsitektur untuk pengembangan project **Power Meter & Server Monitor**. Setiap penambahan fitur, refactoring, atau modifikasi kode wajib mematuhi aturan di bawah ini agar kode tetap stabil, performan, dan konsisten.

---

## 1. Arsitektur & Struktur Direktori

Project menggunakan framework **Arduino ESP32** dengan build system **PlatformIO**. Struktur modular berbasis library di dalam direktori `lib/`:

```
Power-meter/
├── src/
│   └── main.cpp               # Setup & non-blocking execution loop
├── lib/
│   ├── LCD/                   # Driver ST7789 TFT, layout dashboard, OTA animation
│   ├── PZEM/                  # Driver Modbus PZEM-004T v3.0 & metrics cache
│   ├── SavedWifi/             # WiFi STA/AP management & NVS persistent storage
│   ├── Web/                   # WebServer HTTP endpoints, REST API, Web OTA
│   ├── index_html/            # Single-page Web UI (index.html & PROGMEM header)
│   └── Devices/               # Unique device identification & mDNS/UDP discovery
├── platformio.ini             # Hardware definitions, build flags, library dependencies
├── README.md                  # Dokumentasi umum & panduan endpoint
└── CODING_RULES.md            # Dokumen standar dan aturan coding ini
```

### Aturan Modularitas:
- **Separation of Concerns**: Logika display berada di `lib/LCD`, komunikasi sensor di `lib/PZEM`, koneksi jaringan di `lib/SavedWifi`, dan endpoint HTTP di `lib/Web`.
- **Minimal Coupling**: `main.cpp` bertindak sebagai koordinator orkestrasi non-blocking antar-modul, bukan tempat menumpuk logika driver.

---

## 2. Hardware & Pin Allocation Registry (STRICT)

Dilarang mengubah atau memetakan ulang GPIO tanpa memperbarui `platformio.ini`, `CODING_RULES.md`, dan `README.md`.

### A. PZEM-004T v3.0 (AC Power Sensor)
| Pin PZEM | GPIO ESP32 | Mode / Fungsi | Aturan Khusus |
|----------|------------|---------------|---------------|
| **TX**   | **GPIO 16** | RX2 (Hardware Serial2) | Wajib cross-connection ke RX PZEM |
| **RX**   | **GPIO 17** | TX2 (Hardware Serial2) | Wajib cross-connection ke TX PZEM |
| **5V**   | **5V / VIN** | Power Optocoupler | Catu daya dari 5V USB/regulator |
| **GND**  | **GND**      | Common Ground | Harus satu ground dengan ESP32 |

> [!IMPORTANT]
> **DILARANG MENGGUNAKAN SoftwareSerial** pada ESP32. Wajib menggunakan `HardwareSerial(2)` (`Serial2.begin(9600, SERIAL_8N1, 16, 17)`).

### B. Layar ST7789 TFT LCD 240x240 (SPI)
| Pin LCD | GPIO ESP32 | Konfigurasi PlatformIO | Keterangan |
|---------|------------|------------------------|------------|
| **VCC** | **3.3V**   | -                      | **Wajib 3.3V**. Jangan hubungkan ke 5V! |
| **GND** | **GND**    | -                      | Ground |
| **SCL** | **GPIO 18** | `-D TFT_SCLK=18`       | SPI Hardware Clock |
| **SDA** | **GPIO 23** | `-D TFT_MOSI=23`       | SPI Master Out Slave In |
| **RES** | **GPIO 26** | `-D TFT_RST=26`        | Hardware Reset |
| **DC**  | **GPIO 27** | `-D TFT_DC=27`         | Data / Command Selection |
| **BLK** | **GPIO 25** | `-D TFT_BL=25`         | Backlight (LEDC PWM Channel 0, 5kHz) |
| **CS**  | **-**      | `-D TFT_CS=-1`         | Modul tanpa pin CS (di-ground di modul) |

### C. Sensor Sentuh TTP223 & Tombol Navigasi
| Komponen | GPIO ESP32 | Mode Logika | Keterangan |
|----------|------------|-------------|------------|
| **TTP223 SIG** | **GPIO 4** | Active-HIGH (`INPUT_PULLDOWN`) | Sensor sentuh kapasitif navigasi layar |
| **BOOT Switch**| **GPIO 0** | Active-LOW (`INPUT_PULLUP`) | Tombol onboard ESP32 (backup trigger) |

---

## 3. Aturan Layar LCD & Desain Visual (TFT ST7789)

### A. Zero-Flicker Rendering Rule
1. **Dilarang keras memakai `tft.fillRect(...)`** untuk menghapus teks lama pada loop periodik. Hal ini menyebabkan layar berkedip (*flickering*).
2. **Wajib menggunakan `tft.setTextPadding(width)`**:
   ```cpp
   tft.setTextPadding(168); // Otomatis mengisi background dan teks dalam 1 pass SPI
   tft.drawString(valString, x, y, font);
   tft.setTextPadding(0);   // Selalu reset padding setelah selesai
   ```
3. **Dirty-Checking Cache**: Hanya lakukan penulisan ke layar jika nilai berubah melebihi batas toleransi:
   ```cpp
   if (!prevInit || (fabs(m.power - prevPower) >= 0.1f)) {
       prevPower = m.power;
       // Gambar nilai baru
   }
   ```

### B. Standar Palet Warna Kontras Tinggi (Color Hunt Adaptation)
Layar TFT LCD memiliki lampu latar (*backlight*), sehingga warna abu-abu gelap akan tampak berkabut/pudar. Gunakan palet berikut:

```cpp
#define C_BG         0x0000   // Pure Pitch Black (Menghilangkan backlight wash)
#define C_CARD       0x1162   // #121c12 Deep Forest Card Surface
#define C_BORDER     0x4B65   // #4a6c34 Crisp Visible Olive Border
#define C_PRIMARY    0x6408   // #628141 Olive Green
#define C_ACCENT     0x8E8C   // #8bae66 Vibrant Sage Green
#define C_ACCENT_HI  0x9FEA   // #9de754 High-Luminance Lime (Satuan & Highlight)
#define C_TEXT       0xEEF5   // #ebd5ab Warm Cream untuk Label & Header
#define C_VALUE      0xFFFF   // Pure White untuk Angka Metrik Utama (Super Tajam)
#define C_DIM        0x7BEF   // Neutral Muted Gray untuk status offline
#define C_ALERT      0xF9A6   // #f83434 High-Contrast Red Alert
```

### C. Kontrol Backlight (LEDC PWM)
- Dilarang menggunakan `digitalWrite(TFT_BL, ...)` karena pin sudah di-attach ke peripheral LEDC PWM.
- Wajib menggunakan `ledcWrite(ledChannel, brightness)` (rentang `0` s.d. `255`).
- Nilai kecerahan default optimal: **240** (~94% PWM) untuk mencegah *backlight bleed*.

---

## 4. Aturan Loop Eksekusi & Multitasking Non-Blocking

1. **DILARANG MENGGUNAKAN `delay()` di dalam `loop()`**:
   - Fungsi `delay()` akan menghentikan respons WebServer, menghambat penerimaan paket UDP, dan membuat pembacaan PZEM macet.
2. **Gunakan Polling Berbasis `millis()`**:
   ```cpp
   unsigned long currentMillis = millis();
   if (currentMillis - lastPzemRead >= 1000) {
       lastPzemRead = currentMillis;
       readPZEM();
       updatePowerMeterDisplay(getPZEMMetrics(), statusInfo);
   }
   ```
3. **Frekuensi Polling PZEM**:
   - Interval aman pembacaan PZEM adalah **1000 ms** (1 detik).
   - **Jangan polling lebih cepat dari 200 ms** karena protokol Modbus RTU PZEM membutuhkan jeda respon optocoupler.

---

## 5. Standar Web Portal & REST API

### A. Sinkronisasi Aset Web
- File sumber utama tampilan web adalah `lib/index_html/index.html`.
- File `lib/index_html/index_html.h` menyimpan string HTML ke flash PROGMEM menggunakan literal `R"rawliteral(...)rawliteral"`.
- **Setiap kali mengedit `index.html`, file `index_html.h` WAJIB disinkronkan kembali sebelum kompilasi.**

### B. Standar REST API
- Format respon data sistem & sensor wajib JSON valid:
  - Header: `Content-Type: application/json`
  - Tambahkan CORS header (`Access-Control-Allow-Origin: *`) pada endpoint publik.
- Endpoint aksi mutating (seperti reset energi atau ubah wifi) wajib menggunakan metode **POST**.

### C. Filosofi Web UI (Configuration-Only, No Aggressive Polling)
- **Web UI murni berfungsi sebagai portal konfigurasi**: Pengaturan WiFi, konfigurasi Beszel Hub, manajemen server (tambah/hapus/tes), dan Web OTA.
- **Tugas monitoring real-time dipegang penuh oleh layar LCD TFT**: Dilarang menambahkan polling periodik (`setInterval`) agresif di frontend web UI.
- Pemuatan data di Web UI hanya dilakukan **sekali saat halaman dibuka** (*initial load*) atau secara **on-demand** saat pengguna menekan tombol aksi / tautan *"Segarkan"*. Hal ini menjaga socket TCP lwIP ESP32 tetap longgar dan responsif untuk proses input data.

### D. Web OTA (Over-The-Air Update)
- Gunakan implementasi native `<Update.h>` berbasis multipart POST di endpoint `/update`.
- Jangan mengganti implementasi ini dengan library pihak ketiga yang membebani RAM (seperti ElegantOTA lama).
- Proses OTA wajib menampilkan visual progres di LCD melalui:
  - `drawOtaStart()`
  - `drawOtaProgress(percent)`
  - `drawOtaSuccess()`
  - `drawOtaError(msg)`

---

## 6. Manajemen Memori & Flash

- **RAM Budget**: Batas aman heap sisa tidak boleh di bawah 20 KB. Hindari alokasi buffer dinamis besar di dalam fungsi yang dipanggil berulang kali.
- **Buffer Formatting**: Gunakan `snprintf()` dengan buffer statis atau stack berukuran terukur daripada manipulasi `String` terus-menerus.
- **Partisi LittleFS**: Digunakan untuk penyimpanan data konfigurasi/file lokal. Format partisi tersedia melalui endpoint `/sys/format`.

---

## 7. Checklist Sebelum Commit / Menambahkan Fitur Baru

- [ ] Apakah ada pemanggilan `delay()` yang dapat memblokir `loop()`? *(Harus dihilangkan)*
- [ ] Apakah pin GPIO yang digunakan bertabrakan dengan pin LCD atau PZEM?
- [ ] Apakah perubahan warna layar LCD tetap mematuhi rasio kontras tinggi?
- [ ] Apakah tampilan teks di LCD menggunakan `setTextPadding()` tanpa `fillRect` berkedip?
- [ ] Jika memodifikasi UI web, apakah `index_html.h` sudah disinkronkan dengan `index.html`?
- [ ] Apakah project lolos kompilasi bersih (`pio run`) dengan status SUCCESS?
