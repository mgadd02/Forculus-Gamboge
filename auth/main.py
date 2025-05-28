#!/usr/bin/env python3
import argparse
import tkinter as tk
from tkinter import ttk

import config
from models            import load_models
from gui.camera_tab    import CameraTab
from gui.serial_tab    import SerialTab
from gui.sensor_tab    import SensorTab
from gui.kalman_tab    import KalmanTab
from gui.blockchain_tab import BlockchainTab
from gui.mqtt_tab import MqttTab

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", choices=["linux","windows"])
    args = parser.parse_args()
    if args.platform:
        config.PLATFORM = args.platform

    root = tk.Tk()
    root.title("SLARM System")

    fnet, svm = load_models()
    nb = ttk.Notebook(root)
    nb.add(CameraTab     (nb, fnet, svm), text="Camera")
    nb.add(SerialTab     (nb)          , text="Serial")
    nb.add(SensorTab     (nb)          , text="Sensors")
    nb.add(KalmanTab     (nb)          , text="Fusion/Anomaly")
    nb.add(BlockchainTab (nb)          , text="Blockchain")
    nb.add(MqttTab(nb), text="MQTT")
    nb.pack(fill="both", expand=True)

    root.mainloop()

if __name__ == "__main__":
    main()
