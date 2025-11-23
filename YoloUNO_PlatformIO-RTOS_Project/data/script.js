// ==================== WEBSOCKET ====================
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// ==================== DARK MODE ====================
function initTheme() {
    const savedTheme = localStorage.getItem('theme') || 'light';
    document.documentElement.dataset.theme = savedTheme;
    updateThemeIcon();
}

function toggleTheme() {
    const current = document.documentElement.dataset.theme || 'light';
    const newTheme = current === 'dark' ? 'light' : 'dark';
    document.documentElement.dataset.theme = newTheme;
    localStorage.setItem('theme', newTheme);
    updateThemeIcon();
    console.log('🎨 Theme:', newTheme);
}

function updateThemeIcon() {
    const icon = document.querySelector('.theme-icon');
    if (icon) {
        const theme = document.documentElement.dataset.theme;
        icon.textContent = theme === 'dark' ? '☀️' : '🌙';
    }
}

// ==================== ENVIRONMENT STATUS ====================
function getEnvironmentStatus(temp, humidity) {
    let tempStatus = { text: 'Loading...', class: '' };
    let humStatus = { text: 'Loading...', class: '' };

    // Temperature status - MATCHES LED LOGIC from neo_blinky.cpp
    if (temp !== undefined && temp !== '--') {
        if (temp >= 50) {
            tempStatus = { text: 'Very Hot!', class: 'status-alert' };
        } else if (temp >= 40) {
            tempStatus = { text: 'Hot', class: 'status-alert' };
        } else if (temp >= 25) {
            tempStatus = { text: 'Comfortable', class: 'status-comfortable' }; // LED = Green
        } else if (temp >= 15) {
            tempStatus = { text: 'Cool', class: 'status-warning' }; // LED = Cyan
        } else {
            tempStatus = { text: 'Too Cold', class: 'status-alert' }; // LED = Blue
        }
    }

    // Humidity status
    if (humidity !== undefined && humidity !== '--') {
        if (humidity > 70) {
            humStatus = { text: 'Too Humid', class: 'status-warning' };
        } else if (humidity >= 40) {
            humStatus = { text: 'Optimal', class: 'status-comfortable' };
        } else {
            humStatus = { text: 'Dry', class: 'status-warning' };
        }
    }

    return { tempStatus, humStatus };
}

function updateEnvironmentBadges(temp, humidity) {
    const { tempStatus, humStatus } = getEnvironmentStatus(temp, humidity);

    const tempBadge = document.getElementById('tempStatus');
    const humBadge = document.getElementById('humStatus');

    if (tempBadge) {
        tempBadge.className = `status-badge ${tempStatus.class}`;
        tempBadge.textContent = tempStatus.text;
    }

    if (humBadge) {
        humBadge.className = `status-badge ${humStatus.class}`;
        humBadge.textContent = humStatus.text;
    }
}

window.addEventListener('load', onLoad);

function onLoad(event) {
    initTheme();
    initWebSocket();
}

function onOpen(event) {
    console.log('Connection opened');
    // Update WiFi status
    const wifiCard = document.getElementById('wifiCard');
    const wifiStatus = document.getElementById('wifiStatus');
    const wifiIcon = document.getElementById('wifiIcon');
    if (wifiCard) {
        wifiCard.classList.add('connected');
        wifiCard.classList.remove('disconnected');
    }
    if (wifiStatus) wifiStatus.textContent = 'Connected';
    if (wifiIcon) wifiIcon.textContent = '📶';
}

function onClose(event) {
    console.log('Connection closed');
    // Update WiFi status
    const wifiCard = document.getElementById('wifiCard');
    const wifiStatus = document.getElementById('wifiStatus');
    const wifiIcon = document.getElementById('wifiIcon');
    if (wifiCard) {
        wifiCard.classList.add('disconnected');
        wifiCard.classList.remove('connected');
    }
    if (wifiStatus) wifiStatus.textContent = 'Disconnected';
    if (wifiIcon) wifiIcon.textContent = '❌';
    setTimeout(initWebSocket, 2000);
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function Send_Data(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(data);
        console.log("📤 Gửi:", data);
    } else {
        console.warn("⚠️ WebSocket chưa sẵn sàng!");
        alert("⚠️ WebSocket chưa kết nối!");
    }
}

