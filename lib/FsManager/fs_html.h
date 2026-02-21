const char INDEX_HTML[] = R"rawliteral(
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
        
        /* Storage Info */
        .storage-container { margin-bottom: 20px; }
        .progress-bar { height: 12px; background: #e2e8f0; border-radius: 6px; overflow: hidden; margin: 8px 0; }
        .progress-fill { height: 100%; background: var(--primary); transition: width 0.5s ease-in-out; }
        .storage-text { font-size: 0.85rem; display: flex; justify-content: space-between; color: #64748b; }

        /* Upload Section */
        .upload-zone { border: 2px dashed #cbd5e1; padding: 20px; text-align: center; border-radius: 8px; margin-bottom: 20px; transition: 0.3s; }
        .upload-zone:hover { border-color: var(--primary); background: #eff6ff; }
        
        /* Table Style */
        table { width: 100%; border-collapse: collapse; }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #e2e8f0; }
        th { background: #f1f5f9; font-weight: 600; }
        
        .btn { padding: 6px 12px; border-radius: 6px; border: none; cursor: pointer; font-weight: 500; transition: 0.2s; font-size: 0.85rem; margin-left: 4px; }
        .btn-upload { background: var(--primary); color: white; padding: 10px 20px; font-size: 1rem; }
        .btn-delete { background: #fee2e2; color: var(--danger); }
        .btn-delete:hover { background: var(--danger); color: white; }
        .btn-show { background: #dcfce7; color: var(--success); }
        .btn-show:hover { background: var(--success); color: white; }
        .btn-refresh { background: #e2e8f0; color: #475569; }
        
        .status { margin-top: 10px; padding: 10px; border-radius: 6px; display: none; font-size: 0.9rem; }
    </style>
</head>
<body>

<div class="container">
    <div class="card">
        <div class="storage-container">
            <div class="storage-text">
                <strong>Penyimpanan Sistem</strong>
                <span id="storagePercent">0% Terpakai</span>
            </div>
            <div class="progress-bar">
                <div id="storageFill" class="progress-fill" style="width: 0%"></div>
            </div>
            <div class="storage-text">
                <span id="storageUsed">Used: 0 KB</span>
                <span id="storageFree">Free: 0 KB</span>
            </div>
        </div>
    </div>

    <div class="card">
        <h2>
            📁 File Manager
            <button class="btn btn-refresh" onclick="refreshData()">🔄 Refresh</button>
        </h2>

        <div class="upload-zone">
            <input type="file" id="fileInput">
            <button class="btn btn-upload" onclick="uploadFile()">Upload File</button>
            <div id="uploadStatus" class="status"></div>
        </div>

        <div id="tableContainer">
            <table>
                <thead>
                    <tr>
                        <th>Nama File</th>
                        <th>Ukuran</th>
                        <th style="text-align: right;">Aksi</th>
                    </tr>
                </thead>
                <tbody id="fileList"></tbody>
            </table>
        </div>
    </div>
</div>

<script>
    const fileListElement = document.getElementById('fileList');
    const uploadStatus = document.getElementById('uploadStatus');

    // REFRESH SEMUA DATA
    function refreshData() {
        fetchFiles();
        fetchStorageInfo();
    }

    // 1. INFO PENYIMPANAN (/fs/info)
    async function fetchStorageInfo() {
        try {
            const response = await fetch('/fs/info');
            const info = await response.json();
            
            document.getElementById('storageFill').style.width = info.usedPercent + '%';
            document.getElementById('storagePercent').innerText = info.usedPercent + '% Terpakai';
            document.getElementById('storageUsed').innerText = `Terpakai: ${formatBytes(info.used)}`;
            document.getElementById('storageFree').innerText = `Tersisa: ${formatBytes(info.free)}`;
            
            // Ubah warna bar jika hampir penuh
            const bar = document.getElementById('storageFill');
            bar.style.backgroundColor = info.usedPercent > 85 ? '#dc2626' : '#2563eb';
        } catch (err) {
            console.error('Gagal mengambil info storage:', err);
        }
    }

    // 2. DAFTAR FILE (/fs/list)
    async function fetchFiles() {
        try {
            const response = await fetch('/fs/list');
            const data = await response.json();
            const files = data.files || data; // Handle jika format {files: []} atau []
            
            fileListElement.innerHTML = '';
            files.forEach(file => {
                const isImage = /\.(jpg|jpeg|png|gif|bmp)$/i.test(file.name);
                let actionButtons = '';

                // Tambahkan tombol Show LCD jika file adalah gambar
                if (isImage) {
                    actionButtons += `<button class="btn btn-show" onclick="showOnLcd('${file.name}')">📺 Show LCD</button>`;
                }

                actionButtons += `<button class="btn btn-delete" onclick="deleteFile('${file.name}')">Hapus</button>`;

                const row = `
                    <tr>
                        <td>${file.name}</td>
                        <td>${formatBytes(file.size)}</td>
                        <td style="text-align: right;">${actionButtons}</td>
                    </tr>
                `;
                fileListElement.innerHTML += row;
            });
        } catch (err) {
            console.error('Gagal mengambil file:', err);
        }
    }

    // 3. TAMPILKAN KE LCD (/lcd/show?img=...)
    async function showOnLcd(filename) {
        try {
            const response = await fetch(`/lcd/show?img=${filename}`);
            if (response.ok) {
                alert(`Perintah kirim gambar ${filename} ke LCD berhasil!`);
            } else {
                alert('Gagal mengirim ke LCD');
            }
        } catch (err) {
            console.error('Error LCD:', err);
        }
    }

    // 4. UPLOAD FILE (/fs/upload)
    async function uploadFile() {
        const fileInput = document.getElementById('fileInput');
        if (fileInput.files.length === 0) return alert('Pilih file dulu!');

        const formData = new FormData();
        formData.append('data', fileInput.files[0], '/' + fileInput.files[0].name);

        showStatus('Sedang mengunggah...', '#e0f2fe', '#0369a1');

        try {
            const response = await fetch('/fs/upload', { method: 'POST', body: formData });
            if (response.ok) {
                showStatus('Upload Berhasil!', '#dcfce7', '#15803d');
                fileInput.value = '';
                refreshData();
            } else {
                throw new Error('Upload gagal');
            }
        } catch (err) {
            showStatus('Gagal: ' + err.message, '#fee2e2', '#b91c1c');
        }
    }

    // 5. HAPUS FILE (/fs/delete)
    async function deleteFile(path) {
        if (!confirm(`Hapus file ${path}?`)) return;
        const formData = new FormData();
        formData.append('path', '/' + path);

        try {
            const response = await fetch('/fs/delete', { method: 'POST', body: formData });
            if (response.ok) {
                refreshData();
            } else {
                alert('Gagal menghapus file');
            }
        } catch (err) {
            console.error('Error:', err);
        }
    }

    function formatBytes(bytes, decimals = 2) {
        if (bytes === 0) return '0 Bytes';
        const k = 1024;
        const dm = decimals < 0 ? 0 : decimals;
        const sizes = ['Bytes', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
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
