#!/usr/bin/env python3
# esp32_auth/gui/camera_tab.py

import threading
import time
import urllib.request
import urllib.error
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext

import cv2
from PIL import Image, ImageTk
import face_recognition

from config import CAM_HOST, STREAM_URL, FRAME_WIDTH, FRAME_HEIGHT, RESOLUTIONS, THRESHOLD
from models import get_embedding
from sensor_store import store

class CameraTab(ttk.Frame):
    def __init__(self, parent, fnet, svm):
        super().__init__(parent)
        self.fnet = fnet
        self.svm  = svm
        self.cap  = None
        self.running = False

        # Shared data
        self.latest_frame = None
        self.detection    = None   # (top, right, bottom, left, label, color)
        self.rec_fps      = 0.0
        self.lock         = threading.Lock()
        self.prev_time    = time.time()

        # --- Controls ---
        ttk.Label(self, text="Resolution:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.cmb_res = ttk.Combobox(self, values=[n for n,_ in RESOLUTIONS], state='readonly')
        default_idx = next(i for i,(_,v) in enumerate(RESOLUTIONS) if v=="6")  # QVGA
        self.cmb_res.current(default_idx)
        self.cmb_res.grid(row=0, column=1, padx=5, pady=5, sticky="w")
        self.cmb_res.bind("<<ComboboxSelected>>", self.change_resolution)

        self.btn_start = ttk.Button(self, text="Start Stream", command=self.start_stream)
        self.btn_start.grid(row=0, column=2, padx=5, pady=5)
        self.btn_stop  = ttk.Button(self, text="Stop Stream",  command=self.stop_stream, state='disabled')
        self.btn_stop.grid(row=0, column=3, padx=5, pady=5)

        # --- Video canvas ---
        self.canvas = tk.Canvas(self, width=FRAME_WIDTH, height=FRAME_HEIGHT, bg='black')
        self.canvas.grid(row=1, column=0, columnspan=4, padx=5, pady=5)

        # --- Recognition log ---
        self.log = scrolledtext.ScrolledText(self, height=8, state='disabled')
        self.log.grid(row=2, column=0, columnspan=4, sticky="nsew", padx=5, pady=5)
        self.grid_rowconfigure(2, weight=1)
        self.grid_columnconfigure(3, weight=1)

        # Auto‐start the stream
        self.start_stream()

    def log_message(self, msg):
        self.log.configure(state='normal')
        self.log.insert('end', f"{time.strftime('%H:%M:%S')} – {msg}\n")
        self.log.see('end')
        self.log.configure(state='disabled')

    def change_resolution(self, event=None):
        name = self.cmb_res.get()
        val  = next(v for n,v in RESOLUTIONS if n==name)
        url  = f"http://{CAM_HOST}/control?var=framesize&val={val}"
        try:
            urllib.request.urlopen(url, timeout=2)
            self.log_message(f"Set camera → {name}")
        except urllib.error.URLError as e:
            messagebox.showerror("Resolution Error", f"Could not set resolution:\n{e}")

    def start_stream(self):
        if self.running:
            return
        self.cap = cv2.VideoCapture(STREAM_URL)
        if not self.cap.isOpened():
            messagebox.showerror("Stream Error", f"Cannot open stream:\n{STREAM_URL}")
            return
        self.running = True
        self.btn_start.config(state='disabled')
        self.btn_stop.config(state='normal')
        threading.Thread(target=self._capture_loop,   daemon=True).start()
        threading.Thread(target=self._recognition_loop, daemon=True).start()
        self._update_frame()

    def stop_stream(self):
        self.running = False
        self.btn_start.config(state='normal')
        self.btn_stop.config(state='disabled')
        if self.cap:
            self.cap.release()
            self.cap = None

    def _capture_loop(self):
        while self.running and self.cap:
            ret, frame = self.cap.read()
            if ret:
                with self.lock:
                    self.latest_frame = frame
            else:
                time.sleep(0.01)

    def _recognition_loop(self):
        while self.running:
            frame = None
            with self.lock:
                if self.latest_frame is not None:
                    frame = self.latest_frame.copy()

            if frame is not None:
                t0   = time.time()
                rgb  = frame[:,:,::-1]
                locs = face_recognition.face_locations(rgb, model="hog")

                if locs:
                    top, right, bottom, left = locs[0]
                    crop = frame[top:bottom, left:right]
                    emb  = get_embedding(self.fnet, crop)
                    probs = self.svm.predict_proba([emb])[0]
                    idx, prob = probs.argmax(), probs.max()
                    name = self.svm.classes_[idx]

                    if prob >= THRESHOLD:
                        label, color = f"{name} ({prob:.2f})", (0,255,0)
                        self.log_message(f"Authorized: {name} ({prob:.2f})")
                        store.add('camera', 'person_present', 1)
                        store.add('camera', 'person', name)
                    else:
                        label, color = f"Unknown ({prob:.2f})", (0,0,255)
                        self.log_message(f"Unauthorized ({prob:.2f})")
                        store.add('camera', 'person_present', 1)
                        store.add('camera', 'person', 'Unknown')
                else:
                    label = None
                    self.log_message("No face detected")
                    store.add('camera', 'person_present', 0)
                    store.add('camera', 'person', '')

                detection = (top, right, bottom, left, label, color) if locs else None
                dt = time.time() - t0

                with self.lock:
                    self.detection = detection
                    self.rec_fps   = 1.0/dt if dt>0 else 0.0

            time.sleep(0.01)

    def _update_frame(self):
        if not self.running:
            return

        frame = None
        with self.lock:
            if self.latest_frame is not None:
                frame   = self.latest_frame.copy()
                rec_fps = self.rec_fps
                det     = self.detection

        if frame is not None:
            t1 = time.time()
            raw_fps = 1.0/(t1 - self.prev_time) if t1!=self.prev_time else 0.0
            self.prev_time = t1

            if det:
                top, right, bottom, left, label, color = det
                if label:
                    cv2.rectangle(frame, (left, top), (right, bottom), color, 2)
                    cv2.putText(frame, label, (left, top-10),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)

            cv2.putText(frame,
                        f"RAW FPS: {raw_fps:.1f} | REC FPS: {rec_fps:.1f}",
                        (10, FRAME_HEIGHT-10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255,255,255), 2)

            frame = cv2.resize(frame, (FRAME_WIDTH, FRAME_HEIGHT))
            img = ImageTk.PhotoImage(Image.fromarray(frame[:,:,::-1]))
            self.canvas.img = img
            self.canvas.create_image(0,0, anchor='nw', image=img)

        self.after(10, self._update_frame)
