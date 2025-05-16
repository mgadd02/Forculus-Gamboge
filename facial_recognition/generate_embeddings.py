#!/usr/bin/env python3
import os
import pickle
from glob import glob

import torch
from facenet_pytorch import InceptionResnetV1
from PIL import Image
import numpy as np

# Paths
DATA_DIR     = "./dataset"           # output of detect_and_crop
EMB_FILE     = "embeddings_fnet.pkl"

# Load pretrained FaceNet
device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
model  = InceptionResnetV1(pretrained='vggface2').eval().to(device)

embs, labels = [], []

# Loop over each person folder
for person in os.listdir(DATA_DIR):
    person_dir = os.path.join(DATA_DIR, person)
    if not os.path.isdir(person_dir):
        continue

    for img_path in glob(os.path.join(person_dir, "*.jpg")):
        img = Image.open(img_path).convert('RGB').resize((160,160))
        img_tensor = torch.tensor(np.asarray(img),
                                  dtype=torch.float32,
                                  device=device).permute(2,0,1).unsqueeze(0)
        img_tensor.div_(255)  # scale to [0,1]
        with torch.no_grad():
            embedding = model(img_tensor).cpu().numpy()[0]
        embs.append(embedding)
        labels.append(person)

# Save to disk
with open(EMB_FILE, 'wb') as f:
    pickle.dump({'embeddings': np.vstack(embs), 'labels': labels}, f)

print(f"Saved {len(labels)} embeddings → {EMB_FILE}")
