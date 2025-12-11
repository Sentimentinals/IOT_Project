from flask import Flask, Response, jsonify, render_template_string
from flask_cors import CORS
import cv2
import mediapipe as mp
import paho.mqtt.client as mqtt
import json
import time

app = Flask(__name__)
CORS(app)

# Configuration
MQTT_BROKER = "app.coreiot.io"
MQTT_PORT = 1883
MQTT_TOKEN = "YwHdvCAyHicwQccYeIWD"
ESP32_IP = "192.168.50.193"

# State
camera = None
is_running = False
current_gesture = "NONE"
last_command = None
cooldown_frames = 0
mqtt_client = None
mqtt_connected = False

# MQTT
def on_mqtt_connect(client, userdata, flags, rc):
    global mqtt_connected
    mqtt_connected = (rc == 0)
    if rc == 0:
        print("[MQTT] Connected")

def init_mqtt():
    global mqtt_client
    mqtt_client = mqtt.Client()
    mqtt_client.username_pw_set(MQTT_TOKEN)
    mqtt_client.on_connect = on_mqtt_connect
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT)
        mqtt_client.loop_start()
    except:
        pass

def send_command(cmd):
    global mqtt_client, mqtt_connected
    if cmd not in ["FAN_ON", "FAN_OFF"]:
        return
    
    fan_state = (cmd == "FAN_ON")
    
    # 1. WebSocket -> ESP32 (fast, direct control)
    try:
        from websocket import create_connection
        ws = create_connection(f"ws://{ESP32_IP}/ws", timeout=2)
        ws.send(json.dumps({"page": "fan_control", "value": {"enabled": fan_state}}))
        ws.close()
        print(f"[WS] {cmd}")
    except:
        print(f"[WS] Failed")
    
    # 2. MQTT -> CoreIOT (log + Rule Chain)
    if mqtt_client and mqtt_connected:
        telemetry = {"gesture": cmd, "fanEnabled": fan_state}
        mqtt_client.publish('v1/devices/me/telemetry', json.dumps(telemetry), 1)
        print(f"[MQTT] {cmd}")

# MediaPipe
mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils
hands = mp_hands.Hands(model_complexity=1, max_num_hands=1,
    min_detection_confidence=0.5, min_tracking_confidence=0.5)

def is_fist(lm):
    tips = [8, 12, 16, 20]
    pips = [6, 10, 14, 18]
    closed = sum(1 for t, p in zip(tips, pips) if lm[t].y > lm[p].y)
    if lm[4].x > lm[3].x: closed += 1
    return closed >= 4

def is_open_palm(lm):
    tips = [8, 12, 16, 20]
    pips = [6, 10, 14, 18]
    return sum(1 for t, p in zip(tips, pips) if lm[t].y < lm[p].y) >= 4

def detect_gesture(lm):
    if is_fist(lm): return "FAN_OFF"
    if is_open_palm(lm): return "FAN_ON"
    return "NONE"

# Video Stream
def generate_frames():
    global camera, is_running, current_gesture, last_command, cooldown_frames
    
    while is_running:
        if camera is None or not camera.isOpened():
            time.sleep(0.1)
            continue
            
        success, frame = camera.read()
        if not success: continue
        
        frame = cv2.flip(frame, 1)
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = hands.process(rgb)
        
        gesture = "NONE"
        if results.multi_hand_landmarks:
            hand = results.multi_hand_landmarks[0]
            gesture = detect_gesture(hand.landmark)
            mp_draw.draw_landmarks(frame, hand, mp_hands.HAND_CONNECTIONS)
        
        current_gesture = gesture
        
        if gesture != "NONE" and gesture != last_command and cooldown_frames <= 0:
            send_command(gesture)
            last_command = gesture
            cooldown_frames = 15
        
        cooldown_frames = max(0, cooldown_frames - 1)
        
        color = (0, 255, 0) if gesture != "NONE" else (128, 128, 128)
        cv2.putText(frame, f"{gesture}", (10, 40), cv2.FONT_HERSHEY_SIMPLEX, 1, color, 2)
        
        ret, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
        yield (b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')

# Routes
@app.route('/')
def index():
    return render_template_string('''
<!DOCTYPE html>
<html>
<head>
    <title>Gesture Server</title>
    <style>
        *{margin:0;padding:0;box-sizing:border-box}
        body{font-family:sans-serif;background:#1a1a2e;min-height:100vh;color:#fff;padding:20px}
        .container{max-width:800px;margin:0 auto}
        h1{text-align:center;margin-bottom:20px;color:#00d9ff}
        .video{background:#0f0f23;border-radius:12px;padding:10px;margin-bottom:15px}
        img{width:100%;border-radius:8px}
        .controls{display:flex;gap:10px;justify-content:center;margin-bottom:15px}
        button{padding:10px 25px;font-size:14px;border:none;border-radius:8px;cursor:pointer}
        .start{background:#00d9ff;color:#000}
        .stop{background:#ff4757;color:#fff}
        .status{text-align:center;padding:10px;background:#0f0f23;border-radius:8px}
        .gesture{font-size:20px;font-weight:bold;color:#00d9ff}
    </style>
</head>
<body>
    <div class="container">
        <h1>Hand Gesture Server</h1>
        <div class="video"><img id="feed" src="/video_feed"></div>
        <div class="controls">
            <button class="start" onclick="fetch('/start',{method:'POST'}).then(()=>document.getElementById('feed').src='/video_feed?'+Date.now())">Start</button>
            <button class="stop" onclick="fetch('/stop',{method:'POST'})">Stop</button>
        </div>
        <div class="status">
            Gesture: <span class="gesture" id="g">--</span> | 
            MQTT: <span id="m">--</span>
        </div>
    </div>
    <script>
        setInterval(()=>fetch('/status').then(r=>r.json()).then(d=>{
            document.getElementById('g').textContent=d.gesture;
            document.getElementById('m').textContent=d.mqtt_connected?'OK':'--';
        }),500);
    </script>
</body>
</html>''')

@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/status')
def status():
    return jsonify({'gesture': current_gesture, 'is_running': is_running, 'mqtt_connected': mqtt_connected})

@app.route('/start', methods=['POST'])
def start_camera():
    global camera, is_running
    if not is_running:
        camera = cv2.VideoCapture(0)
        camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        is_running = True
    return jsonify({'status': 'started'})

@app.route('/stop', methods=['POST'])
def stop_camera():
    global camera, is_running, current_gesture
    is_running = False
    current_gesture = "NONE"
    if camera:
        camera.release()
        camera = None
    return jsonify({'status': 'stopped'})

if __name__ == '__main__':
    print("Hand Gesture Server")
    print(f"ESP32: {ESP32_IP}")
    init_mqtt()
    camera = cv2.VideoCapture(0)
    camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    is_running = True
    app.run(host='0.0.0.0', port=5000, threaded=True)
