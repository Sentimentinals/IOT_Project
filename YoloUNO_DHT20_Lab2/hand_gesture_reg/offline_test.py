import cv2
import mediapipe as mp
import numpy as np

mp_hands = mp.solutions.hands
mp_drawing = mp.solutions.drawing_utils

hands = mp_hands.Hands(
    model_complexity=1,
    max_num_hands=1,
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5
)

cap = cv2.VideoCapture(0)

def is_fist(landmarks):
    tips = [4, 8, 12, 16, 20]
    palm = landmarks[0]
    distances = [
        np.linalg.norm(
            np.array([landmarks[t].x, landmarks[t].y]) -
            np.array([palm.x, palm.y])
        ) for t in tips
    ]
    return np.mean(distances) < 0.12

def is_open_palm(landmarks):
    tips = [8, 12, 16, 20]
    palm = landmarks[0]
    distances = [
        np.linalg.norm(
            np.array([landmarks[t].x, landmarks[t].y]) -
            np.array([palm.x, palm.y])
        ) for t in tips
    ]
    return np.mean(distances) > 0.25


while True:
    ret, frame = cap.read()
    if not ret:
        break

    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands.process(rgb)

    gesture = "none"

    if results.multi_hand_landmarks:
        hand = results.multi_hand_landmarks[0]

        if is_fist(hand.landmark):
            gesture = "FIST (LED OFF)"
        elif is_open_palm(hand.landmark):
            gesture = "OPEN PALM (LED ON)"

        mp_drawing.draw_landmarks(frame, hand, mp_hands.HAND_CONNECTIONS)

    cv2.putText(frame, f"{gesture}", (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 1, (50, 255, 50), 2)

    cv2.imshow("Hand Test", frame)

    if cv2.waitKey(1) & 0xFF == 27:  # ESC
        break

cap.release()
cv2.destroyAllWindows()
