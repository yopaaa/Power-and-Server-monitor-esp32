# ⚡ Power Meter Backend & Web Dashboard (Golang + SQLite + Docker)

Backend server mandiri yang ultra-ringan (~15 MB RAM) untuk menerima data telemetri dari ESP32 + PZEM-004T dan menampilkan dashboard web interaktif secara realtime dengan sistem otentikasi.

---

## 🚀 Fitur Utama

- **Keamanan Penuh (Authentication)**:
  - **Web Dashboard**: Halaman login (Username & Password) dengan session token.
  - **ESP32 Telemetry**: Proteksi `X-API-Key` pada endpoint penerima data.
  - **File Konfigurasi JSON**: Semua kredensial disimpan di file `data/auth.json` di dalam folder data yang di-mount.
- **Ultra Cepat & Ringan**: Ditulis dengan Go murni dan SQLite WAL mode.
- **Docker Ready**: Dilengkapi `Dockerfile` multi-stage dan `docker-compose.yml`.
- **Persistent Data**: Folder `./data` di-mount ke `/app/data` di dalam container sehingga database SQLite dan file kredensial tersimpan aman di host.
- **Embedded Web Dashboard**: File web (HTML/Tailwind/Chart.js) disematkan langsung di dalam binary Go via `embed.FS`.
- **Analitik Listrik**:
  - Live KPI: Watt, Volt, Ampere, kWh, PF, Hz.
  - Estimasi biaya listrik PLN (Rupiah) harian.
  - Grafik riwayat beban (1 jam, 6 jam, 24 jam, 7 hari, 30 hari).
  - Indikator status koneksi ESP32 (Online/Offline pulse).

---

## 🔐 Konfigurasi Kredensial (`data/auth.json`)

Kredensial disimpan dalam format JSON di file **`server/data/auth.json`**. File ini otomatis dibuat saat pertama kali server dijalankan jika belum ada:

```json
{
  "username": "admin",
  "password": "admin123",
  "api_key": "esp32_secret_token_123"
}
```

- **`username` & `password`**: Digunakan untuk login ke Web Dashboard.
- **`api_key`**: Digunakan oleh ESP32 untuk mengirimkan data melalui header `X-API-Key`.
- **Hot-Reload**: Jika Anda mengubah file `auth.json`, perubahan akan langsung terbaca tanpa perlu me-restart server!

---

## 🛠️ Cara Menjalankan dengan Docker

1. Masuk ke direktori `server/`:
   ```bash
   cd server
   ```

2. Jalankan dengan Docker Compose:
   ```bash
   docker compose up -d --build
   ```

3. Buka dashboard di browser:
   ```text
   http://localhost:8080
   # atau jika di server/VPS: http://<IP-SERVER>:8080
   ```
   Masukkan username (`admin`) dan password (`admin123`).

4. Database SQLite dan file kredensial tersimpan di folder host:
   - `server/data/power.db`
   - `server/data/auth.json`

---

## 📡 Dokumentasi Endpoint REST API

### 1. Login Web Dashboard
- **Method**: `POST`
- **Path**: `/api/auth/login`
- **Body**: `{"username":"admin", "password":"admin123"}`
- **Response**: `{"status":"success", "token":"...", "username":"admin"}`

### 2. Kirim Metrik dari ESP32 (Harus Terotentikasi)
- **Method**: `POST`
- **Path**: `/api/metrics`
- **Headers**:
  - `Content-Type: application/json`
  - `X-API-Key: esp32_secret_token_123` *(sesuai auth.json)*
- **Body**:
  ```json
  {
    "voltage": 224.5,
    "current": 1.25,
    "power": 280.6,
    "energy": 12.45,
    "frequency": 50.0,
    "pf": 0.98
  }
  ```

### 3. Ambil Data Metrik (Perlu Header `Authorization: Bearer <token>`)
- `GET /api/metrics/latest`
- `GET /api/metrics/summary`
- `GET /api/metrics/history?range=24h` *(pilihan: `1h`, `6h`, `24h`, `7d`, `30d`)*
