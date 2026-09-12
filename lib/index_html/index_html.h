const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
  <head>
    <title>ESP Config</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style>
      body {
        font-family: sans-serif;
        margin: 20px;
        line-height: 1.6;
        color: #333;
      }
      .container {
        padding: 20px;
        margin: auto;
      }
      input[type="text"],
      input[type="password"] {
        width: 100%;
        padding: 8px;
        margin: 5px 0 15px;
        box-sizing: border-box;
      }
      .hidden {
        display: none;
      }
      pre {
        background: #eee;
        padding: 10px;
        border-radius: 4px;
        font-size: 12px;
      }
      button {
        width: 100%;
        padding: 10px;
        cursor: pointer;
        border: none;
        border-radius: 4px;
        margin-bottom: 10px;
        font-weight: bold;
      }
      #btnTest {
        background: #008cba;
        color: white;
      }
      #btnSave {
        background: #4caf50;
        color: white;
      }
      .deleteBtn {
        background: #e53935;
        color: white;
        margin-top: 5px;
      }
      .card {
        border: 1px solid #ddd;
        padding: 15px;
        border-radius: 8px;
        box-shadow: 2px 2px 5px #eee;
        margin-bottom: 20px;
      }
      .wifiItem {
        border-bottom: 1px solid #eee;
        padding: 8px 0;
      }
      .container-card {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 20px;
      }

      @media (max-width: 700px) {
        .container {
          padding: 10px;
          margin: auto;
        }
        .container-card {
          display: grid;
          grid-template-columns: 1fr;
          gap: 10px;
        }
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h2>WiFi Config</h2>

      <div class="container-card">
        <div class="card">
          <h3>Tambah / Update WiFi</h3>
          SSID: <input id="ssid" type="text" placeholder="Nama WiFi" />
          <br />
          Password:<input id="pass" type="password" placeholder="Password" />

          <label
            ><input
              type="checkbox"
              id="static"
              onchange="toggleStatic()"
              disabled
            />
            Gunakan Static IP</label
          ><br /><br />

          <div id="static_fields" class="hidden">
            IP Address:
            <input id="ip" type="text" value="192.168.1.15" /> Gateway:
            <input id="gw" type="text" value="192.168.1.1" /> Subnet:
            <input id="sn" type="text" value="255.255.255.0" />
          </div>

          <button type="button" id="btnTest">Cek Koneksi</button>
          <button type="button" id="btnSave">Simpan WiFi</button>

          <strong>Status:</strong>
          <pre id="out">Ready</pre>
        </div>

        <div class="card">
          <h3>WiFi Tersimpan</h3>
          <div id="wifiList">Loading...</div>
        </div>

        <div class="card" style="grid-column: 1 / -1;">
          <h3>Web OTA Firmware Update</h3>
          <p style="color: #666; font-size: 13px; margin: 0 0 10px 0;">Upload file <code>firmware.bin</code> untuk flash firmware baru secara Over-The-Air.</p>
          <form id="ota-form">
            <input type="file" id="file-input" name="update" accept=".bin" required style="margin-bottom: 10px;" />
            <button type="submit" id="btnUpdate" style="background: #e67e22; color: white;">Mulai Update Firmware</button>
          </form>
          <div id="prg-bar" style="display:none; background: #eee; border-radius: 4px; height: 18px; margin-top: 12px; overflow: hidden; border: 1px solid #ccc;">
            <div id="prg-fill" style="background: #27ae60; height: 100%; width: 0%; transition: width 0.15s;"></div>
          </div>
          <div id="ota-status" style="margin-top: 10px; font-weight: bold; font-family: monospace; font-size: 13px;"></div>
        </div>
      </div>
    </div>

    <script>
      const out = document.getElementById("out");
      const wifiListDiv = document.getElementById("wifiList");

      function toggleStatic() {
        document
          .getElementById("static_fields")
          .classList.toggle(
            "hidden",
            !document.getElementById("static").checked,
          );
      }

      async function loadWiFiList() {
        try {
          const res = await fetch("/wifi/data");
          const data = await res.json();
          

          if (!data.wifiList || data.wifiList.length === 0) {
            wifiListDiv.innerHTML = "Belum ada WiFi tersimpan.";
            return;
          }

          wifiListDiv.innerHTML = "";

          data.wifiList.forEach((w) => {
            const div = document.createElement("div");
            div.className = "wifiItem";
            div.innerHTML = `
            <strong>${w.ssid}</strong><br>
            Static: ${w.useStatic}<br>
            IP: ${w.ip}<br>
            Gateway: ${w.gateway}<br>
            Subnet: ${w.subnet}
            <button class="deleteBtn" onclick="deleteWiFi('${w.ssid}')">
              Delete
            </button>
          `;
            wifiListDiv.appendChild(div);
          });
        } catch (e) {
          wifiListDiv.innerHTML = "Gagal memuat data.";
        }
      }

      async function deleteWiFi(ssid) {
        if (!confirm("Hapus WiFi: " + ssid + " ?")) return;

        const params = new URLSearchParams();
        params.append("ssid", ssid);

        const res = await fetch("/wifi/delete", {
          method: "POST",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: params,
        });

        const d = await res.json();

        if (d.status === "ok") {
          loadWiFiList();
        }
      }

      async function sendData(path) {
        out.textContent = "Memproses...";

        const params = new URLSearchParams();
        params.append("ssid", document.getElementById("ssid").value);
        params.append("pass", document.getElementById("pass").value);

        if (document.getElementById("static").checked)
          params.append("static", "1");

        params.append("ip", document.getElementById("ip").value);
        params.append("gw", document.getElementById("gw").value);
        params.append("sn", document.getElementById("sn").value);

        try {
          const res = await fetch(path, {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params,
          });

          const d = await res.json();

          if (path === "/wifi/test") {
            if (d.ok) {
              out.textContent = `BERHASIL\nIP: ${d.ip}\nGW: ${d.gw}\nSN: ${d.sn}`;
              document.getElementById("ip").value = d.ip;
              document.getElementById("gw").value = d.gw;
              document.getElementById("sn").value = d.sn;
              document.getElementById("static").checked = true;
              toggleStatic();
            } else {
              out.textContent = "GAGAL koneksi";
            }
          } else {
            out.textContent = d.status === "ok" ? "Tersimpan" : "Gagal";

            loadWiFiList();
          }
        } catch (e) {
          out.textContent = "Error koneksi";
        }
      }

      document.getElementById("btnTest").onclick = () => sendData("/wifi/test");

      document.getElementById("btnSave").onclick = () => sendData("/wifi/add");

      document.getElementById("ota-form").addEventListener("submit", function(e) {
        e.preventDefault();
        const fileInput = document.getElementById("file-input");
        if (fileInput.files.length === 0) return;
        const file = fileInput.files[0];
        const formData = new FormData();
        formData.append("update", file);

        const xhr = new XMLHttpRequest();
        const prgBar = document.getElementById("prg-bar");
        const prgFill = document.getElementById("prg-fill");
        const status = document.getElementById("ota-status");
        const btn = document.getElementById("btnUpdate");

        prgBar.style.display = "block";
        prgFill.style.width = "0%";
        status.innerText = "Memulai upload...";
        btn.disabled = true;

        xhr.upload.addEventListener("progress", function(e) {
          if (e.lengthComputable) {
            const p = Math.round((e.loaded / e.total) * 100);
            prgFill.style.width = p + "%";
            status.innerText = "Uploading: " + p + "%";
          }
        });

        xhr.addEventListener("load", function() {
          status.innerText = xhr.responseText;
          if (xhr.status === 200) {
            prgFill.style.width = "100%";
            status.innerText = xhr.responseText + " Silakan refresh halaman setelah ESP32 restart.";
          } else {
            btn.disabled = false;
          }
        });

        xhr.addEventListener("error", function() {
          status.innerText = "Upload gagal! Periksa koneksi ke perangkat.";
          btn.disabled = false;
        });

        xhr.open("POST", "/update");
        xhr.send(formData);
      });

      loadWiFiList();
    </script>
  </body>
</html>
)rawliteral";
