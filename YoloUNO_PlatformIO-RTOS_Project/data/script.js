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
    loadRelays();      // Load saved relays from localStorage
    renderRelays();    // Render relay cards
    loadNotifications();
}

function onOpen(event) {
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
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function Send_Data(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(data);
    } else {
        alert("⚠️ WebSocket not connected!");
    }
}

// Track state alerts to avoid duplicates
let lastStateAlert = 'normal';
let warningAlertActive = false;
let criticalAlertActive = false;

function onMessage(event) {
    try {
        var data = JSON.parse(event.data);
        
        // Handle OTA update notification
        if (data.type === "ota") {
            if (data.status === "starting") {
                addNotification(
                    'info',
                    '🔄 OTA Update',
                    'Firmware/Filesystem update starting. Device will reboot...'
                );
                // Show alert to user
                alert("⚠️ OTA Update starting!\n\nDevice will reboot. Please wait and refresh the page after ~10 seconds.");
            }
            return;
        }
        
        // Handle state alerts from ESP32
        if (data.type === "state_alert") {
            handleStateAlert(data);
            return;
        }
        
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
                if (lightEl) lightEl.innerHTML = Math.round(data.light);
            }
            if (data.moisture !== undefined) {
                const moistureEl = document.getElementById("moisture");
                if (moistureEl) moistureEl.innerHTML = parseFloat(data.moisture).toFixed(1);
            }
            // Water pump status update
            if (data.water_pump !== undefined) {
                updatePumpButton(data.water_pump, data.pump_auto);
            }
            // Fan status update (if sent from server)
            if (data.fan !== undefined) {
                updateFanButton(data.fan);
            }
            if (data.flame !== undefined) {
                const flameEl = document.getElementById("flame");
                const flameCard = document.getElementById('flameCard');
                if (flameEl) {
                    if (data.flame === true) {
                        flameEl.innerHTML = '<span class="flame-icon">🔥</span><span class="flame-text">FIRE!</span>';

                        if (flameCard) {
                            flameCard.classList.add('flame-alert');
                            flameCard.classList.remove('flame-safe');
                        }

                        document.body.classList.add('emergency-alert');

                        if (!flameAlertActive) {
                            flameAlertActive = true;
                            addNotification(
                                'fire',
                                '🔥 FIRE ALERT!',
                                'Fire detected! Check flame sensor immediately!'
                            );
                            showFireNotification("🔥 FIRE DETECTED! Check flame sensor immediately!");
                        }
                    } else {
                        flameEl.innerHTML = '<span class="flame-icon">✓</span><span class="flame-text">Safe</span>';

                        if (flameCard) {
                            flameCard.classList.add('flame-safe');
                            flameCard.classList.remove('flame-alert');
                        }

                        document.body.classList.remove('emergency-alert');

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
                        '📶 WiFi Connected',
                        `Connected to ${data.ssid || 'network'}. IP: ${data.ip || '--'}`
                    );
                } else if (data.wifiStatus === 'failed') {
                    addNotification(
                        'wifi',
                        '❌ WiFi Failed',
                        'Failed to connect to WiFi. AP mode still active at 192.168.4.1'
                    );
                } else if (data.wifiStatus === 'connecting') {
                    addNotification(
                        'wifi',
                        '🔄 WiFi Connecting',
                        `Attempting to connect to ${data.ssid || 'network'}...`
                    );
                } else if (data.wifiStatus === 'ap_mode') {
                    addNotification(
                        'info',
                        '📶 Access Point Mode',
                        `Device running in AP mode. IP: ${data.ip || '192.168.4.1'}`
                    );
                }
            }

            // Update environment status badges
            if (data.temperature !== undefined && data.humidity !== undefined) {
                updateEnvironmentBadges(data.temperature, data.humidity);
            }
        }


    } catch (e) {
        // Invalid JSON
    }
}


// ==================== UI NAVIGATION ====================
// Relay list for custom devices (Fan & Pump now have dedicated cards)
let relayList = [];
let deleteTarget = null;

// Load relays from localStorage on startup
function loadRelays() {
    try {
        const saved = localStorage.getItem('iot_relays');
        if (saved) {
            relayList = JSON.parse(saved);
        }
    } catch (e) {
        relayList = [];
    }
}

// Save relays to localStorage
function saveRelaysToStorage() {
    try {
        localStorage.setItem('iot_relays', JSON.stringify(relayList));
    } catch (e) {
        console.log('Could not save relays');
    }
}

function showSection(id, event) {
    // Hide all sections including home
    document.querySelectorAll('.section').forEach(sec => {
        sec.style.display = 'none';
    });
    
    // Show target section
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
    // Clear previous inputs
    document.getElementById('relayName').value = '';
    document.getElementById('relayGPIO').value = '';
}

