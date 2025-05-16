#!/usr/bin/env python3
import os
import random
import re
from glob import glob
from multiprocessing import Pool, cpu_count

from PIL import Image
import face_recognition

# ===== Configuration (edit as needed) =====
FRAMES_DIR         = "./frames"    # where your extracted .jpgs live
OUT_DIR            = "./dataset"   # where cropped faces will go
SAMPLES_PER_PERSON = 50            # max frames to sample per person
SEED               = 42            # for reproducible sampling
# ==========================================

def process_image(args):
    img_path, person_out = args
    # load as RGB
    image = face_recognition.load_image_file(img_path)
    # detect faces
    locations = face_recognition.face_locations(image)
    if not locations:
        return 0

    pil_img  = Image.fromarray(image)
    basename = os.path.splitext(os.path.basename(img_path))[0]
    saved = 0

    # crop & save each face
    for i, (top, right, bottom, left) in enumerate(locations):
        face = pil_img.crop((left, top, right, bottom))
        face.save(os.path.join(person_out, f"{basename}_face{i}.jpg"))
        saved += 1

    return saved

def main():
    random.seed(SEED)
    os.makedirs(OUT_DIR, exist_ok=True)

    # gather all frame files
    all_frames = glob(os.path.join(FRAMES_DIR, "*.jpg"))
    by_person = {}

    # group by person name (strip trailing digits)
    for path in all_frames:
        raw = os.path.basename(path).split('_')[0]       # e.g. "corey1"
        person = re.sub(r'\d+$', '', raw)                # → "corey"
        by_person.setdefault(person, []).append(path)

    # build tasks: sample up to N per person
    tasks = []
    for person, imgs in by_person.items():
        n = min(len(imgs), SAMPLES_PER_PERSON)
        sampled = random.sample(imgs, n)
        person_out = os.path.join(OUT_DIR, person)
        os.makedirs(person_out, exist_ok=True)
        tasks += [(img, person_out) for img in sampled]

    # process in parallel
    with Pool(processes=cpu_count()) as pool:
        total = sum(pool.map(process_image, tasks))

    print(f"Done: saved {total} face crops into '{OUT_DIR}/'")

if __name__ == "__main__":
    main()
