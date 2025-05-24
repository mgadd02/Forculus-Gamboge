#!/usr/bin/env python3
import os
import pickle

import torch
import numpy as np
from PIL import Image
from sklearn.metrics import classification_report, confusion_matrix

# ===== Config =====
DATA_DIR        = "./dataset"               # each subfolder is a person, containing face crops
TS_MODEL_FILE   = "fnet_ts.pt"              # from export_fnet_ts.py
SVM_MODEL_FILE  = "face_recognizer_svm.pkl" # from train_classifier.py
# ==================

def load_models():
    # Load TorchScript FaceNet
    fnet = torch.jit.load(TS_MODEL_FILE, map_location="cpu")
    fnet.eval()
    # Load trained SVM
    svm = pickle.load(open(SVM_MODEL_FILE, "rb"))
    return fnet, svm

def get_embedding(fnet, pil_img):
    # Ensure RGB + correct size
    img = pil_img.convert("RGB").resize((160, 160))
    arr = np.asarray(img).transpose(2, 0, 1)[None] / 255.0
    tensor = torch.tensor(arr, dtype=torch.float32)
    with torch.no_grad():
        emb = fnet(tensor).cpu().numpy()[0]
    return emb

def main():
    fnet, svm = load_models()
    y_true, y_pred = [], []

    # Iterate each person's folder
    for person in sorted(os.listdir(DATA_DIR)):
        person_dir = os.path.join(DATA_DIR, person)
        if not os.path.isdir(person_dir):
            continue

        for img_name in os.listdir(person_dir):
            img_path = os.path.join(person_dir, img_name)
            try:
                pil_img = Image.open(img_path)
            except Exception:
                continue

            emb = get_embedding(fnet, pil_img)
            pred = svm.predict([emb])[0]

            y_true.append(person)
            y_pred.append(pred)

    # Print results
    print("Classification Report:\n")
    print(classification_report(y_true, y_pred))
    print("\nConfusion Matrix:\n")
    print(confusion_matrix(y_true, y_pred))

if __name__ == "__main__":
    main()
