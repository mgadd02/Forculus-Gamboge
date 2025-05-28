# esp32_auth/models.py

import torch, pickle
import numpy as np
from PIL import Image
import cv2
from config import TS_MODEL_FILE, SVM_MODEL_FILE

def load_models():
    fnet = torch.jit.load(TS_MODEL_FILE, map_location="cpu")
    fnet.eval()
    svm  = pickle.load(open(SVM_MODEL_FILE, "rb"))
    return fnet, svm

def get_embedding(fnet, bgr_crop):
    rgb    = cv2.cvtColor(bgr_crop, cv2.COLOR_BGR2RGB)
    img    = Image.fromarray(rgb).resize((160,160))
    arr    = np.asarray(img).transpose(2,0,1)[None]/255.0
    tensor = torch.tensor(arr, dtype=torch.float32)
    with torch.no_grad():
        emb = fnet(tensor).cpu().numpy()[0]
    return emb