function onMessage(event) {
    console.log("📩 Nhận:", event.data);
    try {
        var data = JSON.parse(event.data);
        if (data.type === "sensor") {

            const tempEl = document.getElementById("temp");
            const humEl = document.getElementById("hum");

            if (tempEl && data.temperature !== undefined) {
                tempEl.innerHTML = parseFloat(data.temperature).toFixed(1);
            }
            if (humEl && data.humidity !== undefined) {
                humEl.innerHTML = parseFloat(data.humidity).toFixed(1);
            }

            // New sensors
            if (data.light !== undefined) {
                const lightEl = document.getElementById("light");
                if (lightEl) lightEl.innerHTML = Math.round(data.light);  // Show as integer Lux
            }
            if (data.moisture !== undefined) {
                const moistureEl = document.getElementById("moisture");
                if (moistureEl) moistureEl.innerHTML = parseFloat(data.moisture).toFixed(1);
            }
            if (data.flame !== undefined) {
                const flameEl = document.getElementById("flame");
                const flameCard = flameEl?.closest('.gauge-card');

                if (flameEl) {
                    if (data.flame === true) {
                        flameEl.innerHTML = "FIRE!";
                        // Add alert class to card
                        if (flameCard) {
                            flameCard.classList.add('flame-alert');
                            flameCard.classList.remove('flame-safe');
                        }
                        // TRIGGER FULL DASHBOARD ALERT
                        document.body.classList.add('emergency-alert');
                    } else {
                        flameEl.innerHTML = "Safe";
                        // Add safe class to card
                        if (flameCard) {
                            flameCard.classList.add('flame-safe');
                            flameCard.classList.remove('flame-alert');
                        }
                        // REMOVE FULL DASHBOARD ALERT
                        document.body.classList.remove('emergency-alert');
                    }
                }
            }

            // Update environment status badges
            if (data.temperature !== undefined && data.humidity !== undefined) {
                updateEnvironmentBadges(data.temperature, data.humidity);
            }
        }


    } catch (e) {
        console.warn("Không phải JSON hợp lệ:", event.data);
    }
}


// ==================== UI NAVIGATION ====================
// Initialize with Fan as default relay (but deletable)
let relayList = [
    { id: 9999, name: "🌀 Cooling Fan", gpio: 14, state: false }
];
let deleteTarget = null;

function showSection(id, event) {
    document.querySelectorAll('.section').forEach(sec => sec.style.display = 'none');
    document.getElementById(id).style.display = id === 'settings' ? 'flex' : 'block';
    document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
    event.currentTarget.classList.add('active');

    // Update Info section when shown
    if (id === 'info') {
        updateInfo();
        // Auto-refresh every 5 seconds
        if (window.infoInterval) clearInterval(window.infoInterval);
        window.infoInterval = setInterval(updateInfo, 5000);
    } else {
        // Clear interval when leaving Info section
        if (window.infoInterval) {
            clearInterval(window.infoInterval);
            window.infoInterval = null;
        }
    }
}


// ==================== DEVICE FUNCTIONS ====================
function openAddRelayDialog() {
    document.getElementById('addRelayDialog').style.display = 'flex';
}
function closeAddRelayDialog() {
    document.getElementById('addRelayDialog').style.display = 'none';
}
function saveRelay() {
    const name = document.getElementById('relayName').value.trim();
    const gpio = document.getElementById('relayGPIO').value.trim();
    if (!name || !gpio) return alert("⚠️ Please fill all fields!");
    relayList.push({ id: Date.now(), name, gpio, state: false });
    renderRelays();
    closeAddRelayDialog();
}
function renderRelays() {
    const container = document.getElementById('relayContainer');
    container.innerHTML = "";
    relayList.forEach(r => {
        const card = document.createElement('div');
        card.className = 'device-card';
        card.innerHTML = `
      <div class="device-icon">⚡</div>
      <h3>${r.name}</h3>
      <p>GPIO: ${r.gpio}</p>
      <button class="toggle-btn ${r.state ? 'on' : ''}" onclick="toggleRelay(${r.id})">
        ${r.state ? 'ON' : 'OFF'}
      </button>
      <span class="delete-icon" onclick="showDeleteDialog(${r.id})">🗑️</span>
    `;
        container.appendChild(card);
    });
}
function toggleRelay(id) {
    const relay = relayList.find(r => r.id === id);
    if (relay) {
        relay.state = !relay.state;
        const relayJSON = JSON.stringify({
            page: "device",
            value: {
                status: relay.state ? "ON" : "OFF",
                gpio: relay.gpio
            }
        });
        Send_Data(relayJSON);
        renderRelays();
    }
}
function showDeleteDialog(id) {
    deleteTarget = id;
    document.getElementById('confirmDeleteDialog').style.display = 'flex';
}
function closeConfirmDelete() {
    document.getElementById('confirmDeleteDialog').style.display = 'none';
}
function confirmDelete() {
    relayList = relayList.filter(r => r.id !== deleteTarget);
    renderRelays();
    closeConfirmDelete();
}


