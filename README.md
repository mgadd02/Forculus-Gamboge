# Forculus-Gamboge
4011 Team Project. Max, Peter, Corey.

# SLARM: Smart Lock and Room Monitoring System

## Project Description  
The SLARM system integrates door access control with environmental monitoring to provide a secure, data-driven solution for room management. A network of IoT boards handles user authentication, sensor data collection, local and remote dashboards, and actuators (LED or servo) to lock or unlock doors. Australian English spelling and punctuation are used throughout.

INTENDED FOR TEMPERATURE SENSITIVE ENVIRONMENTS SUCH AS COLD ROOMS FOR FOOD STORAGE, OR SERVER/COMPUTE ROOMS.

The SLARM system uses a Zephyr-based mesh of IoT nodes to control secure access and monitor room conditions: a DiscoL475 board at the door handles PIN-Code, NFC and facial-based authentication and drives a servo (or LED) to lock/unlock; an inward-facing ultrasonic sensor automatically opens the door when someone leaves to prevent entrapment; a Thingy:52 node measures temperature and air quality; and a Raspberry Pi 5 with Arducam/ESP32-CAM runs a CNN for facial recognition. All boards communicate via BLE through an nRF52840 base node, which relays data over UART/MQTT to an InfluxDB–Grafana dashboard and the M5Core 2 admin display.

## Hardware Components  
- **Primary Door Node (DiscoL475 IoT Discovery Board)**  
  - Keypad for PIN entry
  - NFC reader/writer and NFC card for authentication (IF REQUIRED)
  - Ultrasonic sensor for door‐presence detection  
  - Actuator: LED indicator (base node) or servo motor (door node, IF REQUIRED)  

- **Base Node (nRF52840 DK)**  
  - Central BLE gateway for Thingy:52 and DiscoL475 and the M5 Core2 Admin Display
  - Serial link to Raspberry Pi 5 running Zephyr Shell Commands ETC for info transfer and logging.
  - Status LEDs for locked/unlocked and temperature warnings  

- **Admin Dashboard Display (M5Core 2)**  
  - BLE client displaying real-time sensor readings, door status, entry/exit logs  

- **Environmental Sensor Node (Thingy:52)**  
  - Temperature and air-quality sensing  
  - RGB LED to reflect air-quality status  
  - BLE transmitter to Base Node  

- **Facial Recognition Node (Raspberry Pi 5 4 GB + Arducam / ESP32-CAM)**  
  - Convolutional Neural Network (CNN) for face recognition  
  - Serial communication to Base Node  

- **Development & Communication**  
  - MQTT broker (online dashboard & data logging)
  - Graphana Dashboard, InfluxDB, online hosted, using PC/RPI-5 for logging and remote monitoring.
  - Optional blockchain ledger for immutable access logs (IF REQUIRED) 

## Team Allocation  
- **Corey** - 47450020
  - MQTT infrastructure, online data logging & visualisation Graphana and InfluxDB  
  - Admin dashboard on M5Core 2  
  - Optional: blockchain logging as per supervisor recommendation 

- **Max** - 46985431
  - Facial-recognition ML on Raspberry Pi 5  
  - Thingy:52 sensor integration (temperature, air quality, RGB feedback)  
  - Base Node implementation on nRF52840 DK; serial comms to Pi 5  

- **Peter** - 49040760
  - Ultrasonic sensor and keypad integration on DiscoL475  
  - Optional: NFC integration  (AS REQUIRED BY TUTOR)
  - Bluetooth data emitting to base node.

## System Architecture  

### Block Diagram  
![Block Diagram](/assets/block_diagram.png)

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
![image](https://github.com/user-attachments/assets/f1a748b1-dedd-46c3-babf-83043bfb1249)


## Deliverables and Key Performance Indicators (KPIs)  
| KPI ID  | Description                                               | Target                                                     |
| :----:  | :-------------------------------------------------------- | :--------------------------------------------------------- |
| KPI 1   | Authentication success rate                               | ≥ 90% (facial recognition alongside correct PIN and/or NFC |
| KPI 2   | Average response time for door actuation                  | ≤ 1 s from auth decision to actuator signal                |
| KPI 3   | BLE packet reliability                                    | ≥ Good packet delivery rate between nodes                  |
| KPI 4   | Temperature/air-quality reporting latency                 | ≤ 1 s from sensor read to dashboard display                |
| KPI 5   | System uptime (excluding planned maintenance)             | ≥ Good system reliability                                  |

## Sensors List
Ultrasonic distance sensor
* DiscoL475 IoT Discovery Board
* Detects presence at the door (exit/entry)

Hall-effect sensor (optional)
* DiscoL475 IoT Discovery Board
* Alternative door-position detection

Keypad
* DiscoL475 IoT Discovery Board
* PIN entry for authentication

NFC reader/writer (optional)
* DiscoL475 IoT Discovery Board
* Reads NFC cards for authentication

Temperature sensor
* Thingy:52
* Monitors room temperature

Air-quality sensor
* Thingy:52
* Measures CO₂, VOCs, etc.

Camera module (Arducam / ESP32-CAM)
* Raspberry Pi 5
* Captures images for CNN facial recognition