function closeAddRelayDialog() {
    document.getElementById('addRelayDialog').style.display = 'none';
}

function saveRelay() {
    const name = document.getElementById('relayName').value.trim();
    const gpio = document.getElementById('relayGPIO').value.trim();
    if (!name || !gpio) return alert("Please fill all fields!");

    relayList.push({ id: Date.now(), name, gpio: parseInt(gpio), state: false });
    saveRelaysToStorage();
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
    if (!container) return;
    
    if (relayList.length === 0) {
        container.innerHTML = '<p style="color: var(--text-muted); text-align: center; padding: 20px;">No custom relays. Click + to add one.</p>';
        return;
    }
    
    container.innerHTML = "";
    relayList.forEach(r => {
        const card = document.createElement('div');
        card.className = 'device-card';
        card.innerHTML = `
            <div class="device-icon">⚡</div>
            <h3>${r.name}</h3>
            <p>GPIO: ${r.gpio}</p>
            <button class="toggle-btn ${r.state ? 'on' : 'off'}" onclick="toggleRelay(${r.id})">
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
        saveRelaysToStorage();
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
    saveRelaysToStorage();
    renderRelays();
    closeConfirmDelete();
}


// ==================== NEO LED CONTROL ====================
let neoLedState = true; // Mặc định BẬT

function toggleNeoLED() {
    neoLedState = !neoLedState;
    updateNeoLedButton(neoLedState);

    // Gửi lệnh qua WebSocket
    const ledJSON = JSON.stringify({
        page: "neoled",
        value: {
            enabled: neoLedState
        }
    });

    Send_Data(ledJSON);
}

// Update NeoLED button state
function updateNeoLedButton(state) {
    neoLedState = state;
    const btn = document.getElementById("neoLedBtn");
    const statusText = document.getElementById("neoLedStatusText");
    
    if (btn) {
        if (state) {
            btn.classList.add("on");
            btn.classList.remove("off");
            btn.innerHTML = '<span class="btn-icon">⏻</span><span class="btn-text">ON</span>';
        } else {
            btn.classList.remove("on");
            btn.classList.add("off");
            btn.innerHTML = '<span class="btn-icon">⏻</span><span class="btn-text">OFF</span>';
        }
    }
    
    if (statusText) {
        statusText.textContent = state ? "ON" : "OFF";
        statusText.style.color = state ? "#10B981" : "var(--text-muted)";
    }
}


// ==================== FAN CONTROL ====================
let fanState = false; // Mặc định TẮT

function toggleFan() {
    fanState = !fanState;
    updateFanButton(fanState);

    // Gửi lệnh qua WebSocket
    const fanJSON = JSON.stringify({
        page: "fan_control",
        value: {
            enabled: fanState
        }
    });

    Send_Data(fanJSON);
}

// Update fan button state from server or toggle
function updateFanButton(state) {
    fanState = state;
    const btn = document.getElementById("fanBtn");
    const statusText = document.getElementById("fanStatusText");
    
    if (btn) {
        if (state) {
            btn.classList.add("on");
            btn.classList.remove("off");
            btn.innerHTML = '<span class="btn-icon">⏻</span><span class="btn-text">ON</span>';
        } else {
            btn.classList.remove("on");
            btn.classList.add("off");
            btn.innerHTML = '<span class="btn-icon">⏻</span><span class="btn-text">OFF</span>';
        }
    }
    
    if (statusText) {
        statusText.textContent = state ? "ON" : "OFF";
        statusText.style.color = state ? "#10B981" : "var(--text-muted)";
    }
}


// ==================== WATER PUMP CONTROL ====================
let pumpState = false; // Mặc định TẮT
let pumpAutoMode = true; // Mặc định AUTO

function togglePump() {
    pumpState = !pumpState;
    updatePumpButton(pumpState, false); // Manual mode when toggled

    // Gửi lệnh qua WebSocket
    const pumpJSON = JSON.stringify({
        page: "pump_control",
        value: {
            enabled: pumpState
        }
    });

    Send_Data(pumpJSON);
}

// Update pump button state from server
function updatePumpButton(state, autoMode) {
    pumpState = state;
    if (autoMode !== undefined) {
        pumpAutoMode = autoMode;
    }
    
    const btn = document.getElementById("pumpBtn");
    const statusText = document.getElementById("pumpStatusText");
    const modeBadge = document.getElementById("pumpModeBadge");
    
    if (btn) {
        if (state) {
            btn.classList.add("on");
            btn.classList.remove("off");
            btn.innerHTML = '<span class="btn-icon">⏻</span><span class="btn-text">ON</span>';
        } else {
            btn.classList.remove("on");
            btn.classList.add("off");
            btn.innerHTML = '<span class="btn-icon">⏻</span><span class="btn-text">OFF</span>';
        }
    }
    
    if (statusText) {
        statusText.textContent = state ? "ON" : "OFF";
        statusText.style.color = state ? "#10B981" : "var(--text-muted)";
    }
    
    if (modeBadge) {
        if (pumpAutoMode) {
            modeBadge.textContent = "Auto";
            modeBadge.classList.add("auto");
        } else {
            modeBadge.textContent = "Manual";
            modeBadge.classList.remove("auto");
        }
    }
}


// ==================== CSV CONTROLS ====================
function downloadCSV() {
    window.location.href = "/download";
}

function clearCSV() {
    if (confirm("Are you sure you want to delete all sensor data? This action cannot be undone.")) {
        fetch("/clear")
            .then(response => response.text())
            .then(data => {
                alert("✅ " + data);
                updateCSVInfo();
            })
            .catch(err => {
                alert("❌ Connection error!");
            });
    }
}

function formatFileSize(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
}

function updateCSVInfo() {
    fetch("/csv-info")
        .then(response => response.json())
        .then(data => {
            const statusEl = document.getElementById("csvStatus");
            const recordsEl = document.getElementById("csvRecords");
            const sizeEl = document.getElementById("csvSize");
            
            if (data.exists && data.lines > 1) {
                const records = data.lines - 1;  // Subtract header row
                
                if (recordsEl) recordsEl.textContent = records.toLocaleString();
                if (sizeEl) sizeEl.textContent = formatFileSize(data.size);
                if (statusEl) statusEl.textContent = "Ready";
                if (statusEl) statusEl.style.color = "#10B981";
            } else {
                if (recordsEl) recordsEl.textContent = "0";
                if (sizeEl) sizeEl.textContent = "0 B";
                if (statusEl) statusEl.textContent = "No data";
                if (statusEl) statusEl.style.color = "#F59E0B";
            }
        })
        .catch(err => {
            const statusEl = document.getElementById("csvStatus");
            if (statusEl) {
                statusEl.textContent = "Offline";
                statusEl.style.color = "#EF4444";
            }
        });
}

setInterval(updateCSVInfo, 10000);
setTimeout(updateCSVInfo, 2000);

// ==================== WIFI SETTINGS FORM ====================
document.getElementById("wifiForm").addEventListener("submit", function (e) {
    e.preventDefault();
    const ssid = document.getElementById("ssid").value.trim();
    const password = document.getElementById("password").value.trim();

    if (!ssid) {
        alert("⚠️ Please enter a WiFi name (SSID)!");
        return;
    }

    const wifiJSON = JSON.stringify({
        page: "wifi_setting",
        value: {
            ssid: ssid,
            password: password
        }
    });
    Send_Data(wifiJSON);

    addNotification(
        'wifi',
        '📡 WiFi Saved',
        `Connecting to: ${ssid}...`
    );
    alert("✅ WiFi configuration sent!\n\nDevice will reboot and reconnect to: " + ssid);
});

// ==================== COREIOT SETTINGS FORM ====================
document.getElementById("coreiotForm").addEventListener("submit", function (e) {
    e.preventDefault();
    const token = document.getElementById("token").value.trim();
    const server = document.getElementById("server").value.trim() || "app.coreiot.io";
    const port = document.getElementById("port").value.trim() || "1883";

    if (!token) {
        alert("⚠️ Please enter a Device Access Token!");
        return;
    }

    const coreiotJSON = JSON.stringify({
        page: "coreiot_setting",
        value: {
            token: token,
            server: server,
            port: port
        }
    });
    Send_Data(coreiotJSON);

    addNotification(
        'info',
        '☁️ CoreIOT Saved',
        `Server: ${server}:${port}`
    );
    alert("✅ CoreIOT configuration sent!\n\nDevice will reboot.");
});


// ==================== INFO SECTION ====================
function updateInfo() {
    fetch("/info")
        .then(response => response.json())
        .then(data => {
            document.getElementById("chipModel").textContent = data.chipModel || "Yolo UNO";
            document.getElementById("freeRam").textContent = (data.freeRam / 1024).toFixed(1) + " KB";
            document.getElementById("systemUptime").textContent = formatUptime(data.uptime);

            document.getElementById("wifiSSID").textContent = data.wifiSSID || "--";
            document.getElementById("ipAddress").textContent = data.ipAddress || "--";
            document.getElementById("wifiSignal").textContent = data.wifiSignal ? data.wifiSignal + " dBm" : "--";
        })
        .catch(err => {});
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
            document.getElementById("freeRam").textContent = (data.freeRam / 1024).toFixed(1) + " KB";

            baseUptime = data.uptime || 0;
            lastUptimeUpdate = Date.now();
            updateUptimeDisplay();
            
            document.getElementById("wifiSSID").textContent = data.wifiSSID || "--";
            document.getElementById("ipAddress").textContent = data.ipAddress || "--";
            document.getElementById("wifiSignal").textContent = data.wifiSignal ? data.wifiSignal + " dBm" : "--";
        })
        .catch(err => {});
}

// Update uptime display realtime
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

/**
 * Show fire notification toast
 */
function showFireNotification(message = "🔥 FIRE DETECTED! Take immediate action!") {
    dismissFireNotification();

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
    playAlertSound();
}

/**
 * Handle state alerts from ESP32 (WARNING, CRITICAL, FIRE)
 */
function handleStateAlert(data) {
    const level = data.level;
    const reason = data.reason || 'Unknown';
    const temp = data.temperature;
    const hum = data.humidity;
    
    // Validate sensor data - skip if invalid
    if (temp === undefined || temp === null || temp <= 0 || temp >= 80 ||
        hum === undefined || hum === null || hum <= 0 || hum > 100) {
        console.log('Invalid sensor data, skipping alert:', data);
        return;
    }
    
    const tempStr = temp.toFixed(1);
    const humStr = hum.toFixed(1);
    
    // Avoid duplicate alerts for same level
    if (level === lastStateAlert) return;
    
    console.log('State alert:', level, reason, tempStr, humStr);
    
    if (level === 'warning') {
        lastStateAlert = level;
        warningAlertActive = true;
        criticalAlertActive = false;
        
        addNotification(
            'warning',
            '⚠️ WARNING: ' + reason,
            `🌡️ Temp: ${tempStr}°C | 💧 Humidity: ${humStr}%`
        );
        
        // Visual indicator removed as per user request
    }
    else if (level === 'critical') {
        lastStateAlert = level;
        criticalAlertActive = true;
        warningAlertActive = false;
        
        addNotification(
            'warning',
            '🚨 CRITICAL: ' + reason,
            `🌡️ Temp: ${tempStr}°C | 💧 Humidity: ${humStr}% - Action required!`
        );
        
        // Play alert sound for critical
        playAlertSound();
    }
    else if (level === 'fire') {
        // Fire is handled by flame sensor data
    }
    else if (level === 'normal') {
        // Only notify if we were in warning/critical state before
        if (warningAlertActive || criticalAlertActive) {
            addNotification(
                'info',
                '✅ Status Normal',
                'All readings returned to normal range'
            );
        }
        
        // Clear all state alerts
        lastStateAlert = level;
        warningAlertActive = false;
        criticalAlertActive = false;
    }
}

// Dismiss fire notification toast
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
        const audioContext = new (window.AudioContext || window.webkitAudioContext)();
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.connect(gainNode);
        gainNode.connect(audioContext.destination);

        oscillator.frequency.value = 800;
        oscillator.type = 'sine';

        gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);

        oscillator.start(audioContext.currentTime);
        oscillator.stop(audioContext.currentTime + 0.5);
    } catch (e) {
        // Audio not supported
    }
}

/**
 * Update alert counter badge
 */
function updateAlertBadge() {
    const alertsTodayEl = document.getElementById('alertsToday');
    if (alertsTodayEl) {
        const fireAlerts = notifications.filter(n => n.type === 'fire').length;
        alertsTodayEl.textContent = fireAlerts;
    }
}

let notifications = [];
let notificationIdCounter = 0;

function saveNotifications() {
    try {
        const data = {
            notifications: notifications.slice(0, 100),
            counter: notificationIdCounter,
            timestamp: Date.now()
        };
        localStorage.setItem('iot_notifications', JSON.stringify(data));
    } catch (e) {
        if (e.name === 'QuotaExceededError') {
            localStorage.removeItem('iot_notifications');
        }
    }
}

function loadNotifications() {
    try {
        const saved = localStorage.getItem('iot_notifications');
        if (!saved) return;

        const data = JSON.parse(saved);

        const sevenDays = 7 * 24 * 60 * 60 * 1000;
        if (data.timestamp && (Date.now() - data.timestamp > sevenDays)) {
            localStorage.removeItem('iot_notifications');
            return;
        }

        if (Array.isArray(data.notifications)) {
            notifications = data.notifications.map(n => ({
                ...n,
                timestamp: new Date(n.timestamp)
            }));
            notificationIdCounter = data.counter || 0;

            updateNotificationUI();
            updateNotificationBadge();
            updateAlertBadge();
        }
    } catch (e) {
        localStorage.removeItem('iot_notifications');
    }
}

function clearStoredNotifications() {
    localStorage.removeItem('iot_notifications');
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
    
    // Update alert badge for fire notifications (syncs overview card)
    if (type === 'fire') {
        updateAlertBadge();
    }

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
    loadNotifications();
});

window.addEventListener('beforeunload', function () {
    if (notifications.length > 0) {
        saveNotifications();
    }
});