// ==================== NEO LED CONTROL ====================
let neoLedState = true; // Mặc định BẬT

function toggleNeoLED() {
    neoLedState = !neoLedState;
    const btn = document.getElementById("neoLedBtn");

    if (neoLedState) {
        btn.classList.add("on");
        btn.textContent = "ON";
    } else {
        btn.classList.remove("on");
        btn.textContent = "OFF";
    }

    // Gửi lệnh qua WebSocket
    const ledJSON = JSON.stringify({
        page: "neoled",
        value: {
            enabled: neoLedState
        }
    });

    Send_Data(ledJSON);
    console.log("💡 NeoLED:", neoLedState ? "BẬT" : "TẮT");
}


// ==================== FAN CONTROL ====================
let fanState = false; // Mặc định TẮT

function toggleFan() {
    fanState = !fanState;
    const btn = document.getElementById("fanBtn");

    if (fanState) {
        btn.classList.add("on");
        btn.classList.remove("off");
        btn.textContent = "ON";
    } else {
        btn.classList.remove("on");
        btn.classList.add("off");
        btn.textContent = "OFF";
    }

    // Gửi lệnh qua WebSocket
    const fanJSON = JSON.stringify({
        page: "fan_control",
        value: {
            enabled: fanState
        }
    });

    Send_Data(fanJSON);
    console.log("🌀 Fan:", fanState ? "BẬT" : "TẮT");
}


// ==================== CSV CONTROLS ====================
function downloadCSV() {
    console.log("📥 Đang tải file CSV...");
    window.location.href = "/download";
}

function clearCSV() {
    if (confirm("⚠️ Bạn có chắc muốn xóa toàn bộ dữ liệu CSV?")) {
        fetch("/clear")
            .then(response => response.text())
            .then(data => {
                alert(data);
                updateCSVInfo();
            })
            .catch(err => {
                console.error("❌ Lỗi xóa CSV:", err);
                alert("❌ Lỗi kết nối!");
            });
    }
}

function updateCSVInfo() {
    fetch("/csv-info")
        .then(response => response.json())
        .then(data => {
            const statusEl = document.getElementById("csvStatus");
            if (data.exists) {
                statusEl.innerHTML = `📄 Size: ${data.size} bytes | Lines: ~${data.lines}`;
            } else {
                statusEl.innerHTML = "❌ Chưa có dữ liệu";
            }
        })
        .catch(err => {
            console.error("❌ Lỗi lấy thông tin CSV:", err);
            document.getElementById("csvStatus").innerHTML = "⚠️ Không thể lấy thông tin";
        });
}

// Cập nhật thông tin CSV mỗi 10 giây
setInterval(updateCSVInfo, 10000);
// Và cập nhật ngay khi load trang
setTimeout(updateCSVInfo, 2000);


// ==================== SETTINGS FORM (BỔ SUNG) ====================
document.getElementById("settingsForm").addEventListener("submit", function (e) {
    e.preventDefault();

    const ssid = document.getElementById("ssid").value.trim();
    const password = document.getElementById("password").value.trim();
    const token = document.getElementById("token").value.trim();
    const server = document.getElementById("server").value.trim();
    const port = document.getElementById("port").value.trim();

    const settingsJSON = JSON.stringify({
        page: "setting",
        value: {
            ssid: ssid,
            password: password,
            token: token,
            server: server,
            port: port
        }
    });

    Send_Data(settingsJSON);
    alert("✅ Cấu hình đã được gửi đến thiết bị!");
});

// ==================== INFO SECTION ====================
function updateInfo() {
    fetch("/info")
        .then(response => response.json())
        .then(data => {
            // System info
            document.getElementById("chipModel").textContent = data.chipModel || "ESP32";
            document.getElementById("freeHeap").textContent = (data.freeHeap / 1024).toFixed(1) + " KB";
            document.getElementById("systemUptime").textContent = formatUptime(data.uptime);

            // WiFi info
            document.getElementById("wifiSSID").textContent = data.wifiSSID || "--";
            document.getElementById("ipAddress").textContent = data.ipAddress || "--";
            document.getElementById("wifiSignal").textContent = data.wifiSignal ? data.wifiSignal + " dBm" : "--";
        })
        .catch(err => console.error("❌ Error fetching info:", err));
}

function formatUptime(seconds) {
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;

    if (days > 0) {
        return `${days}d ${hours}h ${minutes}m`;
    } else if (hours > 0) {
        return `${hours}h ${minutes}m ${secs}s`;
    } else {
        return `${minutes}m ${secs}s`;
    }
}
