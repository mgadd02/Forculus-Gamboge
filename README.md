# Forculus-Gamboge
4011 Team Project. Max, Peter, Corey.

# SLARM: Smart Lock and Room Monitoring System

## Project Description  
The SLARM system integrates door access control with environmental monitoring to provide a secure, data-driven solution for room management. A network of IoT boards handles user authentication, sensor data collection, local and remote dashboards, and actuators (LED or servo) to lock or unlock doors. Australian English spelling and punctuation are used throughout.

## Hardware Components  
- **Primary Door Node (DiscoL475 IoT Discovery Board)**  
  - Keypad for PIN entry  
  - NFC reader/writer and NFC card for authentication  
  - Ultrasonic sensor for door‐presence detection  
  - Actuator: LED indicator (base node) or servo motor (door node)  

- **Base Node (nRF52840 DK)**  
  - Central BLE gateway for Thingy:52 and DiscoL475  
  - Serial link to Raspberry Pi 5  
  - Status LEDs for locked/unlocked and temperature warnings  

- **Admin Dashboard Display (M5Core 2)**  
  - BLE client displaying real-time sensor readings, door status, entry/exit logs  

- **Environmental Sensor Node (Thingy:52)**  
  - Temperature and air-quality sensing  
  - RGB LED to reflect air-quality status  
  - BLE transmitter to Base Node  

- **Facial Recognition Node (Raspberry Pi 5 4 GB + Arducam ESP32-CAM)**  
  - Convolutional Neural Network (CNN) for face recognition  
  - Serial communication to Base Node  

- **Development & Communication**  
  - MQTT broker (online dashboard & data logging)  
  - Optional blockchain ledger for immutable access logs  

## Team Allocation  
- **Corey**  
  - MQTT infrastructure, online data logging & visualisation  
  - Admin dashboard on M5Core 2  
  - Optional: blockchain logging for supervisor requirements  

- **Max**  
  - Facial-recognition ML on Raspberry Pi 5  
  - Thingy:52 sensor integration (temperature, air quality, RGB feedback)  
  - Base Node implementation on nRF52840 DK; serial comms to Pi 5  

- **Peter**  
  - Ultrasonic sensor and keypad integration on DiscoL475  
  - Door actuator control (servo or LED)  
  - Optional: NFC integration  

## System Architecture  

### Block Diagram  
> _Placeholder for block diagram image – insert `assets/block_diagram.png` or draw directly in README. Should show connections between: DiscoL475 (door node), Thingy:52, nRF52840 (base node), Raspberry Pi 5, M5Core 2, and MQTT broker._

### DIKW Pyramid Abstraction  
- **Data**: Raw sensor readings (temperature, air quality levels, ultrasonic distance, keypad entries, NFC IDs, facial features)  
- **Information**: Processed values (e.g. “Room temperature: 22 °C”, “Air quality index: 45”)  
- **Knowledge**: Patterns and correlations (e.g. “High occupant count leads to elevated CO₂ levels”, “Unrecognised faces trigger access denial”)  
- **Wisdom**: Automated actions and recommendations (e.g. “Lock door after 30 s of no presence”, “Notify admin if AQI > 100 for 10 min”)  

## System Integration  
All sensors and actuators communicate via the Base Node (nRF52840 DK):  
1. **Thingy:52** → BLE → Base Node → serial → Pi 5 / MQTT  
2. **DiscoL475 IoT** → BLE → Base Node → serial → Pi 5 / MQTT  
3. **Raspberry Pi 5** ↔ Serial → Base Node → BLE → M5Core 2  
4. **M5Core 2** → BLE → Base Node → real-time dashboard  

Full integration ensures that:  
- Door actuation only occurs after successful PIN, NFC or facial recognition  
- Environmental alerts affect LED indicators on Base Node and mobile/dashboard views  
- Entry/exit events are timestamp-logged online and optionally on blockchain  

## Wireless Network Communications  
- **BLE** between sensor/actuator nodes and Base Node (GATT profiles for temperature, air quality, door status)  
- **Serial (UART)** link between Base Node and Raspberry Pi 5 (frame format: `[HEADER][MSG_TYPE][LENGTH][PAYLOAD][CRC]`)  
- **MQTT** for cloud logging & remote dashboard (topics: `slarm/temperature`, `slarm/airquality`, `slarm/doorstatus`, `slarm/accesslog`)  
- **Message Protocol Diagram**  
  > _Placeholder: insert sequence diagram showing PIN/NFC/facial auth → Base Node request → Pi 5 verification → Base Node unlock command → actuator._

## Deliverables and Key Performance Indicators (KPIs)  
| KPI ID | Description                                               | Target                                                     |
| :----: | :-------------------------------------------------------- | :--------------------------------------------------------- |
| KPI 1   | Authentication success rate                              | ≥ 98% (correct PIN, NFC or facial recognition)            |
| KPI 2   | Average response time for door actuation                  | ≤ 200 ms from auth decision to actuator signal             |
| KPI 3   | BLE packet reliability                                    | ≥ 99.5% packet delivery rate between nodes                 |
| KPI 4   | Temperature/air-quality reporting latency                 | ≤ 1 s from sensor read to dashboard display                |
| KPI 5   | System uptime (excluding planned maintenance)             | ≥ 99.9% over 30 days                                      |
