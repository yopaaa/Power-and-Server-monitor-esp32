const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP File Manager Pro</title>
    <style>
        :root {
            --primary: #2563eb;
            --success: #16a34a;
            --danger: #dc2626;
            --bg: #f8fafc;
            --card: #ffffff;
        }
        body { font-family: system-ui, sans-serif; background: var(--bg); margin: 0; padding: 20px; color: #1e293b; }
        .container { max-width: 900px; margin: 0 auto; }
        .card { background: var(--card); padding: 20px; border-radius: 12px; box-shadow: 0 4px 6px -1px rgb(0 0 0 / 0.1); margin-bottom: 20px; }
        
        h2 { margin-top: 0; display: flex; justify-content: space-between; align-items: center; }
        
        .storage-text { font-size: 0.85rem; display: flex; justify-content: space-between; color: #64748b; }
        .progress-bar { height: 12px; background: #e2e8f0; border-radius: 6px; overflow: hidden; margin: 8px 0; }
        .progress-fill { height: 100%; background: var(--primary); transition: width 0.5s; }

        .upload-zone { border: 2px dashed #cbd5e1; padding: 20px; text-align: center; border-radius: 8px; margin-bottom: 20px; }
        .upload-zone.dragover { border-color: var(--primary); background: #eff6ff; }
        
        .breadcrumb { margin-bottom: 15px; font-weight: bold; color: var(--primary); font-size: 1.1rem; }
        .breadcrumb span { cursor: pointer; text-decoration: underline; margin-right: 5px; }

        table { width: 100%; border-collapse: collapse; }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #e2e8f0; }
        tr:hover { background: #f8fafc; }
        
        .btn { padding: 6px 12px; border-radius: 6px; border: none; cursor: pointer; font-weight: 500; font-size: 0.85rem; margin-left: 4px; }
        .btn-upload { background: var(--primary); color: white; padding: 10px 20px; }
        .btn-delete { background: #fee2e2; color: var(--danger); }
        .btn-show { background: #dcfce7; color: var(--success); }
        .folder-icon { color: #f59e0b; margin-right: 8px; font-weight: bold; }
        
        .status { margin-top: 10px; padding: 10px; border-radius: 6px; display: none; font-size: 0.9rem; }
        .nav-controls { margin-bottom: 15px; display: flex; gap: 10px; }
    </style>
</head>
<body>

<div class="container">
    <div class="card">
        <div class="storage-text">
            <strong>Penyimpanan LittleFS</strong>
            <span id="storagePercent">0%</span>
        </div>
        <div class="progress-bar"><div id="storageFill" class="progress-fill"></div></div>
        <div class="storage-text">
            <span id="storageUsed">Used: 0 KB</span>
            <span id="storageFree">Free: 0 KB</span>
        </div>
    </div>

    <div class="card">
        <h2>📁 File Explorer <button class="btn btn-refresh" onclick="refreshData()">🔄 Refresh</button></h2>
        
        <div class="nav-controls">
            <button class="btn btn-refresh" onclick="createFolder()">+ Folder Baru</button>
        </div>

        <div class="breadcrumb" id="breadcrumb">Path: /</div>

        <div class="upload-zone" id="dropZone">
            <input type="file" id="fileInput" multiple hidden>
            <button class="btn btn-upload" onclick="document.getElementById('fileInput').click()">Pilih Banyak File</button>
            <p style="font-size: 0.8rem; color: #64748b;">Atau Drag & Drop file ke sini</p>
            <div id="uploadStatus" class="status"></div>
        </div>

        <table>
            <thead>
                <tr>
                    <th>Nama</th>
                    <th>Ukuran</th>
                    <th style="text-align: right;">Aksi</th>
                </tr>
            </thead>
            <tbody id="fileList"></tbody>
        </table>
    </div>
</div>

<script>
    let currentPath = "/";
    const fileListElement = document.getElementById('fileList');
    const uploadStatus = document.getElementById('uploadStatus');

    function refreshData() {
        fetchFiles(currentPath);
        fetchStorageInfo();
    }

    async function fetchStorageInfo() {
        try {
            const res = await fetch('/fs/info');
            const info = await res.json();
            document.getElementById('storageFill').style.width = info.usedPercent + '%';
            document.getElementById('storagePercent').innerText = info.usedPercent + '% Terpakai';
            document.getElementById('storageUsed').innerText = `Used: ${formatBytes(info.used)}`;
            document.getElementById('storageFree').innerText = `Free: ${formatBytes(info.free)}`;
        } catch (err) { console.error(err); }
    }

    async function fetchFiles(path) {
        currentPath = path;
        document.getElementById('breadcrumb').innerText = `Path: ${currentPath}`;
        try {
            // Kita asumsikan API /fs/list?path=... mengembalikan array [{name, size, isDir}]
            const res = await fetch(`/fs/list?path=${path}`);
            const files = await res.json();
            
            fileListElement.innerHTML = '';

            // Tombol Kembali (Back)
            if (path !== "/") {
                const row = `<tr onclick="goUp()" style="cursor:pointer; color:var(--primary)">
                    <td colspan="3"><b>⬅ .. (Kembali ke Folder Induk)</b></td>
                </tr>`;
                fileListElement.innerHTML += row;
            }

            files.forEach(file => {
                const isImg = /\.(jpg|jpeg|png|gif)$/i.test(file.name);
                const fullPath = (currentPath === "/" ? "" : currentPath) + "/" + file.name;
                
                let actionBtns = `<button class="btn btn-delete" onclick="deleteItem('${fullPath}')">Hapus</button>`;
                if (isImg && !file.isDir) {
                    actionBtns = `<button class="btn btn-show" onclick="showOnLcd('${fullPath}')">📺 Show</button>` + actionBtns;
                }

                const nameDisplay = file.isDir 
                    ? `<span class="folder-icon">📁</span><a href="#" onclick="enterFolder('${file.name}')">${file.name}</a>` 
                    : file.name;

                fileListElement.innerHTML += `
                    <tr>
                        <td>${nameDisplay}</td>
                        <td>${file.isDir ? '--' : formatBytes(file.size)}</td>
                        <td style="text-align: right;">${actionBtns}</td>
                    </tr>`;
            });
        } catch (err) { console.error(err); }
    }

    function enterFolder(folderName) {
        const newPath = (currentPath === "/" ? "" : currentPath) + "/" + folderName;
        fetchFiles(newPath);
    }

    function goUp() {
        const parts = currentPath.split('/').filter(p => p !== "");
        parts.pop();
        fetchFiles("/" + parts.join('/'));
    }

    async function createFolder() {
        const name = prompt("Nama folder baru:");
        if (!name) return;
        const path = (currentPath === "/" ? "" : currentPath) + "/" + name;
        const fd = new FormData();
        fd.append('path', path);
        await fetch('/fs/mkdir', {method:'POST', body: fd});
        refreshData();
    }

    async function uploadFile() {
        const input = document.getElementById('fileInput');
        const files = input.files;
        if (files.length === 0) return;

        showStatus(`Mengunggah ${files.length} file...`, '#e0f2fe', '#0369a1');

        for (let i = 0; i < files.length; i++) {
            const formData = new FormData();
            // Simpan file ke path saat ini
            const fullPath = (currentPath === "/" ? "" : currentPath) + "/" + files[i].name;
            formData.append('data', files[i], fullPath);

            try {
                await fetch('/fs/upload', { method: 'POST', body: formData });
            } catch (err) { console.error("Gagal upload:", files[i].name); }
        }

        showStatus('Semua file berhasil diunggah!', '#dcfce7', '#15803d');
        input.value = '';
        refreshData();
    }

    // Listener untuk Multiple Upload Otomatis
    document.getElementById('fileInput').addEventListener('change', uploadFile);

    async function deleteItem(path) {
        if (!confirm(`Hapus ${path}?`)) return;
        const fd = new FormData();
        fd.append('path', path);
        await fetch('/fs/delete', { method: 'POST', body: fd });
        refreshData();
    }

    async function showOnLcd(path) {
        await fetch(`/lcd/show?img=${path}`);
        alert("Gambar dikirim ke LCD");
    }

    function formatBytes(bytes) {
        if (bytes === 0) return '0 B';
        const i = Math.floor(Math.log(bytes) / Math.log(1024));
        return (bytes / Math.pow(1024, i)).toFixed(1) + ' ' + ['B', 'KB', 'MB'][i];
    }

    function showStatus(msg, bg, color) {
        uploadStatus.innerText = msg;
        uploadStatus.style.display = 'block';
        uploadStatus.style.backgroundColor = bg;
        uploadStatus.style.color = color;
        setTimeout(() => { uploadStatus.style.display = 'none'; }, 3000);
    }

    window.onload = refreshData;
</script>
</body>
</html>
)rawliteral";
