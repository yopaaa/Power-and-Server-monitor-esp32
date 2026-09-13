# ⚡ Power Meter Backend & Web Dashboard (Golang + SQLite + Docker)

Backend server mandiri yang ultra-ringan (~15 MB RAM) untuk menerima data telemetri dari ESP32 + PZEM-004T dan menampilkan dashboard web interaktif secara realtime.

---

## 🚀 Fitur Utama

- **Ultra Cepat & Ringan**: Ditulis dengan Go murni (tanpa runtime berat) dan SQLite WAL mode.
- **Docker Ready**: Dilengkapi `Dockerfile` multi-stage dan `docker-compose.yml`.
- **Persistent Data**: Folder `./data` di-mount ke `/app/data` di dalam container sehingga database SQLite tersimpan aman di host.
- **Embedded Web Dashboard**: File web (HTML/Tailwind/Chart.js) disematkan langsung di dalam binary Go via `embed.FS`.
- **Analitik Listrik**:
  - Live KPI: Watt, Volt, Ampere, kWh, PF, Hz.
  - Estimasi biaya listrik PLN (Rupiah) harian.
  - Grafik riwayat beban (1 jam, 6 jam, 24 jam, 7 hari, 30 hari).
  - Indikator status koneksi ESP32 (Online/Offline pulse).

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

4. Database SQLite akan otomatis dibuat dan tersimpan di folder host:
   `server/data/power.db`

---

## 📡 Dokumentasi Endpoint REST API

### 1. Kirim Metrik dari ESP32
- **Method**: `POST`
- **Path**: `/api/metrics`
- **Headers**:
  - `Content-Type: application/json`
  - `X-API-Key: rahasia123` *(sesuai konfigurasi di docker-compose.yml)*
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

### 2. Ambil Metrik Terkini
- **Method**: `GET`
- **Path**: `/api/metrics/latest`

### 3. Ambil Ringkasan & Estimasi Biaya Hari Ini
- **Method**: `GET`
- **Path**: `/api/metrics/summary`

### 4. Ambil Riwayat untuk Grafik
- **Method**: `GET`
- **Path**: `/api/metrics/history?range=24h` *(pilihan range: `1h`, `6h`, `24h`, `7d`, `30d`)*

---

## ⚙️ Variabel Lingkungan (`environment` di `docker-compose.yml`)

| Variable | Default | Keterangan |
|---|---|---|
| `PORT` | `8080` | Port HTTP server |
| `DB_PATH` | `/app/data/power.db` | Lokasi file database SQLite di container |
| `API_KEY` | `rahasia123` | Token otentikasi POST dari ESP32 (kosongkan jika tanpa proteksi) |
| `PLN_RATE` | `1444.70` | Tarif PLN per kWh (Rp 1.444,70 untuk R-1/1300-2200VA) |
| `TZ` | `Asia/Jakarta` | Zona waktu server |
