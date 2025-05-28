#!/usr/bin/env python3
# esp32_auth/gui/mqtt_tab.py

import threading
import time
import paho.mqtt.client as mqtt
import tkinter as tk
from tkinter import ttk, messagebox

from sensor_store import store

class MqttTab(ttk.Frame):
    PUBLISH_INTERVAL = 1.0   # seconds between publishes
    BROKER   = "broker.hivemq.com"
    PORT     = 1883
    TOPIC    = "topic/test/esp32_sub"

    def __init__(self, parent):
        super().__init__(parent)
        self._running = True

        # ——— MQTT setup ———
        self.client = mqtt.Client(client_id="SLARM_MQTT_Publisher")
        try:
            self.client.connect(self.BROKER, self.PORT, keepalive=60)
        except Exception as e:
            messagebox.showerror("MQTT Connection Failed", str(e))
        else:
            self.client.loop_start()

        # ——— UI ———
        self._build_ui()

        # ——— Start background publisher ———
        threading.Thread(target=self._publish_loop, daemon=True).start()

    def _build_ui(self):
        frm = ttk.Frame(self)
        frm.pack(fill='x', padx=5, pady=5)

        ttk.Label(frm, text=f"Broker: {self.BROKER}:{self.PORT}").grid(row=0, column=0, sticky='w')
        ttk.Label(frm, text=f"Topic:  {self.TOPIC}"         ).grid(row=1, column=0, sticky='w')

        self.lbl_last = ttk.Label(self, text="Last sent: (none)", anchor='w')
        self.lbl_last.pack(fill='x', padx=5, pady=(5,0))

    def _publish_loop(self):
        while self._running:
            # 1) gather current non-anomaly values
            current = store.get_current()  # { sensor: {metric: value, ...}, ... }
            parts = []
            for sensor, metrics in current.items():
                for metric, val in metrics.items():
                    if metric.endswith('_anomaly'):
                        continue
                    # format as [sensor,metric,value]
                    parts.append(f"[{sensor},{metric},{val}]")

            message = ",".join(parts) if parts else "[no_data,0]"

            # 2) publish
            try:
                self.client.publish(self.TOPIC, message)
            except Exception as e:
                print(f"[MQTT] publish error: {e}")

            # 3) update UI on main thread
            self.after(0, lambda m=message: self.lbl_last.config(text=f"Last sent: {m}"))

            # 4) wait
            time.sleep(self.PUBLISH_INTERVAL)

    def destroy(self):
        # stop publishing and disconnect cleanly
        self._running = False
        try:
            self.client.loop_stop()
            self.client.disconnect()
        except:
            pass
        super().destroy()
