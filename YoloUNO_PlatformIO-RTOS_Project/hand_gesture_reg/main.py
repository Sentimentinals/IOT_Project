import utils.py
import cv2
import mediapipe as mp
import numpy as np
import paho.mqtt.client as mqttclient
import json
import time

# -----------------------------
# MQTT Setup
# -----------------------------
BROKER_ADDRESS = "app.coreiot.io"
PORT = 1883
ACCESS_TOKEN = "YwHdvCAyHicwQccYeIWD"   # <-- Replace with your CoreIoT device access token

def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed...")

def recv_message(client, userdata, message):
    print("Received:", message.payload.decode("utf-8"))

def connected(client, usedata, flags, rc):
    if rc == 0:
        print("Connected successfully to CoreIoT!")
        client.subscribe('v1/devices/me/rpc/request/+')
    else:
        print("Connection failed")

client = mqttclient.Client()
client.username_pw_set(ACCESS_TOKEN)
client.on_connect = connected
client.on_subscribe = subscribed
client.on_message = recv_message

client.connect(BROKER_ADDRESS, PORT)
client.loop_start()


# -----------------------------
# MediaPipe Hand Detection Setup
# -----------------------------
mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

hands = mp_hands.Hands(
    model_complexity=1,
    max_num_hands=1,
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5
)


# -----------------------------
# Camera Loop
# -----------------------------
cap = cv2.VideoCapture(0)

last_cmd = None      # prevent spamming server
cooldown = 0         # delay between sends

while True:
    ret, frame = cap.read()
    if not ret:
        break

    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands.process(rgb)

    cmd = "NONE"

    if results.multi_hand_landmarks:
        hand = results.multi_hand_landmarks[0]

        if is_fist(hand.landmark):
            cmd = "FAN_OFF"
        elif is_open_palm(hand.landmark):
            cmd = "FAN_ON"

        mp_draw.draw_landmarks(frame, hand, mp_hands.HAND_CONNECTIONS)

    # -------------------------
    # Send to CoreIoT only when changed
    # -------------------------
    if cmd != "NONE" and cmd != last_cmd and cooldown <= 0:
        payload = {
            "method": "setValue",    
            "cmd": cmd               # FAN_ON, FAN_OFF
        }

        client.publish('v1/devices/me/telemetry', json.dumps(payload), 1)
        print("Sent:", payload)

        last_cmd = cmd
        cooldown = 15               # frames to wait before next send

    cooldown -= 1

    cv2.putText(frame, f"Gesture: {cmd}", (10, 40),
                cv2.FONT_HERSHEY_SIMPLEX, 1, (0,255,0), 2)

    cv2.imshow("Gesture Control", frame)

    if cv2.waitKey(1) & 0xFF == 27:  # ESC to quit
        break

cap.release()
cv2.destroyAllWindows()
client.loop_stop()
