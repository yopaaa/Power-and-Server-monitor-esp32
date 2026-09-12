# Power Meter & Server Monitor

Project Power Meter dan Server Monitor berbasis ESP32 dengan sensor PZEM dan layar LCD TFT ST7789.

## 🔌 Format Umum
- **Metode**: GET atau POST
- **Base URL**: `http://{{host}}`
- **Header Umum**: `Content-Type: application/x-www-form-urlencoded` untuk POST.

---

## 🛠️ Endpoints Sistem
| Endpoint | Metode | Deskripsi |
|----------|--------|-----------|
| `/sys/info` | GET | Mendapatkan status sistem (chip ID, heap, uptime, WiFi, dan metrik PZEM live) |
| `/sys/format` | GET | Format partisi LittleFS (menghapus seluruh file/gambar lama) |
| `/update` | POST | Upload firmware `.bin` secara multipart langsung (Web OTA) |
| `/pzem/reset` | POST | Reset akumulasi hitungan energi (kWh) pada sensor PZEM ke 0 |
| `/` | GET | Halaman utama portal dashboard sistem, WiFi config, dan Web OTA |

---

## 📌 Konfigurasi Pin Hardware (Wiring)

### ⚡ 1. Sensor PZEM-004T v3.0 (Serial2)
| Pin PZEM-004T | Pin ESP32 (GPIO) | Keterangan |
|---------------|------------------|------------|
| **TX** | **GPIO 16** (RX2) | Jalur data serial masuk ke ESP32 |
| **RX** | **GPIO 17** (TX2) | Jalur data serial keluar dari ESP32 |
| **5V** | **5V / VIN** | Catu daya modul optocoupler PZEM |
| **GND** | **GND** | Ground bersama (Common Ground) |

> [!NOTE]
> Pastikan pin RX PZEM disambung ke TX ESP32, dan pin TX PZEM disambung ke RX ESP32 (*crossed connection*).

---

### 🖥️ 2. Layar LCD TFT ST7789 (240 × 240 SPI)
| Pin Layar ST7789 | Pin ESP32 (GPIO) | Keterangan |
|------------------|------------------|------------|
| **VCC** | **3.3V** | Wajib 3.3V (jangan gunakan 5V) |
| **GND** | **GND** | Ground |
| **SCL / SCLK** | **GPIO 18** | SPI Clock |
| **SDA / MOSI** | **GPIO 23** | SPI Master Out Slave In |
| **RES / RST** | **GPIO 26** | Hardware Reset |
| **DC / RS** | **GPIO 27** | Data / Command Selection |
| **BLK / BL** | **GPIO 25** | Backlight LED (PWM Brightness 0-255) |
| **CS** | *-* | Tidak terhubung / GND (CS=-1 pada board tanpa CS) |

---

## 📶 WiFi API
| Endpoint | Metode | Parameter | Keterangan |
|----------|--------|-----------|------------|
| `/wifi/data` | GET | - | Ambil daftar konfigurasi WiFi tersimpan |
| `/wifi/add` | POST | `ssid`, `pass`, `static` (opsional) | Simpan/update konfigurasi WiFi |
| `/wifi/delete` | POST | `ssid` | Hapus WiFi yang tersimpan |
| `/wifi/test` | POST | `ssid`, `pass` | Uji koneksi ke access point |

**Contoh curl**:
```sh
curl -X POST http://{{host}}/wifi/add \
  -d "ssid=MyWiFi&pass=mypassword&static=false"
```

---

## 🖥️ LCD Display API
| Endpoint | Metode | Parameter | Keterangan |
|----------|--------|-----------|------------|
| `/lcd/backlight` | GET | `value` (0 - 255) | Atur kecerahan backlight LCD |
| `/lcd/clear` | GET | - | Bersihkan layar LCD |

