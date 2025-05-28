#!/usr/bin/env python3
# esp32_auth/sensor_store.py

import time
import threading
import os
from collections import defaultdict
from blockchain_manual import Blockchain
from config import BLOCKCHAIN_DIR

# ensure blockchain directory exists
os.makedirs(BLOCKCHAIN_DIR, exist_ok=True)

# single, app‐wide blockchain instance
bc = Blockchain()

class SensorStore:
    def __init__(self):
        # in‐memory history: { sensor_label: { metric: [(ts,val),…] } }
        self.store = defaultdict(lambda: defaultdict(list))
        # buffer for blockchain: list of dict entries
        self._buffer = []
        self._lock   = threading.Lock()
        # start background flusher
        t = threading.Thread(target=self._flush_loop, daemon=True)
        t.start()

    def add(self, sensor_label: str, metric: str, value):
        """Record in history, and buffer for blockchain."""
        ts = time.time()
        # 1) history
        self.store[sensor_label][metric].append((ts, value))
        # 2) buffer
        entry = {
            "sensor":    sensor_label,
            "metric":    metric,
            "value":     value,
            "timestamp": ts
        }
        with self._lock:
            self._buffer.append(entry)

    def get_current(self, sensor_label=None):
        """Latest value for each metric."""
        result = {}
        labels = [sensor_label] if sensor_label else list(self.store.keys())
        for lbl in labels:
            result[lbl] = {}
            for m, lst in self.store[lbl].items():
                if lst:
                    result[lbl][m] = lst[-1][1]
        return result

    def get_history(self, sensor_label, metric):
        return self.store[sensor_label][metric]

    def _flush_loop(self):
        """Every second, take all buffered entries and write one block."""
        while True:
            time.sleep(1.0)
            with self._lock:
                if not self._buffer:
                    continue
                batch = self._buffer[:]
                self._buffer.clear()
            try:
                # one block containing the list of entries
                bc.add_block(batch)
            except Exception as e:
                print(f"[SensorStore] blockchain batch add error: {e}")

# shared instance
store = SensorStore()
