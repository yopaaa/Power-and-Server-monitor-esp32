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
| `/sys/info` | GET | Mendapatkan status informasi sistem (chip ID, heap, uptime, WiFi, dll) |
| `/update` | POST | Upload firmware `.bin` secara multipart langsung (Web OTA) |
| `/` | GET | Halaman utama portal konfigurasi WiFi dan Web OTA Upload |

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

---

## ⚡ Rencana Sensor PZEM
- Membaca data tegangan (V), arus (A), daya (W), energi (kWh), frekuensi (Hz), dan power factor (PF).
- Menampilkan hasil pembacaan langsung di layar LCD TFT.

