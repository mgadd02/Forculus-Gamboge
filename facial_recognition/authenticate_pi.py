#!/usr/bin/env python3
import cv2
import threading
import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
from PIL import Image, ImageTk
import face_recognition
import torch
import pickle
import numpy as np
import serial
import serial.tools.list_ports
import time
import urllib.request, urllib.error

# ===== Configuration =====
CAM_HOST         = "192.168.92.137"                     # your ESP32-CAM IP
STREAM_URL       = f"http://{CAM_HOST}:81/stream"       # MJPEG stream URL
FRAME_WIDTH      = 640
FRAME_HEIGHT     = 480
TS_MODEL_FILE    = "fnet_ts.pt"
SVM_MODEL_FILE   = "face_recognizer_svm.pkl"
THRESHOLD        = 0.85                                 # raised threshold
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
# ==========================

def load_models():
    fnet = torch.jit.load(TS_MODEL_FILE, map_location="cpu")
    fnet.eval()
    svm = pickle.load(open(SVM_MODEL_FILE, "rb"))
    return fnet, svm

def get_embedding(fnet, bgr_crop):
    rgb = cv2.cvtColor(bgr_crop, cv2.COLOR_BGR2RGB)
    img = Image.fromarray(rgb).resize((160,160))
    arr = np.asarray(img).transpose(2,0,1)[None]/255.0
    tensor = torch.tensor(arr, dtype=torch.float32)
    with torch.no_grad():
        emb = fnet(tensor).cpu().numpy()[0]
    return emb

class CameraTab(ttk.Frame):
    def __init__(self, parent, fnet, svm):
        super().__init__(parent)
        self.fnet = fnet
        self.svm = svm
        self.cap = None
        self.running = False

        # Shared data
        self.latest_frame = None
        self.detection = None   # (top, right, bottom, left, label, color)
        self.rec_fps = 0.0
        self.lock = threading.Lock()

        # Timing for RAW FPS
        self.prev_time = time.time()

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

        # Start capture and recognition threads
        threading.Thread(target=self._capture_loop,   daemon=True).start()
        threading.Thread(target=self._recognition_loop, daemon=True).start()
        # Start the GUI display loop
        self._update_frame()

    def stop_stream(self):
        self.running = False
        self.btn_start.config(state='normal')
        self.btn_stop.config(state='disabled')
        if self.cap:
            self.cap.release()
            self.cap = None

    def _capture_loop(self):
        """Continuously read from the MJPEG stream."""
        while self.running and self.cap:
            ret, frame = self.cap.read()
            if ret:
                with self.lock:
                    self.latest_frame = frame
            else:
                time.sleep(0.01)

    def _recognition_loop(self):
        """Continuously run face-recognition on the newest frame."""
        while self.running:
            frame = None
            with self.lock:
                if self.latest_frame is not None:
                    frame = self.latest_frame.copy()
            if frame is not None:
                t0 = time.time()
                rgb  = frame[:,:,::-1]
                locs = face_recognition.face_locations(rgb, model="hog")
                if locs:
                    top, right, bottom, left = locs[0]
                    crop = frame[top:bottom, left:right]
                    emb  = get_embedding(self.fnet, crop)
                    probs = self.svm.predict_proba([emb])[0]
                    idx, prob = np.argmax(probs), np.max(probs)
                    name = self.svm.classes_[idx]
                    if prob >= THRESHOLD:
                        label, color = f"✔ {name} ({prob:.2f})", (0,255,0)
                        self.log_message(f"Authorized: {name} ({prob:.2f})")
                    else:
                        label, color = f"✖ Unknown ({prob:.2f})", (0,0,255)
                        self.log_message(f"Unauthorized ({prob:.2f})")
                    detection = (top, right, bottom, left, label, color)
                else:
                    detection = None

                # record recognition FPS
                dt = time.time() - t0
                with self.lock:
                    self.detection = detection
                    self.rec_fps   = 1.0/dt if dt>0 else 0.0
            time.sleep(0.01)

    def _update_frame(self):
        """Pull the latest frame and draw it (with any last detection) onto the Tk canvas."""
        if self.running:
            frame = None
            with self.lock:
                if self.latest_frame is not None:
                    frame   = self.latest_frame.copy()
                    rec_fps = self.rec_fps
                    det     = self.detection
            if frame is not None:
                # compute RAW FPS
                t1 = time.time()
                raw_fps = 1.0/(t1 - self.prev_time) if t1 != self.prev_time else 0.0
                self.prev_time = t1

                # overlay last detection
                if det:
                    top, right, bottom, left, label, color = det
                    cv2.rectangle(frame, (left, top), (right, bottom), color, 2)
                    cv2.putText(frame, label, (left, top-10),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)

                # overlay both FPS
                cv2.putText(frame,
                            f"RAW FPS: {raw_fps:.1f} | REC FPS: {rec_fps:.1f}",
                            (10, FRAME_HEIGHT-10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255,255,255), 2)

                # display
                frame = cv2.resize(frame, (FRAME_WIDTH, FRAME_HEIGHT))
                img   = ImageTk.PhotoImage(Image.fromarray(frame[:,:,::-1]))
                self.canvas.img = img
                self.canvas.create_image(0,0, anchor='nw', image=img)

            # schedule next draw ASAP
            self.after(10, self._update_frame)

