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
    renderRelays();
    loadNotifications();
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
                const flameCard = document.getElementById('flameCard');
                if (flameEl) {
                    if (data.flame === true) {
                        // Fire detected!
                        flameEl.innerHTML = '<span class="flame-icon">🔥</span><span class="flame-text">FIRE!</span>';

                        // Add alert class to card
                        if (flameCard) {
                            flameCard.classList.add('flame-alert');
                            flameCard.classList.remove('flame-safe');
                        }

                        // Trigger full page red blinking
                        document.body.classList.add('emergency-alert');

                        // Show notification (only once per alert)
                        if (!flameAlertActive) {
                            flameAlertActive = true;

                            // Update alert counter
                            alertCount++;
                            updateAlertBadge();

                            // Add to notification panel
                            addNotification(
                                'fire',
                                '🔥 FIRE ALERT!',
                                'Fire detected! Check flame sensor immediately!',
                                '🔥'
                            );

                            // Show fire notification toast
                            showFireNotification("🔥 FIRE DETECTED! Check flame sensor immediately!");
                        }
                    } else {
                        // Fire cleared
                        flameEl.innerHTML = '<span class="flame-icon">✓</span><span class="flame-text">Safe</span>';

                        // Remove alert class
                        if (flameCard) {
                            flameCard.classList.add('flame-safe');
                            flameCard.classList.remove('flame-alert');
                        }

                        // Remove full page blinking
                        document.body.classList.remove('emergency-alert');

                        // Reset alert flag
                        if (flameAlertActive) {
                            flameAlertActive = false;
                            dismissFireNotification();
                        }
                    }
                }
            }
            // WiFi Status notifications
            if (data.wifiStatus !== undefined) {
                if (data.wifiStatus === 'connected') {
                    addNotification(
                        'wifi',
                        'WiFi Connected',
                        `Connected to ${data.ssid || 'network'}. IP: ${data.ip || '--'}`
                    );
                } else if (data.wifiStatus === 'failed') {
                    addNotification(
                        'wifi',
                        'WiFi Failed',
                        'Failed to connect to WiFi. AP mode still active at 192.168.4.1'
                    );
                } else if (data.wifiStatus === 'connecting') {
                    addNotification(
                        'wifi',
                        'WiFi Connecting',
                        `Attempting to connect to ${data.ssid || 'network'}...`
                    );
                } else if (data.wifiStatus === 'ap_mode') {
                    addNotification(
                        'info',
                        '📶 Access Point Mode',
                        `Device running in AP mode. IP: ${data.ip || '192.168.4.1'}`,
                        '📶'
                    );
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
    document.querySelectorAll('.section').forEach(sec => {
        sec.style.display = 'none';
    });
    const targetSection = document.getElementById(id);
    if (targetSection) {
        targetSection.style.display = id === 'settings' ? 'flex' : 'block';
    }

    // XÓA TẤT CẢ active class trước
    document.querySelectorAll('.nav-item').forEach(item => {
        item.classList.remove('active');
    });

    // THÊM active class cho item được click
    if (event && event.currentTarget) {
        event.currentTarget.classList.add('active');
    }
    // Update Info section when shown
    if (id === 'info') {
        updateInfo();
        if (window.infoInterval) clearInterval(window.infoInterval);
        window.infoInterval = setInterval(updateInfo, 5000);
    } else {
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
    if (!name || !gpio) return alert("Please fill all fields!");

    relayList.push({ id: Date.now(), name, gpio, state: false });
    renderRelays();
    closeAddRelayDialog();
    addNotification(
        'info',
        '➕ Device Added',
        `New relay "${name}" added on GPIO ${gpio}`
    );
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
    console.log("💡 NeoLED:", neoLedState ? "On" : "Off");
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
    console.log("🌀 Fan:", fanState ? "On" : "Off");
}


// ==================== CSV CONTROLS ====================
function downloadCSV() {
    console.log("📥 Downloading CSV file...");
    window.location.href = "/download";
}

function clearCSV() {
    if (confirm("Do you sure want to delete all CSV data?")) {
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
    addNotification(
        'info',
        'Settings Saved',
        `Configuration saved. Connecting to WiFi: ${ssid}`
    );
    alert("Configuration sent! Device will restart and connect to WiFi.");
    addNotification(
        'wifi',
        'WiFi Connecting',
        `Attempting to connect to ${ssid}...`
    );

    alert("Configuration sent! Device will restart and connect to WiFi.");
});

// ==================== INFO SECTION ====================
function updateInfo() {
    fetch("/info")
        .then(response => response.json())
        .then(data => {
            // System info
            document.getElementById("chipModel").textContent = data.chipModel || "Yolo UNO";
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
let baseUptime = 0;
let lastUptimeUpdate = 0;
function updateInfo() {
    fetch("/info")
        .then(response => response.json())
        .then(data => {
            // System info
            document.getElementById("chipModel").textContent = data.chipModel || "Yolo UNO";
            document.getElementById("freeHeap").textContent = (data.freeHeap / 1024).toFixed(1) + " KB";

            // Lưu uptime từ server
            baseUptime = data.uptime || 0;
            lastUptimeUpdate = Date.now();

            // Update uptime display
            updateUptimeDisplay();
            // WiFi info
            document.getElementById("wifiSSID").textContent = data.wifiSSID || "--";
            document.getElementById("ipAddress").textContent = data.ipAddress || "--";
            document.getElementById("wifiSignal").textContent = data.wifiSignal ? data.wifiSignal + " dBm" : "--";
        })
        .catch(err => console.error("❌ Error fetching info:", err));
}
// Hàm update uptime display realtime
function updateUptimeDisplay() {
    if (baseUptime === 0) return;
    const elapsedSeconds = Math.floor((Date.now() - lastUptimeUpdate) / 1000);
    const currentUptime = baseUptime + elapsedSeconds;

    const uptimeEl = document.getElementById("systemUptime");
    if (uptimeEl) {
        uptimeEl.textContent = formatUptime(currentUptime);
    }
}
setInterval(updateUptimeDisplay, 1000);
// ==================== FLAME ALERT SYSTEM ====================
let flameAlertActive = false;
let alertCount = 0;
let alertHistory = [];
// Show fire notification
function showFireNotification(message = "🔥 FIRE DETECTED! Take immediate action!") {
    // Remove existing notification if any
    dismissFireNotification();

    // Create notification element
    const notification = document.createElement('div');
    notification.className = 'fire-notification';
    notification.id = 'fireNotification';
    notification.innerHTML = `
        <div class="fire-notification-icon">🔥</div>
        <div class="fire-notification-content">
            <div class="fire-notification-title">⚠️ FIRE ALERT!</div>
            <div class="fire-notification-message">${message}</div>
        </div>
        <button class="fire-notification-close" onclick="dismissFireNotification()">×</button>
    `;

    document.body.appendChild(notification);

    // Play alert sound (optional)
    playAlertSound();

    // Log alert
    logAlert();

    // Auto-dismiss after 10 seconds (optional)
    // setTimeout(dismissFireNotification, 10000);
}
// Dismiss fire notification
function dismissFireNotification() {
    const notification = document.getElementById('fireNotification');
    if (notification) {
        notification.style.animation = 'slideInRight 0.3s reverse';
        setTimeout(() => notification.remove(), 300);
    }
}
// Play alert sound (browser beep)
function playAlertSound() {
    try {
        // Create audio context for beep sound
        const audioContext = new (window.AudioContext || window.webkitAudioContext)();
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.connect(gainNode);
        gainNode.connect(audioContext.destination);

        oscillator.frequency.value = 800; // Frequency in Hz
        oscillator.type = 'sine';

        gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);

        oscillator.start(audioContext.currentTime);
        oscillator.stop(audioContext.currentTime + 0.5);
    } catch (e) {
        console.warn("Audio not supported:", e);
    }
}
// Log alert to history
function logAlert() {
    alertCount++;
    const timestamp = new Date().toLocaleString();
    alertHistory.push({
        id: alertCount,
        timestamp: timestamp,
        message: "Fire detected"
    });

    // Update alert counter badge (if exists)
    updateAlertBadge();

    console.log(`🔥 Alert #${alertCount} logged at ${timestamp}`);
}
// Update alert counter badge
function updateAlertBadge() {
    const alertsTodayEl = document.getElementById('alertsToday');
    if (alertsTodayEl) {
        alertsTodayEl.textContent = alertCount;
    }
}
let notifications = [];
let notificationIdCounter = 0;
function saveNotifications() {
    try {
        const data = {
            notifications: notifications.slice(0, 100), // Limit to 100
            counter: notificationIdCounter,
            timestamp: Date.now()
        };
        localStorage.setItem('iot_notifications', JSON.stringify(data));
        console.log(`💾 Saved ${notifications.length} notifications`);
    } catch (e) {
        console.warn('Failed to save notifications:', e);
        // If storage full, clear old data
        if (e.name === 'QuotaExceededError') {
            localStorage.removeItem('iot_notifications');
            console.log(' Cleared old notifications due to quota');
        }
    }
}
// Load notifications from localStorage
function loadNotifications() {
    try {
        const saved = localStorage.getItem('iot_notifications');
        if (!saved) {
            console.log('📭 No saved notifications found');
            return;
        }

        const data = JSON.parse(saved);

        // Check if data is not too old (7 days)
        const sevenDays = 7 * 24 * 60 * 60 * 1000;
        if (data.timestamp && (Date.now() - data.timestamp > sevenDays)) {
            console.log('Notifications too old, clearing...');
            localStorage.removeItem('iot_notifications');
            return;
        }

        // Restore notifications
        if (Array.isArray(data.notifications)) {
            // Convert timestamp strings back to Date objects
            notifications = data.notifications.map(n => ({
                ...n,
                timestamp: new Date(n.timestamp)
            }));
            notificationIdCounter = data.counter || 0;

            console.log(`📥 Loaded ${notifications.length} notifications`);

            // Update UI
            updateNotificationUI();
            updateNotificationBadge();
        }
    } catch (e) {
        console.warn('Failed to load notifications:', e);
        // Clear corrupted data
        localStorage.removeItem('iot_notifications');
    }
}
// Clear all stored notifications
function clearStoredNotifications() {
    localStorage.removeItem('iot_notifications');
    console.log('Cleared stored notifications');
}
// Toggle notification panel
function toggleNotificationPanel() {
    const panel = document.getElementById('notificationPanel');
    const isVisible = panel.style.display !== 'none';

    if (isVisible) {
        panel.style.animation = 'slideDown 0.2s reverse';
        setTimeout(() => {
            panel.style.display = 'none';
        }, 200);
    } else {
        panel.style.display = 'block';
        panel.style.animation = 'slideDown 0.3s ease-out';
    }
}
// Close panel when clicking outside
document.addEventListener('click', function (event) {
    const panel = document.getElementById('notificationPanel');
    const bell = document.querySelector('.notification-bell');

    if (panel && bell && !bell.contains(event.target) && !panel.contains(event.target)) {
        if (panel.style.display !== 'none') {
            toggleNotificationPanel();
        }
    }
});
// Add notification
function addNotification(type, title, message, icon = '🔔') {
    const notification = {
        id: ++notificationIdCounter,
        type: type,  // 'fire', 'wifi', 'info', 'warning'
        title: title,
        message: message,
        icon: icon,
        timestamp: new Date()
    };

    notifications.unshift(notification);  // Add to beginning

    // Limit to 50 notifications
    if (notifications.length > 50) {
        notifications = notifications.slice(0, 50);
    }

    updateNotificationUI();
    updateNotificationBadge();

    saveNotifications();
}
// Update notification badge
function updateNotificationBadge() {
    const badge = document.getElementById('notificationBadge');
    if (badge) {
        const count = notifications.length;
        if (count > 0) {
            badge.textContent = count > 99 ? '99+' : count;
            badge.style.display = 'flex';
        } else {
            badge.style.display = 'none';
        }
    }
}
// Update notification list UI
function updateNotificationUI() {
    const listEl = document.getElementById('notificationList');
    if (!listEl) return;

    if (notifications.length === 0) {
        listEl.innerHTML = '<div class="empty-notifications">No notifications yet</div>';
        return;
    }

    const html = notifications.map(notif => {
        const timeAgo = getTimeAgo(notif.timestamp);
        const typeClass = notif.type || 'info';

        return `
            <div class="notification-item ${typeClass}" onclick="dismissNotification(${notif.id})">
                <div class="notification-item-header">
                    <div class="notification-item-title">
                        <span>${notif.icon}</span>
                        <span>${notif.title}</span>
                    </div>
                    <div class="notification-item-time">${timeAgo}</div>
                </div>
                <div class="notification-item-message">${notif.message}</div>
            </div>
        `;
    }).join('');

    listEl.innerHTML = html;
}
// Get time ago string
function getTimeAgo(date) {
    const seconds = Math.floor((new Date() - date) / 1000);

    if (seconds < 60) return 'Just now';
    if (seconds < 3600) return `${Math.floor(seconds / 60)}m ago`;
    if (seconds < 86400) return `${Math.floor(seconds / 3600)}h ago`;
    return `${Math.floor(seconds / 86400)}d ago`;
}
// Dismiss single notification
function dismissNotification(id) {
    notifications = notifications.filter(n => n.id !== id);
    updateNotificationUI();
    updateNotificationBadge();
    saveNotifications();
}
// Clear all notifications
function clearAllNotifications() {
    if (notifications.length === 0) return;

    if (confirm('Clear all notifications?')) {
        notifications = [];
        updateNotificationUI();
        updateNotificationBadge();
    }
    clearStoredNotifications();
}
// Auto-refresh time ago every minute
setInterval(() => {
    if (notifications.length > 0) {
        updateNotificationUI();
    }
}, 60000);
window.addEventListener('DOMContentLoaded', function () {
    console.log('Dashboard loaded, restoring notifications...');
    loadNotifications();
});
window.addEventListener('beforeunload', function () {
    if (notifications.length > 0) {
        saveNotifications();
    }
});