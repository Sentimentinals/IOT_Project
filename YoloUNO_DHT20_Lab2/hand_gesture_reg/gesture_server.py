"""
Hand Gesture Recognition Server
Flask backend for streaming webcam and detecting hand gestures
Sends commands to ESP32 via MQTT through CoreIOT
"""

from flask import Flask, Response, jsonify, render_template_string
from flask_cors import CORS
import cv2
import mediapipe as mp
import numpy as np
import paho.mqtt.client as mqtt
import json
import threading
import time

app = Flask(__name__)
CORS(app)  # Allow cross-origin requests from ESP32 webserver

# =============================================================================
# CONFIGURATION
# =============================================================================
MQTT_BROKER = "app.coreiot.io"
MQTT_PORT = 1883
MQTT_TOKEN = "YwHdvCAyHicwQccYeIWD"  # Replace with your CoreIOT device access token

# =============================================================================
# GLOBAL STATE
# =============================================================================
camera = None
is_running = False
current_gesture = "NONE"
last_command = None
cooldown_frames = 0
mqtt_client = None
mqtt_connected = False

# =============================================================================
# MQTT SETUP
# =============================================================================
def on_mqtt_connect(client, userdata, flags, rc):
    global mqtt_connected
    if rc == 0:
        print("[MQTT] Connected to CoreIOT!")
        mqtt_connected = True
        client.subscribe('v1/devices/me/rpc/request/+')
    else:
        print(f"[MQTT] Connection failed with code {rc}")
        mqtt_connected = False

def on_mqtt_message(client, userdata, message):
    print(f"[MQTT] Received: {message.payload.decode('utf-8')}")

def init_mqtt():
    global mqtt_client
    mqtt_client = mqtt.Client()
    mqtt_client.username_pw_set(MQTT_TOKEN)
    mqtt_client.on_connect = on_mqtt_connect
    mqtt_client.on_message = on_mqtt_message
    
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT)
        mqtt_client.loop_start()
        print("[MQTT] Connecting...")
    except Exception as e:
        print(f"[MQTT] Error: {e}")

def send_command(cmd):
    global mqtt_client, mqtt_connected
    if mqtt_client and mqtt_connected:
        payload = {
            "method": "setValue",
            "cmd": cmd
        }
        mqtt_client.publish('v1/devices/me/telemetry', json.dumps(payload), 1)
        print(f"[MQTT] Sent: {payload}")

# =============================================================================
# MEDIAPIPE HAND DETECTION
# =============================================================================
mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

hands = mp_hands.Hands(
    model_complexity=1,
    max_num_hands=1,
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5
)

def is_fist(landmarks):
    """Check if hand is making a fist gesture"""
    tips = [8, 12, 16, 20]  # Index, Middle, Ring, Pinky tips
    pips = [6, 10, 14, 18]  # PIP joints
    
    closed_fingers = 0
    for tip, pip in zip(tips, pips):
        if landmarks[tip].y > landmarks[pip].y:
            closed_fingers += 1
    
    # Check thumb
    if landmarks[4].x > landmarks[3].x:  # Right hand
        closed_fingers += 1
    
    return closed_fingers >= 4

def is_open_palm(landmarks):
    """Check if hand is showing open palm"""
    tips = [8, 12, 16, 20]
    pips = [6, 10, 14, 18]
    
    open_fingers = 0
    for tip, pip in zip(tips, pips):
        if landmarks[tip].y < landmarks[pip].y:
            open_fingers += 1
    
    return open_fingers >= 4

def detect_gesture(landmarks):
    """Detect gesture from hand landmarks"""
    if is_fist(landmarks):
        return "FAN_OFF"
    elif is_open_palm(landmarks):
        return "FAN_ON"
    return "NONE"

# =============================================================================
# VIDEO STREAMING
# =============================================================================
def generate_frames():
    global camera, is_running, current_gesture, last_command, cooldown_frames
    
    while is_running:
        if camera is None or not camera.isOpened():
            time.sleep(0.1)
            continue
            
        success, frame = camera.read()
        if not success:
            continue
        
        # Flip frame horizontally for mirror effect
        frame = cv2.flip(frame, 1)
        
        # Convert to RGB for MediaPipe
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = hands.process(rgb)
        
        gesture = "NONE"
        
        if results.multi_hand_landmarks:
            hand = results.multi_hand_landmarks[0]
            gesture = detect_gesture(hand.landmark)
            
            # Draw hand landmarks
            mp_draw.draw_landmarks(frame, hand, mp_hands.HAND_CONNECTIONS)
        
        current_gesture = gesture
        
        # Send command if gesture changed and cooldown expired
        if gesture != "NONE" and gesture != last_command and cooldown_frames <= 0:
            send_command(gesture)
            last_command = gesture
            cooldown_frames = 15
        
        cooldown_frames = max(0, cooldown_frames - 1)
        
        # Draw gesture text on frame
        color = (0, 255, 0) if gesture != "NONE" else (128, 128, 128)
        cv2.putText(frame, f"Gesture: {gesture}", (10, 40),
                    cv2.FONT_HERSHEY_SIMPLEX, 1, color, 2)
        
        # Draw MQTT status
        mqtt_status = "MQTT: Connected" if mqtt_connected else "MQTT: Disconnected"
        mqtt_color = (0, 255, 0) if mqtt_connected else (0, 0, 255)
        cv2.putText(frame, mqtt_status, (10, 80),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, mqtt_color, 2)
        
        # Encode frame as JPEG
        ret, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
        frame_bytes = buffer.tobytes()
        
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')