class SerialTab(ttk.Frame):
    def __init__(self, parent):
        super().__init__(parent)
        self.ser = None
        self.running = False

        ports = [p.device for p in serial.tools.list_ports.comports() if 'USB' in p.device.upper()]
        self.cmb = ttk.Combobox(self, values=ports, state='readonly')
        if ports: self.cmb.current(0)
        self.cmb.grid(row=0, column=0, padx=5, pady=5, sticky='ew')
        self.btn_conn = ttk.Button(self, text="Connect", command=self.toggle_connection)
        self.btn_conn.grid(row=0, column=1, padx=5, pady=5)

        self.txt = scrolledtext.ScrolledText(self, height=15, state='disabled')
        self.txt.grid(row=1, column=0, columnspan=2, sticky="nsew", padx=5, pady=5)

        self.entry = ttk.Entry(self)
        self.entry.grid(row=2, column=0, sticky='ew', padx=5, pady=5)
        self.btn_send = ttk.Button(self, text="Send", command=self.send_command, state='disabled')
        self.btn_send.grid(row=2, column=1, padx=5, pady=5)

        self.grid_rowconfigure(1, weight=1)
        self.grid_columnconfigure(0, weight=1)

    def toggle_connection(self):
        if not self.ser:
            port = self.cmb.get()
            if not port: return
            try:
                self.ser = serial.Serial(port, 115200, timeout=0.1)
            except Exception as e:
                messagebox.showerror("Serial Error", str(e))
                return
            self.running = True
            self.btn_conn.config(text="Disconnect")
            self.btn_send.config(state='normal')
            threading.Thread(target=self._read_loop, daemon=True).start()
        else:
            self.running = False
            time.sleep(0.2)
            self.ser.close()
            self.ser = None
            self.btn_conn.config(text="Connect")
            self.btn_send.config(state='disabled')

    def _read_loop(self):
        while self.running and self.ser and self.ser.is_open:
            try:
                line = self.ser.readline().decode(errors='ignore')
                if line:
                    self.txt.configure(state='normal')
                    self.txt.insert('end', line)
                    self.txt.see('end')
                    self.txt.configure(state='disabled')
            except:
                pass
            time.sleep(0.05)

    def send_command(self):
        if self.ser and self.ser.is_open:
            cmd = self.entry.get().strip()
            if cmd:
                self.ser.write((cmd + '\r\n').encode())
                self.entry.delete(0, 'end')

def main():
    root = tk.Tk()
    root.title("ESP32-CAM Face Authenticator")

    fnet, svm = load_models()

    nb = ttk.Notebook(root)
    nb.add(CameraTab(nb, fnet, svm), text="Camera")
    nb.add(SerialTab (nb)      , text="Serial")
    nb.pack(fill='both', expand=True)

    root.mainloop()

if __name__ == "__main__":
    main()
