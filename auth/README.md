# SLARM System

A Python/Tkinter application integrating ESP32‐CAM face recognition, serial sensor & door control, time‐series fusion/anomaly detection, a simple blockchain ledger, and MQTT publishing.

## Features

- **Camera Tab**: Live MJPEG stream from ESP32-CAM with face detection & SVM-based authentication.
- **Serial Tab**: Connect to multiple USB/COM devices (`base_sensor`, `base_door`), start/stop polling, console I/O, auto-reconnect.
- **Sensors Tab**: View current temperature, humidity, pressure, gas, door & motion readings.
- **Fusion/Anomaly Tab**: 1D Kalman filtering of each sensor time series, ±% threshold bands, auto-detection and flagging of anomalies, reset button.
- **Blockchain Tab**: Manual SHA-256 “blockchain” stored in `./blockchain/chain.json`, auto-append of buffered sensor batches (1 Hz), prune to last 1000 blocks, view recent blocks & details.
- **MQTT Tab**: Publishes all non-anomaly values every second to a configurable broker/topic and shows last published message.

## Requirements

- Python 3.8+  
- OS: Linux, macOS or Windows  
- Hardware: ESP32-CAM (MJPEG stream), USB serial sensors  

### Python packages

Install dependencies with:

```bash
pip install -r requirements.txt
```

### Usage

Use Command
```python main.py --platform [linux|windows]```

Windows:
```python main.py --platform windows```

Linux:
```python main.py --platform linux```




