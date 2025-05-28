#!/usr/bin/env python3
# esp32_auth/gui/kalman_tab.py

import tkinter as tk
from tkinter import ttk, messagebox
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import matplotlib.dates as mdates
from datetime import datetime
import time

import numpy as np
from config import KALMAN_Q, KALMAN_R, ANOMALY_STD_DEV_THRESH
from sensor_store import store

class KalmanFilter1D:
    def __init__(self, Q, R):
        self.Q = Q
        self.R = R
        self.x = None
        self.P = None

    def update(self, z: float):
        if self.x is None:
            self.x, self.P = z, 1.0
            return self.x, self.P, 0.0
        Pp = self.P + self.Q
        K  = Pp / (Pp + self.R)
        x_up = self.x + K * (z - self.x)
        P_up = (1 - K) * Pp
        resid = z - self.x
        self.x, self.P = x_up, P_up
        return x_up, P_up, resid

class KalmanTab(ttk.Frame):
    REFRESH_MS = 2000

    def __init__(self, parent):
        super().__init__(parent)
        self.filters    = {}  # (sensor,metric) -> KalmanFilter1D
        self.last_index = {}  # (sensor,metric) -> int

        self._build_ui()
        self._refresh_metrics()
        self.after(self.REFRESH_MS, self._auto_run)

    def _build_ui(self):
        # Row 0: dropdown + threshold + buttons
        self.combo = ttk.Combobox(self, state='readonly')
        self.combo.grid(row=0, column=0, padx=5, pady=5, sticky='ew')
        self.combo.bind("<<ComboboxSelected>>", lambda e: self._plot_selected())

        ttk.Label(self, text="Threshold %:").grid(row=0, column=1, padx=5)
        self.threshold_combo = ttk.Combobox(
            self, values=["5","10","15","20","25","50"], state='readonly', width=4
        )
        self.threshold_combo.grid(row=0, column=2, padx=5)
        self.threshold_combo.set("15")
        self.threshold_combo.bind("<<ComboboxSelected>>", lambda e: self._plot_selected())

        self.btn_refresh = ttk.Button(self, text="Refresh Metrics", command=self._refresh_metrics)
        self.btn_refresh.grid(row=0, column=3, padx=5)

        self.btn_reset = ttk.Button(self, text="Reset Anomalies", command=self._reset_anomalies)
        self.btn_reset.grid(row=0, column=4, padx=5)

        # Row 1: plot area
        self.fig = Figure(figsize=(5,3))
        self.ax  = self.fig.add_subplot(111)
        self.canvas = FigureCanvasTkAgg(self.fig, master=self)
        self.canvas.get_tk_widget().grid(row=1, column=0, columnspan=5, sticky='nsew')

        # Row 2: anomalies table
        self.tree = ttk.Treeview(
            self,
            columns=("Sensor","Metric","Time","Value","Residual"),
            show='headings', height=6
        )
        for col, title in zip(
            ("Sensor","Metric","Time","Value","Residual"),
            ("Sensor","Metric","Timestamp","Value","Residual")
        ):
            self.tree.heading(col, text=title)
            self.tree.column(col, anchor='center')
        self.tree.grid(row=2, column=0, columnspan=5, sticky='nsew', padx=5, pady=5)

        # Configure resizing
        self.grid_rowconfigure(1, weight=1)
        self.grid_columnconfigure(0, weight=1)

    def _refresh_metrics(self):
        """Rebuild dropdown and reset filters/indices/table."""
        items = []
        for sensor, metrics in store.store.items():
            if sensor == 'camera':
                continue
            for metric in metrics.keys():
                items.append(f"{sensor}|{metric}")
        self.combo['values'] = items
        if items:
            self.combo.current(0)
        # clear table and filters
        for iid in self.tree.get_children():
            self.tree.delete(iid)
        self.filters.clear()
        self.last_index.clear()
        self._plot_selected()

    def _auto_run(self):
        """Continuously process new data, detect anomalies by percentage threshold."""
        pct = float(self.threshold_combo.get()) / 100.0
        for sensor in list(store.store.keys()):
            if sensor == 'camera':
                continue
            for metric in list(store.store[sensor].keys()):
                if metric.endswith('_anomaly'):
                    continue
                hist = store.store[sensor][metric]
                # ensure numeric
                if not hist:
                    continue
                try:
                    float(hist[0][1])
                except:
                    continue

                key = (sensor, metric)
                kf = self.filters.setdefault(key, KalmanFilter1D(KALMAN_Q, KALMAN_R))
                last_i = self.last_index.get(key, 0)

                for ts, val in hist[last_i:]:
                    try:
                        z = float(val)
                    except:
                        continue
                    pred, _, resid = kf.update(z)
                    # detect anomaly: absolute residual > pct * abs(measured value)
                    if abs(resid) > pct * abs(z):
                        store.add(sensor, f"{metric}_anomaly", 1)
                        tstr = datetime.fromtimestamp(ts).strftime("%Y-%m-%d %H:%M:%S")
                        self.tree.insert(
                            '', 'end',
                            values=(sensor, metric, tstr, f"{z:.2f}", f"{resid:.2f}")
                        )
                self.last_index[key] = len(hist)

        # re-plot selection
        self._plot_selected()
        self.after(self.REFRESH_MS, self._auto_run)

    def _plot_selected(self):
        """Plot measured, Kalman prediction, and percentage threshold band."""
        sel = self.combo.get()
        if not sel:
            self.ax.clear()
            self.canvas.draw()
            return
        sensor, metric = sel.split("|",1)
        hist = store.get_history(sensor, metric)
        if not hist:
            self.ax.clear()
            self.canvas.draw()
            return

        # times & values
        times = [datetime.fromtimestamp(ts) for ts, _ in hist]
        vals  = []
        for _, v in hist:
            try:
                vals.append(float(v))
            except:
                vals.append(np.nan)

        # Kalman predictions
        kf_local = KalmanFilter1D(KALMAN_Q, KALMAN_R)
        preds = []
        for z in vals:
            try:
                p,_,_ = kf_local.update(z)
            except:
                p = np.nan
            preds.append(p)

        # threshold percentage band
        pct = float(self.threshold_combo.get()) / 100.0
        upper = [pred + pct * abs(v) for pred, v in zip(preds, vals)]
        lower = [pred - pct * abs(v) for pred, v in zip(preds, vals)]

        # plot
        self.ax.clear()
        self.ax.plot(times, vals, label="Measured")
        self.ax.plot(times, preds, '--', label="Kalman")
        self.ax.plot(times, upper, ':', label=f"+{int(pct*100)}% band")
        self.ax.plot(times, lower, ':', label=f"-{int(pct*100)}% band")
        self.ax.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))
        self.ax.set_title(f"{sensor} – {metric}")
        self.ax.set_ylabel(metric)
        self.ax.legend()
        self.fig.autofmt_xdate()
        self.canvas.draw()

    def _reset_anomalies(self):
        """Reset all anomaly flags to zero and clear table."""
        now = time.time()
        for sensor in list(store.store.keys()):
            for metric in list(store.store[sensor].keys()):
                if metric.endswith('_anomaly'):
                    store.store[sensor][metric] = [(now, 0.0)]
        for iid in self.tree.get_children():
            self.tree.delete(iid)
        self.filters.clear()
        self.last_index.clear()
        self._plot_selected()
        messagebox.showinfo("Reset", "All anomaly flags have been reset.")
