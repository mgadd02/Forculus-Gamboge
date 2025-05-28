#!/usr/bin/env python3
# esp32_auth/gui/serial_tab.py

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import serial.tools.list_ports
import queue
import time
import re

import config
from serial_manager import SerialManager
from sensor_store import store

# Regex to strip ANSI escape sequences
ANSI_ESCAPE = re.compile(r'\x1B\[[0-?]*[ -/]*[@-~]')
# Regex to catch "pin: 12345" anywhere
PIN_REGEX = re.compile(r'pin:\s*(\d+)', re.IGNORECASE)

class SerialTab(ttk.Frame):
    POLL_INTERVAL_MS = 1000
    MAX_QUEUE_SIZE   = 5000
    MAX_LINES        = 1000
    PRUNE_LINES      = 200

    def __init__(self, parent):
        super().__init__(parent)
        # Thread-safe queue and flush flag
        self._msg_queue = queue.Queue(maxsize=self.MAX_QUEUE_SIZE)
        self._flush_scheduled = False

        # Polling flags
        self.sensor_polling = False
        self.door_polling   = False

        # Serial manager invokes _enqueue_message on each incoming line
        self.manager = SerialManager(self._enqueue_message)

        self._build_ui()
        self._refresh_available_ports()
        self.after(self.POLL_INTERVAL_MS, self._poll_ports)

    def _build_ui(self):
        # Top row: port selector, label selector, add button
        top = ttk.Frame(self)
        top.grid(row=0, column=0, columnspan=5, sticky='ew', padx=5, pady=5)
        top.columnconfigure(1, weight=1)

        ttk.Label(top, text="Port:").grid(row=0, column=0, sticky='w')
        self.port_combo = ttk.Combobox(top, state='readonly')
        self.port_combo.grid(row=0, column=1, sticky='ew')

        ttk.Label(top, text="Label:").grid(row=0, column=2, sticky='w')
        self.label_combo = ttk.Combobox(top, values=config.SERIAL_LABELS, state='readonly')
        self.label_combo.grid(row=0, column=3, sticky='w')

        self.btn_add = ttk.Button(top, text="Add Connection", command=self._add_connection)
        self.btn_add.grid(row=0, column=4, padx=5)

        # Row 1: active connections table
        self.tree = ttk.Treeview(self, columns=("Label","Port","Status"), show='headings', height=4)
        for col in ("Label","Port","Status"):
            self.tree.heading(col, text=col)
            self.tree.column(col, width=100, anchor='center')
        self.tree.grid(row=1, column=0, columnspan=5, sticky='nsew', padx=5)

        # Row 2: controls for remove, sensor poll, door poll
        self.btn_remove       = ttk.Button(self, text="Remove Selected", command=self._remove_connection)
        self.btn_remove.grid(row=2, column=0, pady=5, sticky='w')

        self.btn_sensor_start = ttk.Button(self, text="Start Sensor Poll", command=self._toggle_sensor_poll)
        self.btn_sensor_start.grid(row=2, column=1)
        self.btn_sensor_stop  = ttk.Button(self, text="Stop Sensor Poll", command=self._toggle_sensor_poll, state='disabled')
        self.btn_sensor_stop.grid(row=2, column=2)

        self.btn_door_start   = ttk.Button(self, text="Start Door Poll", command=self._toggle_door_poll)
        self.btn_door_start.grid(row=2, column=3)
        self.btn_door_stop    = ttk.Button(self, text="Stop Door Poll", command=self._toggle_door_poll, state='disabled')
        self.btn_door_stop.grid(row=2, column=4)

        # Row 3: console log
        self.txt = scrolledtext.ScrolledText(self, height=10, state='disabled')
        self.txt.grid(row=3, column=0, columnspan=5, sticky='nsew', padx=5, pady=5)

        # Row 4: manual send
        self.entry   = ttk.Entry(self)
        self.entry.grid(row=4, column=0, columnspan=4, sticky='ew', padx=5, pady=5)
        self.btn_send = ttk.Button(self, text="Send", command=self._send, state='disabled')
        self.btn_send.grid(row=4, column=4, padx=5, pady=5)

        self.grid_rowconfigure(3, weight=1)
        self.grid_columnconfigure(0, weight=1)

    def _refresh_available_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo['values'] = ports
        if self.port_combo.get() not in ports:
            self.port_combo.set('')

    def _add_connection(self):
        port  = self.port_combo.get()
        label = self.label_combo.get()
        if not port or not label:
            return messagebox.showwarning("Selection Required", "Select both a port and a label.")
        # Prevent duplicate labels or ports
        for iid in self.tree.get_children():
            lbl, prt, _ = self.tree.item(iid, 'values')
            if lbl == label:
                return messagebox.showerror("Label In Use", f"'{label}' is already assigned.")
            if prt == port:
                return messagebox.showerror("Port In Use", f"'{port}' is already connected.")
        try:
            self.manager.connect(port, label)
        except Exception as e:
            return messagebox.showerror("Connection Failed", str(e))

        self.tree.insert('', 'end', iid=label, values=(label, port, "Connected"))
        self.btn_send.config(state='normal')
        self._log(f"[*] {label} ↔ {port} connected")

    def _remove_connection(self):
        sel = self.tree.selection()
        if not sel:
            return
        label = sel[0]
        try:
            self.manager.disconnect(label)
        except Exception as e:
            messagebox.showerror("Disconnect Error", str(e))
        self.tree.delete(label)
        if not self.tree.get_children():
            self.btn_send.config(state='disabled')
        self._log(f"[!] {label} disconnected")

    def _toggle_sensor_poll(self):
        if not self.sensor_polling:
            if 'base_sensor' not in self.manager.devices:
                return messagebox.showwarning("Not Connected", "Connect 'base_sensor' first.")
            self.sensor_polling = True
            self.btn_sensor_start.config(state='disabled')
            self.btn_sensor_stop .config(state='normal')
            self._sensor_poll()
        else:
            self.sensor_polling = False
            self.btn_sensor_start.config(state='normal')
            self.btn_sensor_stop .config(state='disabled')

    def _sensor_poll(self):
        if not self.sensor_polling:
            return
        try:
            self.manager.send('base_sensor', 'get')
        except KeyError:
            self.sensor_polling = False
            self.btn_sensor_start.config(state='normal')
            self.btn_sensor_stop .config(state='disabled')
            return messagebox.showerror("Error", "'base_sensor' disconnected.")
        self.after(1000, self._sensor_poll)

    def _toggle_door_poll(self):
        if not self.door_polling:
            if 'base_door' not in self.manager.devices:
                return messagebox.showwarning("Not Connected", "Connect 'base_door' first.")
            self.door_polling = True
            self.btn_door_start.config(state='disabled')
            self.btn_door_stop .config(state='normal')
            self._door_poll()
        else:
            self.door_polling = False
            self.btn_door_start.config(state='normal')
            self.btn_door_stop .config(state='disabled')

    def _door_poll(self):
        if not self.door_polling:
            return
        try:
            self.manager.send('base_door', 'status')
        except KeyError:
            self.door_polling = False
            self.btn_door_start.config(state='normal')
            self.btn_door_stop .config(state='disabled')
            return messagebox.showerror("Error", "'base_door' disconnected.")
        self.after(1000, self._door_poll)

    def _enqueue_message(self, label, line):
        # 1) strip ANSI escapes & whitespace
        clean = ANSI_ESCAPE.sub('', line).strip()
        # 2) drop empty or <inf> lines
        if not clean or '<inf>' in clean:
            return
        # 3) enqueue for immediate UI flush
        try:
            self._msg_queue.put_nowait((label, clean))
        except queue.Full:
            self._msg_queue.get_nowait()
            self._msg_queue.put_nowait((label, clean))
        if not self._flush_scheduled:
            self._flush_scheduled = True
            self.after(0, self._flush_messages)
        # 4) parse/store/trigger door logic
        self._parse_message(label, clean)

    def _parse_message(self, label, line):
        # --- Sensor node parsing ---
        if label == 'base_sensor':
            if 'HTS221' in line:
                m1 = re.search(r'Temp:\s*([\d\.]+)', line)
                m2 = re.search(r'Hum:\s*([\d\.]+)', line)
                if m1 and m2:
                    store.add(label, 'Temp', float(m1.group(1)))
                    store.add(label, 'Hum',  float(m2.group(1)))
            elif 'LPS22HB' in line:
                m1 = re.search(r'Press:\s*([\d\.]+)', line)
                m2 = re.search(r'Temp:\s*([\d\.]+)', line)
                if m1 and m2:
                    store.add(label, 'Press',      float(m1.group(1)))
                    store.add(label, 'SensorTemp', float(m2.group(1)))
            elif 'CCS811' in line:
                m1 = re.search(r'eCO2:\s*([\d\.]+)', line)
                m2 = re.search(r'eTVOC:\s*([\d\.]+)', line)
                if m1 and m2:
                    store.add(label, 'eCO2',  float(m1.group(1)))
                    store.add(label, 'eTVOC', float(m2.group(1)))
            elif 'RSSI' in line:
                m = re.search(r'RSSI:\s*([-\d\.]+)', line)
                if m:
                    store.add(label, 'RSSI', float(m.group(1)))

        # --- Door node parsing & pin logic ---
        elif label == 'base_door':
            # pin anywhere in line?
            m = PIN_REGEX.search(line)
            if m:
                pin = m.group(1)
                cur = store.get_current().get('camera', {})
                known = cur.get('person_present',0)==1 and cur.get('person','')!='Unknown'
                if known and pin == config.CORRECT_PIN:
                    self._log("[base_door] ▶ door unlock")
                    self.manager.send('base_door', 'door unlock')
                    self.after(5000, self._auto_lock)
                elif known:
                    self._log(f"[base_door] ❌ Invalid PIN {pin}")
                return
            # ultrasonic
            if line.startswith('ultrasonic:'):
                try:
                    val = float(line.split(':',1)[1].strip())
                    store.add(label, 'ultrasonic', val)
                except:
                    pass
            # magnetometer
            elif line.startswith('magnetometer:'):
                try:
                    val = float(line.split(':',1)[1].strip())
                    store.add(label, 'magnetometer', val)
                except:
                    pass
            # door state
            elif line.startswith('Door is'):
                state = line.split('Door is',1)[1].strip()
                store.add(label, 'door_state', state)

    def _auto_lock(self):
        self._log("[base_door] ▶ door lock")
        self.manager.send('base_door', 'door lock')

    def _flush_messages(self):
        chunk = []
        while not self._msg_queue.empty():
            lbl, txt = self._msg_queue.get()
            chunk.append(f"[{lbl}] {txt}")
        if chunk:
            text = "\n".join(chunk) + "\n"
            self.txt.configure(state='normal')
            self.txt.insert('end', text)
            total = int(self.txt.index('end-1c').split('.')[0])
            if total > self.MAX_LINES:
                self.txt.delete('1.0', f"{total - self.PRUNE_LINES}.0")
            self.txt.see('end')
            self.txt.configure(state='disabled')
        self._flush_scheduled = False

    def _send(self):
        sel = self.tree.selection()
        if not sel:
            return messagebox.showwarning("No Device", "Select a device first.")
        label = sel[0]
        cmd   = self.entry.get().strip()
        if not cmd:
            return
        try:
            self.manager.send(label, cmd)
            self._log(f"[{label}] ▶ {cmd}")
            self.entry.delete(0, 'end')
        except KeyError:
            messagebox.showerror("Send Failed", f"No device '{label}' connected.")

    def _log(self, msg):
        ts = time.strftime("%H:%M:%S")
        self.txt.configure(state='normal')
        self.txt.insert('end', f"{ts} – {msg}\n")
        total = int(self.txt.index('end-1c').split('.')[0])
        if total > self.MAX_LINES:
            self.txt.delete('1.0', f"{total - self.PRUNE_LINES}.0")
        self.txt.see('end')
        self.txt.configure(state='disabled')

    def _poll_ports(self):
        self._refresh_available_ports()
        existing = {p.device for p in serial.tools.list_ports.comports()}
        to_remove = []
        for iid in self.tree.get_children():
            lbl, prt, _ = self.tree.item(iid, 'values')
            dev = self.manager.devices.get(lbl)
            alive = dev and getattr(dev.ser, 'is_open', False)
            if prt not in existing or not alive:
                to_remove.append(lbl)
        for lbl in to_remove:
            self.manager.disconnect(lbl)
            self.tree.delete(lbl)
            self._log(f"[!] {lbl} auto-disconnected")
        for iid in self.tree.get_children():
            lbl, _, _ = self.tree.item(iid, 'values')
            dev = self.manager.devices.get(lbl)
            status = "Connected" if dev and dev.ser.is_open else "Disconnected"
            self.tree.set(iid, "Status", status)
        self.after(self.POLL_INTERVAL_MS, self._poll_ports)
