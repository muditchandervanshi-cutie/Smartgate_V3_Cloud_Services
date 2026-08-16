# Smart Gate V3 — Cloud-Connected Access Control System

A cloud-connected smart gate and access-control system built using an ESP32, RFID authentication, sensors, servo control, Wi-Fi, and Firebase Realtime Database.

Smart Gate V3 builds upon the earlier versions of the project by adding cloud connectivity, authentication, access logging, system heartbeat monitoring, remote gate control, and remote system lockdown.

---

Demo Video + Photos 

https://drive.google.com/drive/folders/1Ut0r61W8mJfhidZGksEiZV_YINtIQAzK

(Demonstration Note: During the front-view recording, the HC-SR04 continuously detected a visitor because I was positioned directly in front of the sensor to obtain a clear view of the system. A separate side-view demonstration is provided to show normal operation when the sensor is not continuously obstructed.) 

---

## 📌 Project Overview

Smart Gate V3 is an embedded access-control system designed to demonstrate how hardware-based authentication can be combined with cloud services.

The system uses an ESP32 as the main controller. A visitor is detected using distance and IR sensing, after which an RFID card can be scanned for authentication.

Depending on the registered card, access is either granted or denied.

The system also communicates with Firebase over Wi-Fi to provide:

- Access event logging
- System heartbeat/status monitoring
- Remote gate control
- Remote system lockdown
- Cloud-based system status

---

## ✨ Features

### 🔐 RFID-Based Access Control

The system uses an MFRC522 RFID reader to identify registered cards.

Cards are assigned roles such as:

- Owner
- Admin
- Member

Unregistered cards are rejected and recorded as denied access events.

---

### 🚶 Visitor Detection

The gate uses sensors before requesting an RFID scan.

The control sequence is:

```

IDLE
  ↓
Visitor Detected
  ↓
IR Detection
  ↓
Scan RFID Card
  ↓
Authentication
  ├── Access Granted
  │      ↓
  │   Gate Opens
  │      ↓
  │   Gate Closes
  │
  └── Access Denied

🔧 Adjustable IR Detection Threshold

The front panel includes a small access opening aligned with the Flying Fish IR sensor's potentiometer. This allows the IR detection threshold to be adjusted externally without removing or disassembling the front panel.

The potentiometer controls the sensor's sensitivity/detection range, allowing the system to be tuned according to the physical placement of the sensor and the required distance at which the RFID scanning stage should be activated.

This provides a simple form of hardware-level calibration, making the visitor-detection and RFID activation point adjustable to suit different physical configurations.



🚪 Servo-Controlled Gate

An SG90 servo motor controls the physical gate mechanism.

The gate uses an animated movement sequence when access is granted.

🖥️ LCD Feedback

A 16×2 I²C LCD provides local system feedback, including messages such as:

System Ready
Visitor Detected
Scan Your Card
Access Granted
Access Denied
Gate Opening
Closing Gate
System Locked

Different user roles can also receive different LCD messages.

💡 LED Status Indicators

Three LEDs provide visual system feedback:

🔴 Red — authentication/locked states
🟢 Green — system ready/access granted
🔵 Blue — connection, processing, or gate operation states
☁️ Cloud Integration

Smart Gate V3 connects to Firebase Realtime Database through Wi-Fi.

The ESP32 uses Firebase for three major functions:

1. Access Logs

Every RFID access attempt is recorded in Firebase.

Each event contains:

name
role
granted
date
time
gate

The events are stored under:

AccessLogs

Each event receives a unique Firebase push ID.

The date and time are stored as fields within each event.

Example structure:

AccessLogs
├── unique_push_id
│   ├── name
│   ├── role
│   ├── granted
│   ├── date
│   ├── time
│   └── gate
│
├── unique_push_id
│   └── ...
│
└── unique_push_id
    └── ...

This allows the dashboard or another application to group and display events according to date without requiring Firebase itself to use date-based node names.

2. System Heartbeat

The ESP32 periodically updates a system-status record in Firebase.

The heartbeat contains information including:

wifiConnected
wifiRSSI
ipAddress
lastHeartbeat

The heartbeat is updated approximately every 10 seconds.

This provides a simple way to monitor whether the system is connected and communicating with the cloud service.

3. Remote Control

Firebase is also used to receive commands from a remote interface.

The system supports:

Remote Gate Opening
Remote System Enable/Disable

A remote gate-opening command can cause the ESP32 to enter the gate-opening sequence.

The system also supports an emergency/administrative lockdown state.

When the system is disabled remotely:

SYSTEM LOCKED
Contact Admin

is displayed locally and normal access processing is stopped.

Remote gate-opening commands are rejected while the system is in lockdown.

🔑 Firebase Authentication

Firebase Anonymous Authentication is used by the ESP32 to authenticate with the Firebase backend.

The ESP32 performs an anonymous sign-up during initialization before interacting with the database.

The ESP32 uses Firebase Anonymous Authentication rather than storing a conventional user password for device authentication.

🕐 Time Synchronization

The ESP32 synchronizes its clock using NTP.

The configured time zone offset is:

UTC +05:30

Access events record:

Date: DD-MM-YYYY
Time: HH:MM:SS

This allows access events to carry a timestamp that can be displayed in the access log.

🧠 Program Architecture

The access-control logic is implemented as a state machine.

The primary states are:

IDLE
VISITOR_DETECTED
SCAN_CARD
GRANTED
DENIED
State flow
        ┌──────────────────┐
        │       IDLE       │
        └────────┬─────────┘
                 │
                 │ Visitor detected
                 ▼
        ┌──────────────────┐
        │ VISITOR_DETECTED │
        └────────┬─────────┘
                 │
                 │ IR detected
                 ▼
        ┌──────────────────┐
        │    SCAN_CARD     │
        └───────┬────┬─────┘
                │    │
          Valid │    │ Invalid
                │    │
                ▼    ▼
        ┌──────────┐ ┌─────────┐
        │ GRANTED  │ │ DENIED  │
        └────┬─────┘ └────┬────┘
             │            │
             ▼            ▼
        Open/Close       Reset
          Gate           State
             │
             ▼
            IDLE
🔧 Hardware

The project uses:

Component	                  Purpose
ESP32	                          Main controller and Wi-Fi connectivity
MFRC522 RFID Reader               RFID card authentication
RFID Cards/Tags	                  User identification
HC-SR04 Ultrasonic Sensor	  Visitor detection
IR Sensor	                  Secondary visitor/card-scan detection
SG90 Servo	                  Gate mechanism
16×2 I²C LCD	                  Local status display
Red LED                     	  Status indication
Green LED	                  Ready/granted indication
Blue LED	                  Processing/connection indication
Breadboard & Jumper Wires	  Prototyping


💻 Software & Libraries

The project was developed using the Arduino environment for ESP32.

Major libraries used include:

WiFi.h
FirebaseESP32.h
MFRC522.h
ESP32Servo.h
LiquidCrystal_I2C.h
Wire.h
SPI.h

Firebase helper components used by the project include:

TokenHelper.h
RTDBHelper.h


🔌 Pin Configuration
Component	  ESP32 GPIO
RFID SS	          GPIO 4
RFID RST	  GPIO 2
RFID SCK	  GPIO 18
RFID MOSI	  GPIO 23
RFID MISO	  GPIO 19
Ultrasonic TRIG   GPIO 5
Ultrasonic ECHO	  GPIO 17
IR Sensor	  GPIO 27
Red LED	          GPIO 32
Green LED	  GPIO 25
Blue LED	  GPIO 26
Servo	          GPIO 13
I²C LCD SDA	  GPIO 21
I²C LCD SCL	  GPIO 22


🔒 Security & Configuration

Credentials and private configuration values are intentionally not included in this repository.

The published source uses placeholders such as:

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"


#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"

Before running the project, these values must be replaced with the appropriate local configuration.

Real RFID card UIDs have also been replaced with example values in the public version of the source code.

Never commit Wi-Fi passwords, private keys, API tokens, or other credentials to a public repository.

```
🧪 Demonstration

