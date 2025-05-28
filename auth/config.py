#!/usr/bin/env python3
import os
import sys

# -----------------------------------------------------------------------------
# Platform selection: 'windows' or 'linux'
PLATFORM = "windows" if sys.platform.startswith("win") else "linux"

# -----------------------------------------------------------------------------
# Base directory (so model files work cross-platform)
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# -----------------------------------------------------------------------------
# Face-recognition model files
TS_MODEL_FILE  = os.path.join(BASE_DIR, "fnet_ts.pt")
SVM_MODEL_FILE = os.path.join(BASE_DIR, "face_recognizer_svm.pkl")

# -----------------------------------------------------------------------------
# ESP32-CAM settings
CAM_HOST     = "192.168.92.137"
STREAM_URL   = f"http://{CAM_HOST}:81/stream"
FRAME_WIDTH  = 640
FRAME_HEIGHT = 480
THRESHOLD    = 0.85

# -----------------------------------------------------------------------------
# Supported camera resolutions
RESOLUTIONS = [
    ("UXGA (1600x1200)", "15"), ("SXGA (1280x1024)", "14"),
    ("HD (1280x720)",   "13"), ("XGA (1024x768)",  "12"),
    ("SVGA (800x600)",  "11"), ("VGA (640x480)",   "10"),
    ("HVGA (480x320)",  "9"),  ("CIF (400x296)",   "8"),
    ("QVGA (320x240)",  "6"),  ("240x240",         "5"),
    ("HQVGA (240x176)", "4"),  ("QCIF (176x144)",  "3"),
    ("128x128",         "2"),  ("QQVGA (160x120)", "1"),
    ("96x96",           "0"),
]

# -----------------------------------------------------------------------------
# Serial-port settings
SERIAL_LABELS   = ["base_sensor", "base_door", "other"]
BAUD_RATE       = 115200
SERIAL_TIMEOUT  = 0.1

# -----------------------------------------------------------------------------
# Door-unlock PIN
CORRECT_PIN = "65896"

# -----------------------------------------------------------------------------
# Sensor-fusion (Kalman) parameters
KALMAN_Q             = 1e-5    # process noise
KALMAN_R             = 1e-2    # measurement noise
ANOMALY_STD_DEV_THRESH = 3.0   # anomalies are ±3σ

# -----------------------------------------------------------------------------
# Blockchain settings
RPC_URL         = "https://mainnet.infura.io/v3/YOUR-PROJECT-ID"
BLOCKCHAIN_DIR  = os.path.join(BASE_DIR, "blockchain")