# =============================================================================
# FLASK ROUTES
# =============================================================================
@app.route('/')
def index():
    """Home page with video preview"""
    return render_template_string('''
<!DOCTYPE html>
<html>
<head>
    <title>Hand Gesture Server</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            color: #fff;
            padding: 20px;
        }
        .container {
            max-width: 900px;
            margin: 0 auto;
        }
        h1 {
            text-align: center;
            margin-bottom: 20px;
            color: #00d9ff;
        }
        .video-container {
            background: #0f0f23;
            border-radius: 12px;
            padding: 15px;
            margin-bottom: 20px;
        }
        img {
            width: 100%;
            border-radius: 8px;
        }
        .controls {
            display: flex;
            gap: 10px;
            justify-content: center;
            margin-bottom: 20px;
        }
        button {
            padding: 12px 30px;
            font-size: 16px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s;
        }
        .start-btn {
            background: #00d9ff;
            color: #000;
        }
        .stop-btn {
            background: #ff4757;
            color: #fff;
        }
        button:hover {
            transform: scale(1.05);
        }
        .status {
            text-align: center;
            padding: 15px;
            background: #0f0f23;
            border-radius: 8px;
        }
        .gesture-display {
            font-size: 24px;
            font-weight: bold;
            color: #00d9ff;
        }
        .instructions {
            margin-top: 20px;
            padding: 15px;
            background: rgba(0,217,255,0.1);
            border-radius: 8px;
        }
        .instructions h3 { margin-bottom: 10px; }
        .instructions li { margin: 5px 0; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🖐️ Hand Gesture Control Server</h1>
        
        <div class="video-container">
            <img id="videoFeed" src="/video_feed" alt="Video Feed">
        </div>
        
        <div class="controls">
            <button class="start-btn" onclick="startCamera()">▶️ Start</button>
            <button class="stop-btn" onclick="stopCamera()">⏹️ Stop</button>
        </div>
        
        <div class="status">
            <p>Current Gesture: <span class="gesture-display" id="gestureText">--</span></p>
            <p>MQTT Status: <span id="mqttStatus">Checking...</span></p>
        </div>
        
        <div class="instructions">
            <h3>📖 Instructions</h3>
            <ul>
                <li>✋ <strong>Open Palm</strong> = FAN ON</li>
                <li>✊ <strong>Fist</strong> = FAN OFF</li>
            </ul>
        </div>
    </div>
    
    <script>
        function startCamera() {
            fetch('/start', {method: 'POST'})
                .then(r => r.json())
                .then(d => {
                    document.getElementById('videoFeed').src = '/video_feed?' + Date.now();
                });
        }
        
        function stopCamera() {
            fetch('/stop', {method: 'POST'})
                .then(r => r.json());
        }
        
        setInterval(() => {
            fetch('/status')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('gestureText').textContent = d.gesture;
                    document.getElementById('mqttStatus').textContent = d.mqtt_connected ? 'Connected ✅' : 'Disconnected ❌';
                });
        }, 500);
    </script>
</body>
</html>
    ''')

@app.route('/video_feed')
def video_feed():
    """MJPEG video stream endpoint"""
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/status')
def status():
    """Get current gesture status"""
    return jsonify({
        'gesture': current_gesture,
        'is_running': is_running,
        'mqtt_connected': mqtt_connected
    })

@app.route('/start', methods=['POST'])
def start_camera():
    """Start camera and gesture detection"""
    global camera, is_running
    
    if not is_running:
        camera = cv2.VideoCapture(0)
        camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        is_running = True
        print("[Camera] Started")
    
    return jsonify({'status': 'started'})

@app.route('/stop', methods=['POST'])
def stop_camera():
    """Stop camera and gesture detection"""
    global camera, is_running, current_gesture
    
    is_running = False
    current_gesture = "NONE"
    
    if camera:
        camera.release()
        camera = None
    
    print("[Camera] Stopped")
    return jsonify({'status': 'stopped'})

# =============================================================================
# MAIN
# =============================================================================
if __name__ == '__main__':
    print("=" * 50)
    print("Hand Gesture Recognition Server")
    print("=" * 50)
    
    # Initialize MQTT
    init_mqtt()
    
    # Start camera by default
    camera = cv2.VideoCapture(0)
    camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    is_running = True
    
    print("\n[Server] Starting on http://0.0.0.0:5000")
    print("[Server] Video feed: http://<your-ip>:5000/video_feed")
    print("\nGestures:")
    print("  ✋ Open Palm = FAN ON")
    print("  ✊ Fist = FAN OFF")
    print("\nPress Ctrl+C to stop\n")
    
    app.run(host='0.0.0.0', port=5000, threaded=True)

