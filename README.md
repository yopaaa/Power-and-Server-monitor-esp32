# LCD Project API Documentation

Dokumentasi ini menjelaskan cara menggunakan API REST yang tersedia pada perangkat ESP32/ESP8266 dalam proyek "LCD". Semua permintaan ditujukan ke alamat IP perangkat (misalnya `192.168.2.182`). Ganti `{{host}}` dengan IP yang sesuai.

## 🔌 Format Umum
- **Metode**: GET atau POST
- **Base URL**: `http://{{host}}`
- **Header Umum**: `Content-Type: application/x-www-form-urlencoded` untuk POST biasa. Untuk upload file gunakan `multipart/form-data`.

---

## 🛠️ Endpoints Sistem
| Endpoint | Metode | Deskripsi |
|----------|--------|-----------|
| `/sys/info` | GET | Mendapatkan informasi sistem (versi, uptime, dll) |
| `/ota` | GET | Mengaktifkan update over-the-air (OTA) |
| `/` | GET | Halaman utama web server |

---

## 📁 File System API
| Endpoint | Metode | Parameter | Keterangan |
|----------|--------|-----------|------------|
| `/fs/list` | GET | `path` (opsional, default `/`) | Daftar file/direktori |
| `/fs/info` | GET | - | Info tentang ruang filesystem |
| `/fs/delete` | POST | `path` | Hapus file atau folder |
| `/fs/mkdir` | POST | `path` | Buat direktori baru |
| `/fs/upload` | POST | body multipart | Upload file |

**Contoh curl**:
```sh
curl "http://{{host}}/fs/list?path=/images"
```

---

## 📶 WiFi API
| Endpoint | Metode | Parameter | Keterangan |
|----------|--------|-----------|------------|
| `/wifi/data` | GET | - | Ambil daftar konfigurasi WiFi tersimpan |
| `/wifi/add` | POST | `ssid`, `pass`, `static` | Tambah WiFi baru |
| `/wifi/delete` | POST | `ssid` | Hapus WiFi tersimpan |
| `/wifi/test` | POST | `ssid`, `pass` | Uji koneksi ke jaringan |

**Contoh**:
```sh
curl -X POST http://{{host}}/wifi/add \
  -d "ssid=MyWiFi&pass=mypassword&static=false"
```

---

## 🖥️ LCD Display API
| Endpoint | Metode | Parameter | Keterangan |
|----------|--------|-----------|------------|
| `/lcd/show` | GET | `img` (nama file) | Tampilkan gambar dari filesystem |
| `/lcd/backlight` | GET | `value` (0/1) | Nyalakan/matikan backlight |
| `/lcd/clear` | GET | - | Bersihkan layar |
| `/lcd/text` | GET | `msg`, `x`, `y`, `size` | Tampilkan teks |
| `/lcd/slideshow` | GET | `folder`, `delay` | Jalankan slideshow gambar |


---

## 🔧 Service API
| Endpoint | Metode | Parameter | Keterangan |
|----------|--------|-----------|------------|
| `/service/select` | GET | `id` | Pilih layanan (misalnya jam analog, GIF, dsb) |

---

## 🌙 Lain-lain
| Endpoint | Metode | Keterangan |
|----------|--------|------------|
| `/sleep/light` | GET | Mode tidur dengan sensor cahaya |

---

## 📌 Tips Penggunaan
1. Pastikan ESP32/ESP8266 terhubung ke jaringan yang sama.
2. Gunakan tool seperti `curl`, Postman, atau file `rest.http` untuk menguji endpoint secara interaktif.
3. Untuk debugging, lihat serial monitor pada perangkat.

---

Semoga dokumentasi ini membantu dalam menggunakan API proyek LCD. Jika Anda menambahkan endpoint baru, perbarui README ini sesuai.
