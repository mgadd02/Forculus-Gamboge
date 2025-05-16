import cv2
import os
from glob import glob

os.makedirs('frames', exist_ok=True)

# process all mp4 files in ./facial_recognition
for video_path in glob('./video_samples/*.mp4'):
    cap = cv2.VideoCapture(video_path)
    basename = os.path.splitext(os.path.basename(video_path))[0]
    idx = 0
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break
        cv2.imwrite(f'frames/{basename}_{idx:05d}.jpg', frame)
        idx += 1
    cap.release()
