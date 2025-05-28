# esp32_auth/serial_manager.py

import serial, threading, time
from serial.tools import list_ports
from config import SERIAL_LABELS, BAUD_RATE, SERIAL_TIMEOUT
import config

class SerialDevice:
    def __init__(self, port: str, label: str, on_receive):
        self.port   = port
        self.label  = label
        self.ser    = serial.Serial(port, BAUD_RATE, timeout=SERIAL_TIMEOUT)
        self.on_receive = on_receive
        self._running = True
        threading.Thread(target=self._read_loop, daemon=True).start()

    def _read_loop(self):
        while self._running and self.ser.is_open:
            try:
                line = self.ser.readline().decode(errors='ignore').rstrip()
                if line:
                    self.on_receive(self.label, line)
            except Exception:
                pass
            time.sleep(0.05)

    def send(self, msg: str):
        if self.ser.is_open:
            self.ser.write((msg + '\r\n').encode())

    def close(self):
        self._running = False
        time.sleep(0.1)
        self.ser.close()

class SerialManager:
    def __init__(self, on_receive):
        self.on_receive = on_receive
        self.devices = {}  # label -> SerialDevice

    @staticmethod
    def available_ports():
        ports = list_ports.comports()
        if config.PLATFORM == "windows":
            return [p.device for p in ports]
        else:
            return [p.device for p in ports if 'USB' in p.device.upper()]
        
    def connect(self, port: str, label: str):
        if label in self.devices:
            raise ValueError(f"{label} already connected")
        self.devices[label] = SerialDevice(port, label, self.on_receive)

    def send(self, label: str, msg: str):
        if label in self.devices:
            self.devices[label].send(msg)
        else:
            raise KeyError(f"No device labeled {label}")

    def disconnect(self, label: str):
        if label in self.devices:
            self.devices[label].close()
            del self.devices[label]
