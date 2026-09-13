#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="id">
<head>
  <meta charset="utf-8" />
  <title>Power Meter &amp; System Configuration</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <style>
    :root {
      --bg: #0B1120;
      --surface: #151F32;
      --surface-inner: #0F172A;
      --border: #23324D;
      --border-light: #334155;
      --primary: #0284C7;
      --primary-hover: #0EA5E9;
      --accent: #06B6D4;
      --accent-emerald: #10B981;
      --accent-dim: rgba(6, 182, 212, 0.12);
      --text: #F1F5F9;
      --text-dim: #94A3B8;
      --text-muted: #64748B;
      --danger: #EF4444;
      --danger-hover: #DC2626;
      --warning: #F59E0B;
    }
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    body {
      font-family: ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
      background-color: var(--bg);
      color: var(--text);
      line-height: 1.4;
      padding: 12px 16px;
      font-size: 13px;
    }
    .wrapper {
      max-width: 1080px;
      margin: 0 auto;
    }
    
    /* Top Header */
    header {
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      align-items: center;
      padding-bottom: 12px;
      border-bottom: 1px solid var(--border);
      margin-bottom: 12px;
      gap: 10px;
    }
    .brand {
      display: flex;
      align-items: center;
      gap: 10px;
    }
    .brand-icon {
      width: 32px;
      height: 32px;
      border-radius: 8px;
      background: linear-gradient(135deg, #0284C7, #10B981);
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 16px;
      box-shadow: 0 0 12px rgba(6, 182, 212, 0.3);
    }
    .brand h1 {
      font-size: 16px;
      font-weight: 700;
      color: var(--text);
      letter-spacing: 0.3px;
    }
    .brand p {
      font-size: 10px;
      color: var(--accent);
      text-transform: uppercase;
      letter-spacing: 0.8px;
      font-weight: 600;
    }
    .badge-bar {
      display: flex;
      gap: 6px;
      align-items: center;
      flex-wrap: wrap;
    }
    .badge {
      display: inline-flex;
      align-items: center;
      gap: 5px;
      padding: 3px 8px;
      border-radius: 6px;
      font-size: 11px;
      font-weight: 500;
      background: var(--surface-inner);
      border: 1px solid var(--border);
      color: var(--accent);
    }
    .badge .dot {
      width: 6px;
      height: 6px;
      border-radius: 50%;
      background: var(--accent-emerald);
      box-shadow: 0 0 6px var(--accent-emerald);
    }

    /* ── 1-LINE COMPACT SYSTEM STRIP ── */
    .sys-strip {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 8px 12px;
      margin-bottom: 14px;
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      font-size: 11px;
    }
    .sys-items {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 8px 14px;
      color: var(--text-dim);
    }
    .sys-item span {
      color: var(--text-muted);
      margin-right: 3px;
    }
    .sys-item b {
      color: var(--text);
      font-weight: 600;
      font-family: ui-monospace, monospace;
    }
    .sys-actions {
      display: flex;
      align-items: center;
      gap: 6px;
    }
    .btn-micro {
      padding: 3px 8px;
      font-size: 10.5px;
      font-weight: 600;
      border-radius: 5px;
      border: 1px solid var(--border);
      background: var(--surface-inner);
      color: var(--text-dim);
      cursor: pointer;
      transition: all 0.15s;
    }
    .btn-micro:hover {
      background: var(--border);
      color: var(--text);
    }
    .btn-micro.accent {
      border-color: rgba(6, 182, 212, 0.4);
      color: var(--accent);
    }
    .btn-micro.danger {
      border-color: rgba(239, 68, 68, 0.3);
      color: var(--danger);
    }
    .btn-micro.danger:hover {
      background: rgba(239, 68, 68, 0.15);
    }

    /* Grid Layout */
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(340px, 1fr));
      gap: 14px;
    }

    /* Cards */
    .card {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 14px 16px;
      position: relative;
      transition: border-color 0.2s;
    }
    .card:hover {
      border-color: rgba(6, 182, 212, 0.35);
    }
    .card h2 {
      font-size: 13px;
      font-weight: 700;
      color: var(--accent);
      letter-spacing: 0.3px;
      margin-bottom: 10px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 1px solid var(--border);
      padding-bottom: 6px;
    }
    .card h2 .action-link {
      font-size: 11px;
      color: var(--text-muted);
      cursor: pointer;
      font-weight: 500;
      text-decoration: none;
    }
    .card h2 .action-link:hover {
      color: var(--accent);
    }
    .card-desc {
      color: var(--text-dim);
      font-size: 11px;
      margin-bottom: 10px;
      line-height: 1.3;
    }

    /* Form Fields */
    .form-row {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
      gap: 8px;
      margin-bottom: 8px;
    }
    .form-group {
      margin-bottom: 8px;
    }
    label {
      display: block;
      font-size: 10.5px;
      font-weight: 600;
      color: var(--text-dim);
      margin-bottom: 4px;
      letter-spacing: 0.2px;
    }
    input[type="text"],
    input[type="password"],
    input[type="number"],
    input[type="file"],
    select {
      width: 100%;
      padding: 7px 10px;
      background: var(--surface-inner);
      border: 1px solid var(--border);
      color: var(--text);
      border-radius: 6px;
      font-size: 12px;
      outline: none;
      transition: border-color 0.2s;
    }
    input[type="text"]:focus,
    input[type="password"]:focus,
    input[type="number"]:focus,
    select:focus {
      border-color: var(--accent);
    }

    /* Buttons */
    .btn-row {
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
      margin-top: 10px;
    }
    button, .btn {
      padding: 6px 12px;
      border-radius: 6px;
      font-size: 11.5px;
      font-weight: 600;
      cursor: pointer;
      border: none;
      transition: all 0.2s;
    }
    .btn-primary {
      background: linear-gradient(135deg, #0284C7, #06B6D4);
      color: #FFFFFF;
      box-shadow: 0 2px 8px rgba(2, 132, 199, 0.25);
    }
    .btn-primary:hover {
      opacity: 0.92;
    }
    .btn-outline {
      background: var(--surface-inner);
      border: 1px solid var(--border);
      color: var(--text-dim);
    }
    .btn-outline:hover {
      border-color: var(--accent);
      color: var(--text);
    }
    .btn-danger {
      background: rgba(239, 68, 68, 0.15);
      border: 1px solid rgba(239, 68, 68, 0.35);
      color: var(--danger);
    }
    .btn-danger:hover {
      background: var(--danger);
      color: #fff;
    }
    button:disabled {
      opacity: 0.45;
      cursor: not-allowed;
    }

    /* Status Messages */
    .status-msg {
      background: var(--surface-inner);
      border-left: 3px solid var(--accent);
      padding: 6px 10px;
      font-size: 11px;
      color: var(--text);
      border-radius: 0 6px 6px 0;
      margin-top: 8px;
      word-break: break-word;
    }

    /* Saved WiFi List */
    .wifi-list {
      display: flex;
      flex-direction: column;
      gap: 6px;
      max-height: 170px;
      overflow-y: auto;
      margin-bottom: 8px;
    }
    .wifi-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      background: var(--surface-inner);
      border: 1px solid var(--border);
      padding: 6px 10px;
      border-radius: 6px;
      font-size: 11.5px;
    }
    .wifi-name {
      font-weight: 600;
      color: var(--text);
    }
    .wifi-detail {
      font-size: 10px;
      color: var(--text-muted);
    }

    /* OTA Progress Bar */
    .progress-track {
      width: 100%;
      height: 6px;
      background: var(--surface-inner);
      border-radius: 3px;
      overflow: hidden;
      margin-top: 8px;
      display: none;
    }
    .progress-fill {
      height: 100%;
      width: 0%;
      background: linear-gradient(90deg, #0284C7, #10B981);
      transition: width 0.2s;
    }

    .hidden {
      display: none !important;
    }
  </style>
</head>
<body>
  <div class="wrapper">
    <!-- Navbar Header -->
    <header>
      <div class="brand">
        <div class="brand-icon">⚡</div>
        <div>
          <h1>Power &amp; Server Monitor</h1>
          <p>ESP32 Config Portal</p>
        </div>
      </div>
      <div class="badge-bar">
        <div class="badge"><span class="dot"></span> Online</div>
        <div class="badge" id="badge-ip">IP: ...</div>
        <div class="badge" id="badge-rssi">RSSI: ...</div>
      </div>
    </header>

    <!-- ── 1-LINE COMPACT SYSTEM STRIP ── -->
    <div class="sys-strip">
      <div class="sys-items">
        <div class="sys-item"><span>Chip:</span><b id="val-chip">...</b></div>
        <div class="sys-item"><span>IP:</span><b id="val-ip">...</b></div>
        <div class="sys-item"><span>Signal:</span><b id="val-rssi">...</b></div>
        <div class="sys-item"><span>Heap:</span><b id="val-heap">...</b></div>
        <div class="sys-item"><span>Up:</span><b id="val-uptime">...</b></div>
        <div class="sys-item"><span>Mode:</span><b id="val-status">...</b></div>
        <div class="sys-item"><span>Cycle:</span><b id="badge-autocycle">MANUAL</b></div>
      </div>
      <div class="sys-actions">
        <button type="button" class="btn-micro accent" id="btn-toggle-autocycle" onclick="toggleAutoCycle()" title="Ubah rotasi otomatis layar">Auto 20s</button>
        <button type="button" class="btn-micro" onclick="toggleLcdInvert()" title="Balik kontras warna layar">Invert LCD</button>
        <button type="button" class="btn-micro danger" onclick="formatFS()" title="Format penyimpanan flash">Format FS</button>
        <button type="button" class="btn-micro" onclick="loadSysInfo()" title="Segarkan info">🔄</button>
      </div>
    </div>

    <!-- Hidden elements required by existing JS logic -->
    <div style="display:none;">
      <span id="val-cpu"></span>
      <span id="val-flash"></span>
      <span id="val-gw"></span>
      <span id="desc-autocycle"></span>
      <span id="sys-action-status"></span>
    </div>

    <!-- ── 2-COLUMN COMPACT CONFIGURATION GRID ── -->
    <div class="grid">

      <!-- CARD 1: Backend Power Logger (Golang Server) -->
      <div class="card">
        <h2>
          <span>⚡ Backend Power Logger</span>
          <span class="action-link" onclick="loadPowerLoggerConfig()">Segarkan</span>
        </h2>
        <p class="card-desc">
          Kirim telemetri PZEM-004T (Volt, Amp, Watt, kWh, PF, Hz) secara periodik ke Backend Golang.
        </p>

        <div class="form-group">
          <label>Endpoint URL</label>
          <input id="logger-url" type="text" placeholder="http://192.168.1.100:8080/api/metrics" />
        </div>

        <div class="form-row">
          <div class="form-group">
            <label>API Key (X-API-Key)</label>
            <input id="logger-key" type="text" placeholder="esp32_secret_token_123" />
          </div>
          <div class="form-group">
            <label>Interval (detik)</label>
            <input id="logger-interval" type="number" min="5" max="300" placeholder="15" />
          </div>
        </div>

        <div style="display: flex; align-items: center; gap: 6px; margin: 4px 0 8px 0;">
          <input type="checkbox" id="logger-enabled" style="width: auto; cursor: pointer;" />
          <label for="logger-enabled" style="margin: 0; cursor: pointer; font-size: 11.5px; color: var(--text);">
            Aktifkan Pengiriman Otomatis
          </label>
        </div>

        <div class="btn-row">
          <button type="button" class="btn-outline" id="btnTestLogger" onclick="testPowerLogger()">
            🔍 Tes Kirim
          </button>
          <button type="button" class="btn-primary" id="btnSaveLogger" onclick="savePowerLoggerConfig()">
            💾 Simpan Pengaturan
          </button>
        </div>
        <div id="logger-status" class="status-msg" style="display: none;"></div>
      </div>

      <!-- CARD 2: Beszel Hub (PocketBase Server Monitor) -->
      <div class="card">
        <h2>
          <span>🖥️ Beszel Hub Server Monitor</span>
          <span class="action-link" onclick="loadBeszelConfig()">Segarkan</span>
        </h2>
        <p class="card-desc">
          Ambil metrik performa CPU, RAM, Disk, dan Suhu server secara otomatis dari Beszel Hub.
        </p>

        <div class="form-group">
          <label>URL Beszel Hub</label>
          <input id="beszel-url" type="text" placeholder="http://192.168.1.100:8090" />
        </div>

        <div class="form-row">
          <div class="form-group">
            <label>Email / Username</label>
            <input id="beszel-email" type="text" placeholder="admin@example.com" />
          </div>
          <div class="form-group">
            <label>Password</label>
            <input id="beszel-pass" type="password" placeholder="Password akun" />
          </div>
        </div>

        <div class="btn-row">
          <button type="button" class="btn-outline" id="btnTestBeszel" onclick="testBeszelHub()">
            🔍 Tes Koneksi
          </button>
          <button type="button" class="btn-primary" id="btnSaveBeszel" onclick="saveBeszelConfigAndSync()">
            💾 Simpan &amp; Sinkronkan
          </button>
        </div>
        <div id="beszel-status" class="status-msg" style="display: none;"></div>
      </div>

      <!-- CARD 3: WiFi Configuration -->
      <div class="card">
        <h2>
          <span>📶 Konfigurasi WiFi Baru</span>
          <span class="action-link" id="btnScan" onclick="scanWifiNetworks()">🔍 Pindai WiFi</span>
        </h2>

        <div class="form-group">
          <label>Nama WiFi (SSID)</label>
          <div style="display: flex; gap: 6px;">
            <input id="ssid" type="text" placeholder="SSID atau pilih dari hasil scan" style="flex: 1;" />
            <select id="wifi-select" onchange="onSelectScannedWifi(this)" style="display: none; max-width: 130px; font-size: 11px;">
              <option value="">-- Hasil Scan --</option>
            </select>
          </div>
          <input id="target-channel" type="hidden" value="0" />
        </div>

        <div class="form-group">
          <label>Password</label>
          <input id="pass" type="password" placeholder="Password WiFi" />
        </div>

        <div class="form-group" style="margin-bottom: 6px;">
          <label style="display: inline-flex; align-items: center; gap: 6px; cursor: pointer; text-transform: none;">
            <input type="checkbox" id="static" onchange="toggleStatic()" style="width: auto;" />
            Gunakan Static IP
          </label>
        </div>

        <div id="static_fields" class="hidden">
          <div class="form-row">
            <div class="form-group">
              <label>IP Address</label>
              <input id="ip" type="text" value="192.168.1.50" />
            </div>
            <div class="form-group">
              <label>Gateway</label>
              <input id="gw" type="text" value="192.168.1.1" />
            </div>
            <div class="form-group">
              <label>Subnet</label>
              <input id="sn" type="text" value="255.255.255.0" />
            </div>
          </div>
        </div>

        <div class="btn-row">
          <button type="button" class="btn-outline" id="btnTest">Cek Koneksi</button>
          <button type="button" class="btn-primary" id="btnSave">Simpan WiFi</button>
        </div>

        <div style="margin-top: 8px;">
          <pre id="out" style="background: var(--surface-inner); border: 1px solid var(--border); padding: 6px 10px; border-radius: 6px; font-size: 11px; color: var(--text-dim); white-space: pre-wrap;">Ready</pre>
        </div>
      </div>

      <!-- CARD 4: Saved WiFi & Web OTA Update -->
      <div class="card">
        <h2>
          <span>🗂️ WiFi Tersimpan &amp; OTA Update</span>
          <span class="action-link" onclick="loadWiFiList()">Segarkan</span>
        </h2>

        <div class="wifi-list" id="wifiList">
          <div style="color: var(--text-dim); font-size: 11px;">Memuat daftar WiFi...</div>
        </div>

        <div style="border-top: 1px solid var(--border); padding-top: 10px; margin-top: 8px;">
          <label>Web OTA Firmware Flash (.bin)</label>
          <form id="ota-form" style="display: flex; gap: 6px; margin-top: 4px;">
            <input type="file" id="file-input" name="update" accept=".bin" required style="padding: 4px 8px; font-size: 11px;" />
            <button type="submit" class="btn-primary" id="btnUpdate" style="white-space: nowrap;">
              Flash OTA
            </button>
          </form>
          <div class="progress-track" id="prg-bar">
            <div class="progress-fill" id="prg-fill"></div>
          </div>
          <div id="ota-status" class="status-msg" style="display: none;"></div>
        </div>
      </div>

    </div>
  </div>

  <script>
    const out = document.getElementById("out");
    const wifiListDiv = document.getElementById("wifiList");

    function toggleStatic() {
      document.getElementById("static_fields").classList.toggle("hidden", !document.getElementById("static").checked);
    }

    let isSysInfoLoading = false;
    async function loadSysInfo() {
      if (isSysInfoLoading) return;
      isSysInfoLoading = true;
      try {
        const res = await fetch("/sys/info");
        const data = await res.json();
        
        document.getElementById("val-chip").textContent = data.chipId || "-";
        if (document.getElementById("val-cpu")) document.getElementById("val-cpu").textContent = data.cpuFreqMHz ? data.cpuFreqMHz + " MHz" : "-";
        document.getElementById("val-heap").textContent = data.freeHeap ? Math.round(data.freeHeap / 1024) + " KB" : "-";
        if (document.getElementById("val-flash")) document.getElementById("val-flash").textContent = data.flashSize ? Math.round(data.flashSize / (1024 * 1024)) + " MB" : "-";

        const sec = Math.floor((data.uptimeMs || 0) / 1000);
        const d = Math.floor(sec / 86400);
        const h = Math.floor((sec % 86400) / 3600);
        const m = Math.floor((sec % 3600) / 60);
        const s = sec % 60;
        let upStr = "";
        if (d > 0) upStr += d + "d ";
        if (h > 0 || d > 0) upStr += h + "h ";
        upStr += m + "m " + s + "s";
        document.getElementById("val-uptime").textContent = upStr;

        document.getElementById("val-ip").textContent = data.localIP || "-";
        if (document.getElementById("val-gw")) document.getElementById("val-gw").textContent = data.gateway || "-";
        document.getElementById("badge-ip").textContent = "IP: " + (data.localIP || "-");

        const rssi = data.rssi;
        let rssiLabel = "N/A";
        if (rssi) {
          rssiLabel = rssi + " dBm";
          if (rssi >= -60) rssiLabel += " (Bagus)";
          else if (rssi >= -75) rssiLabel += " (Cukup)";
          else rssiLabel += " (Lemah)";
        }
        document.getElementById("val-rssi").textContent = rssiLabel;
        document.getElementById("badge-rssi").textContent = "RSSI: " + (rssi ? rssi + " dBm" : "-");

        let modeText = "STA";
        if (data.wifiMode === 2) modeText = "AP";
        else if (data.wifiMode === 3) modeText = "AP+STA";
        document.getElementById("val-status").textContent = (data.wifiStatus === 3 ? "Connected (" : "Mode (") + modeText + ")";

        if (typeof data.autoCycle !== "undefined") {
          updateAutoCycleUI(data.autoCycle, data.autoCycleInterval);
        }
      } catch (e) {
        console.error("Gagal memuat sysinfo", e);
      } finally {
        isSysInfoLoading = false;
      }
    }

    async function formatFS() {
      if (!confirm("PERINGATAN: Apakah Anda yakin ingin memformat partisi LittleFS?\nSemua file/gambar lama di partisi penyimpanan akan dihapus permanen.")) return;
      alert("Memformat LittleFS... Mohon tunggu sebentar.");
      try {
        const res = await fetch("/sys/format");
        const d = await res.json();
        if (d.status === "ok") {
          alert("Partisi penyimpanan berhasil diformat bersih!");
        } else {
          alert("Gagal memformat: " + (d.message || ""));
        }
      } catch (e) {
        alert("Error menghubungi perangkat.");
      }
    }

    function updateAutoCycleUI(enabled, intervalMs) {
      const badge = document.getElementById("badge-autocycle");
      const btn = document.getElementById("btn-toggle-autocycle");
      const sec = (intervalMs >= 1000) ? Math.round(intervalMs / 1000) : (intervalMs || 20);

      if (enabled) {
        badge.textContent = "AUTO (" + sec + "s)";
        badge.style.color = "var(--accent-emerald)";
        btn.textContent = "Matikan Auto";
      } else {
        badge.textContent = "MANUAL";
        badge.style.color = "var(--text-dim)";
        btn.textContent = "Auto " + sec + "s";
      }
    }

    async function toggleAutoCycle() {
      const btn = document.getElementById("btn-toggle-autocycle");
      btn.disabled = true;
      try {
        const res = await fetch("/lcd/autocycle", { method: "POST" });
        const d = await res.json();
        if (d.status === "ok") {
          const isEnabled = (typeof d.enabled !== "undefined") ? d.enabled : d.autoCycle;
          const intervalMs = d.autoCycleInterval || (d.interval ? d.interval * 1000 : 20000);
          updateAutoCycleUI(isEnabled, intervalMs);
        }
      } catch (e) {
        console.error("Error toggle autocycle", e);
      } finally {
        btn.disabled = false;
      }
    }

    async function toggleLcdInvert() {
      try {
        const res = await fetch("/lcd/invert", { method: "POST" });
        const d = await res.json();
        alert("Warna LCD berhasil dibalik (Invert: " + (d.inverted ? "Aktif" : "Normal") + ")!");
      } catch (e) {
        alert("Error toggle LCD invert: " + e.message);
      }
    }

    async function loadWiFiList() {
      try {
        let res = await fetch("/wifi/data");
        if (!res.ok) res = await fetch("/wifi/list");
        const data = await res.json();
        const list = (data && data.wifiList) ? data.wifiList : (Array.isArray(data) ? data : []);

        if (!list || list.length === 0) {
          wifiListDiv.innerHTML = '<div style="color: var(--text-dim); font-size: 11px;">Belum ada WiFi yang tersimpan.</div>';
          return;
        }

        let html = '';
        list.forEach((item, index) => {
          const ssidEsc = escapeHtml(item.ssid);
          const detail = item.useStatic ? ('Static IP: ' + (item.ip || '')) : 'DHCP';
          html += `
            <div class="wifi-item">
              <div>
                <div class="wifi-name">${ssidEsc}</div>
                <div class="wifi-detail">${detail}</div>
              </div>
              <button class="btn-danger btn-micro" onclick="deleteWiFi('${ssidEsc}', ${index})">Hapus</button>
            </div>
          `;
        });
        wifiListDiv.innerHTML = html;
      } catch (e) {
        wifiListDiv.innerHTML = '<div style="color: var(--danger); font-size: 11px;">Gagal memuat daftar WiFi.</div>';
      }
    }

    async function deleteWiFi(ssid, index) {
      if (!confirm("Hapus WiFi '" + ssid + "' dari daftar tersimpan?")) return;
      try {
        const form = new URLSearchParams();
        form.append("ssid", ssid);
        form.append("index", index);
        const res = await fetch("/wifi/delete", { method: "POST", body: form });
        const d = await res.json();
        if (d.status === "ok") {
          loadWiFiList();
        } else {
          alert("Gagal menghapus WiFi: " + (d.message || ""));
        }
      } catch (e) {
        alert("Error menghapus WiFi.");
      }
    }

    async function scanWifiNetworks() {
      const scanBtn = document.getElementById("btnScan");
      const sel = document.getElementById("wifi-select");
      scanBtn.textContent = "Memindai...";
      scanBtn.style.pointerEvents = "none";

      try {
        const res = await fetch("/wifi/scan");
        const data = await res.json();
        const nets = (data && data.networks) ? data.networks : (Array.isArray(data) ? data : []);
        sel.innerHTML = '<option value="">-- Pilih WiFi (' + nets.length + ' ditemukan) --</option>';
        nets.forEach(n => {
          const opt = document.createElement("option");
          opt.value = n.ssid;
          opt.textContent = n.ssid + " (" + n.rssi + " dBm)";
          opt.dataset.channel = n.channel;
          sel.appendChild(opt);
        });
        sel.style.display = "block";
      } catch (e) {
        alert("Gagal memindai WiFi");
      } finally {
        scanBtn.textContent = "🔍 Pindai WiFi";
        scanBtn.style.pointerEvents = "auto";
      }
    }

    function onSelectScannedWifi(sel) {
      const selected = sel.options[sel.selectedIndex];
      if (!selected.value) return;
      document.getElementById("ssid").value = selected.value;
      document.getElementById("target-channel").value = selected.dataset.channel || "0";
      document.getElementById("pass").focus();
    }

    document.getElementById("btnTest").addEventListener("click", async () => {
      const ssid = document.getElementById("ssid").value.trim();
      const pass = document.getElementById("pass").value;
      if (!ssid) return alert("SSID tidak boleh kosong");

      out.textContent = "Mencoba tes koneksi ke " + ssid + "...";
      const form = new URLSearchParams();
      form.append("ssid", ssid);
      form.append("password", pass);

      try {
        const res = await fetch("/wifi/test", { method: "POST", body: form });
        const d = await res.json();
        out.textContent = d.message || JSON.stringify(d);
      } catch (e) {
        out.textContent = "Error: " + e.message;
      }
    });

    document.getElementById("btnSave").addEventListener("click", async () => {
      const ssid = document.getElementById("ssid").value.trim();
      const pass = document.getElementById("pass").value;
      if (!ssid) return alert("SSID tidak boleh kosong");

      const form = new URLSearchParams();
      form.append("ssid", ssid);
      form.append("password", pass);
      form.append("channel", document.getElementById("target-channel").value || "0");

      if (document.getElementById("static").checked) {
        form.append("static", "1");
        form.append("ip", document.getElementById("ip").value.trim());
        form.append("gateway", document.getElementById("gw").value.trim());
        form.append("subnet", document.getElementById("sn").value.trim());
      }

      out.textContent = "Menyimpan kredensial WiFi...";
      try {
        const res = await fetch("/wifi/save", { method: "POST", body: form });
        const d = await res.json();
        out.textContent = d.message || "Tersimpan!";
        loadWiFiList();
      } catch (e) {
        out.textContent = "Error: " + e.message;
      }
    });

    // OTA Update Form
    document.getElementById("ota-form").addEventListener("submit", (e) => {
      e.preventDefault();
      const fileInput = document.getElementById("file-input");
      if (!fileInput.files.length) return alert("Pilih file firmware.bin!");

      const file = fileInput.files[0];
      const formData = new FormData();
      formData.append("update", file);

      const prgBar = document.getElementById("prg-bar");
      const prgFill = document.getElementById("prg-fill");
      const statusEl = document.getElementById("ota-status");
      const btn = document.getElementById("btnUpdate");

      prgBar.style.display = "block";
      statusEl.style.display = "block";
      statusEl.textContent = "Mengunggah firmware... Mohon jangan matikan perangkat.";
      btn.disabled = true;

      const xhr = new XMLHttpRequest();
      xhr.open("POST", "/update");

      xhr.upload.onprogress = (evt) => {
        if (evt.lengthComputable) {
          const pct = Math.round((evt.loaded / evt.total) * 100);
          prgFill.style.width = pct + "%";
          statusEl.textContent = "Flashing: " + pct + "%...";
        }
      };

      xhr.onload = () => {
        if (xhr.status === 200) {
          statusEl.textContent = "Flash Berhasil! ESP32 akan restart otomatis dalam beberapa detik...";
        } else {
          statusEl.textContent = "Flash Gagal: " + xhr.responseText;
          btn.disabled = false;
        }
      };

      xhr.onerror = () => {
        statusEl.textContent = "Koneksi terputus saat flash.";
        btn.disabled = false;
      };

      xhr.send(formData);
    });

    // Beszel Hub
    async function loadBeszelConfig() {
      try {
        const res = await fetch('/api/beszel/config');
        const d = await res.json();
        if (d.hubUrl) document.getElementById('beszel-url').value = d.hubUrl;
        if (d.email) document.getElementById('beszel-email').value = d.email;
      } catch (e) {
        console.error('Gagal memuat config Beszel', e);
      }
    }

    async function testBeszelHub() {
      const url = document.getElementById('beszel-url').value.trim();
      const email = document.getElementById('beszel-email').value.trim();
      const pass = document.getElementById('beszel-pass').value;
      const statusEl = document.getElementById('beszel-status');
      const btn = document.getElementById('btnTestBeszel');

      if (!url || !email) {
        alert('Mohon isi URL Beszel Hub dan Email terlebih dahulu.');
        return;
      }

      statusEl.style.display = 'block';
      statusEl.textContent = 'Menguji koneksi ke Beszel Hub...';
      btn.disabled = true;

      try {
        const form = new URLSearchParams();
        form.append('hubUrl', url);
        form.append('email', email);
        form.append('password', pass);

        const res = await fetch('/api/beszel/test', { method: 'POST', body: form });
        const d = await res.json();
        statusEl.innerHTML = '<strong>' + escapeHtml(d.message || '') + '</strong>';
      } catch (e) {
        statusEl.textContent = 'Error: ' + e.message;
      } finally {
        btn.disabled = false;
      }
    }

    async function saveBeszelConfigAndSync() {
      const url = document.getElementById('beszel-url').value.trim();
      const email = document.getElementById('beszel-email').value.trim();
      const pass = document.getElementById('beszel-pass').value;
      const statusEl = document.getElementById('beszel-status');
      const btn = document.getElementById('btnSaveBeszel');

      if (!url || !email) return alert('Mohon isi URL dan Email Beszel.');

      statusEl.style.display = 'block';
      statusEl.textContent = 'Menyimpan & sinkronisasi...';
      btn.disabled = true;

      try {
        const form = new URLSearchParams();
        form.append('hubUrl', url);
        form.append('email', email);
        form.append('password', pass);

        await fetch('/api/beszel/config', { method: 'POST', body: form });
        const syncRes = await fetch('/api/beszel/sync', { method: 'POST' });
        const d = await syncRes.json();
        statusEl.innerHTML = '<strong>' + escapeHtml(d.message || 'Tersimpan!') + '</strong>';
      } catch (e) {
        statusEl.textContent = 'Error: ' + e.message;
      } finally {
        btn.disabled = false;
      }
    }

    // Power Logger
    async function loadPowerLoggerConfig() {
      try {
        const res = await fetch('/api/power-logger/config');
        const d = await res.json();
        if (d.endpointUrl) document.getElementById('logger-url').value = d.endpointUrl;
        if (d.apiKey) document.getElementById('logger-key').value = d.apiKey;
        if (d.intervalSec) document.getElementById('logger-interval').value = d.intervalSec;
        document.getElementById('logger-enabled').checked = !!d.isEnabled;

        if (d.lastHttpStatus !== 0 && d.lastHttpStatus !== undefined) {
          const statusEl = document.getElementById('logger-status');
          statusEl.style.display = 'block';
          statusEl.style.borderLeftColor = (d.lastHttpStatus === 200 || d.lastHttpStatus === 201) ? 'var(--accent-emerald)' : 'var(--danger)';
          statusEl.innerHTML = 'Status: <strong>' + escapeHtml(d.lastPushMessage || '') + '</strong>' + 
            (d.lastPushAgoSec >= 0 ? ' (' + d.lastPushAgoSec + 's lalu)' : '');
        }
      } catch (e) {
        console.error('Gagal memuat config Power Logger', e);
      }
    }

    async function testPowerLogger() {
      const url = document.getElementById('logger-url').value.trim();
      const key = document.getElementById('logger-key').value.trim();
      const statusEl = document.getElementById('logger-status');
      const btn = document.getElementById('btnTestLogger');

      if (!url) return alert('Mohon isi Endpoint URL terlebih dahulu.');

      statusEl.style.display = 'block';
      statusEl.textContent = 'Mengirim metrik uji coba...';
      btn.disabled = true;

      try {
        const form = new URLSearchParams();
        form.append('endpointUrl', url);
        form.append('apiKey', key);

        const res = await fetch('/api/power-logger/test', { method: 'POST', body: form });
        const d = await res.json();
        statusEl.style.borderLeftColor = d.status === 'ok' ? 'var(--accent-emerald)' : 'var(--danger)';
        statusEl.innerHTML = '<strong>' + escapeHtml(d.message || '') + '</strong>';
      } catch (e) {
        statusEl.style.borderLeftColor = 'var(--danger)';
        statusEl.textContent = 'Error: ' + e.message;
      } finally {
        btn.disabled = false;
      }
    }

    async function savePowerLoggerConfig() {
      const url = document.getElementById('logger-url').value.trim();
      const key = document.getElementById('logger-key').value.trim();
      const interval = document.getElementById('logger-interval').value.trim() || '15';
      const enabled = document.getElementById('logger-enabled').checked ? '1' : '0';
      const statusEl = document.getElementById('logger-status');
      const btn = document.getElementById('btnSaveLogger');

      statusEl.style.display = 'block';
      statusEl.textContent = 'Menyimpan konfigurasi...';
      btn.disabled = true;

      try {
        const form = new URLSearchParams();
        form.append('endpointUrl', url);
        form.append('apiKey', key);
        form.append('intervalSec', interval);
        form.append('isEnabled', enabled);

        const res = await fetch('/api/power-logger/config', { method: 'POST', body: form });
        const d = await res.json();
        statusEl.style.borderLeftColor = 'var(--accent-emerald)';
        statusEl.textContent = d.message || 'Berhasil disimpan!';
      } catch (e) {
        statusEl.style.borderLeftColor = 'var(--danger)';
        statusEl.textContent = 'Gagal menyimpan: ' + e.message;
      } finally {
        btn.disabled = false;
      }
    }

    function escapeHtml(str) {
      if (!str) return '';
      return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
    }

    // Init load
    loadSysInfo();
    loadWiFiList();
    loadBeszelConfig();
    loadPowerLoggerConfig();
  </script>
</body>
</html>

)rawliteral";

#endif
