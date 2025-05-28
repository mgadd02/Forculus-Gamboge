#!/usr/bin/env python3
# esp32_auth/gui/sensor_tab.py

import tkinter as tk
from tkinter import ttk

from sensor_store import store

class SensorTab(ttk.Frame):
    REFRESH_MS = 1000

    def __init__(self, parent):
        super().__init__(parent)
        self.tree = ttk.Treeview(self, columns=("Sensor","Metric","Value"), show="headings")
        for col in ("Sensor","Metric","Value"):
            self.tree.heading(col, text=col)
            self.tree.column(col, anchor="center", width=100)
        self.tree.pack(fill="both", expand=True, padx=5, pady=5)

        self._update_ui()

    def _update_ui(self):
        # clear
        for item in self.tree.get_children():
            self.tree.delete(item)

        # current readings
        current = store.get_current()
        for sensor, metrics in current.items():
            for metric, val in metrics.items():
                self.tree.insert("", "end", values=(sensor, metric, val))

        self.after(self.REFRESH_MS, self._update_ui)