A demonstration of Smart Gate V3 is available here:

▶️ Watch the Smart Gate V3 Demonstration

(https://drive.google.com/file/d/1yP2I0FTglJdkVnOfY3EcxiU7r78lYAd7/view?usp=drive_link)

The demonstration shows the system operating with its embedded hardware and cloud-connected features..
```
📈 Project Evolution

Smart Gate V3 is part of an ongoing project series.

V1

The initial prototype established the basic smart-gate concept and hardware control.

V2

The project was expanded with additional sensing, RFID-based access control, display and indicator systems, and improved gate-control functionality.

V3

The system was extended into a cloud-connected IoT access-control system with:

Wi-Fi connectivity
Firebase integration
Anonymous authentication
Access logging
NTP time synchronization
System heartbeat
Remote gate control
Remote system lockdown
Cloud-based system monitoring
🎯 Learning Outcomes

This project provided hands-on experience with:

ESP32 embedded programming
RFID authentication
Sensor integration
Servo motor control
I²C communication
SPI communication
Wi-Fi networking
Firebase Realtime Database
Anonymous authentication
NTP time synchronization
Cloud-connected IoT systems
State-machine design
Remote device control
Event logging
Basic IoT security practices
🚀 Future Improvements

Possible future improvements include:

A dedicated web dashboard
Improved date-based access-log visualization
More granular user management
Better remote authentication and authorization
Persistent configuration management
Improved error handling
Hardware enclosure and PCB implementation
Additional sensors
Camera-based verification
More advanced access analytics
📜 Project Status

Smart Gate V3 — Working Prototype

This project is an educational/hands-on embedded IoT prototype and is being developed incrementally through multiple versions.

👨‍💻 Author

Mudit Chander Vanshi

Electronics & Communication Engineering Student

This project represents an ongoing exploration of embedded systems, IoT, cloud services, and hardware-software integration.
