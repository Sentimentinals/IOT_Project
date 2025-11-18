// ==================== WEBSOCKET ====================
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
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

            if (tempEl) {
                tempEl.innerHTML = parseFloat(data.temperature).toFixed(1);
            }
            if (humEl) {
                humEl.innerHTML = parseFloat(data.humidity).toFixed(1);
            }
        }
        

    } catch (e) {
        console.warn("Không phải JSON hợp lệ:", event.data);
    }
}


// ==================== UI NAVIGATION ====================
let relayList = [];
let deleteTarget = null;

function showSection(id, event) {
    document.querySelectorAll('.section').forEach(sec => sec.style.display = 'none');
    document.getElementById(id).style.display = id === 'settings' ? 'flex' : 'block';
    document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
    event.currentTarget.classList.add('active');
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
                name: relay.name,
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
        btn.textContent = "BẬT";
    } else {
        btn.classList.remove("on");
        btn.textContent = "TẮT";
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
                statusEl.innerHTML = `📄 Kích thước: ${data.size} bytes | Số dòng: ~${data.lines}`;
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
