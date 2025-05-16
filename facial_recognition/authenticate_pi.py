#!/usr/bin/env python3
import os
import glob
import cv2
import torch
import pickle
import numpy as np
from PIL import Image
import face_recognition

# ===== Configuration (edit if needed) =====
FRAME_WIDTH     = 640                   # scale down for speed
FRAME_HEIGHT    = 480
TS_MODEL_FILE   = "fnet_ts.pt"          # TorchScript FaceNet
SVM_MODEL_FILE  = "face_recognizer_svm.pkl"
THRESHOLD       = 0.6                   # min probability to accept
# ==========================================

def find_camera_source():
    """
    Try to open every /dev/video* device first.
    Then fall back to numeric indices 0..9.
    Returns the string or int you can pass to cv2.VideoCapture.
    """
    # 1) try all /dev/video* paths
    for dev in sorted(glob.glob('/dev/video*')):
        cap = cv2.VideoCapture(dev)
        if cap.isOpened():
            ret, _ = cap.read()
            cap.release()
            if ret:
                print(f"→ Using video device {dev}")
                return dev

    # 2) fall back to numeric indices
    for idx in range(10):
        cap = cv2.VideoCapture(idx)
        if cap.isOpened():
            ret, _ = cap.read()
            cap.release()
            if ret:
                print(f"→ Using camera index {idx}")
                return idx

    raise RuntimeError("No usable camera device found (/dev/video* or index 0-9)")

def load_models():
    # 1) FaceNet TorchScript
    fnet = torch.jit.load(TS_MODEL_FILE, map_location="cpu")
    fnet.eval()
    # 2) SVM classifier
    svm = pickle.load(open(SVM_MODEL_FILE, "rb"))
    return fnet, svm

def get_embedding(fnet, bgr_crop):
    # BGR→RGB, resize to 160×160, normalize, embed
    rgb = cv2.cvtColor(bgr_crop, cv2.COLOR_BGR2RGB)
    img = Image.fromarray(rgb).resize((160, 160))
    arr = np.asarray(img).transpose(2, 0, 1)[None] / 255.0
    tensor = torch.tensor(arr, dtype=torch.float32)
    with torch.no_grad():
        emb = fnet(tensor).cpu().numpy()[0]
    return emb

def main():
    # Find which cam to use
    source = find_camera_source()
    cap = cv2.VideoCapture(source)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH , FRAME_WIDTH)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)
    torch.set_num_threads(4)

    fnet, svm = load_models()
    print("Press 'q' to quit.")
    while True:
        ret, frame = cap.read()
        if not ret:
            break

        # detect faces (HOG)
        rgb = frame[:, :, ::-1]
        locs = face_recognition.face_locations(rgb, model="hog")

        if locs:
            top, right, bottom, left = locs[0]
            crop = frame[top:bottom, left:right]
            emb = get_embedding(fnet, crop)

            probs = svm.predict_proba([emb])[0]
            idx = np.argmax(probs)
            prob = probs[idx]
            name = svm.classes_[idx]

            if prob >= THRESHOLD:
                label, color = f"Authorized: {name}", (0,255,0)
                print(f"{label} ({prob:.2f})")
            else:
                label, color = "Not authorized", (0,0,255)
                print(f"{label} ({prob:.2f})")

            cv2.rectangle(frame, (left, top), (right, bottom), color, 2)
            cv2.putText(frame, label, (left, top-10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, color, 2)
        else:
            cv2.putText(frame, "No face detected", (10,30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0,255,255), 2)

        cv2.imshow("Authenticator", frame)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
