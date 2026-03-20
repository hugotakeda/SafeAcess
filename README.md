# SafeAcess

**Intelligent IoT Access Control with Facial Recognition.**  

Academic Project developed in the **4th period: First semester of 2025 (2025/1)**.  
This system integrates ESP32-CAM hardware with a Python backend to provide real-time biometric validation and automated security alerts.

## Technical Overview

| Component | Responsibility |
|---|---|
| **ESP32-CAM** | Image capture and motion sensing (PIR) |
| **Python API** | Biometric processing and face recognition |
| **Discord Bot** | Real-time notifications and image logs |
| **OLED/Relay** | Local feedback and physical lock control |

## Key Features

- **Biometric Validation:** High-accuracy facial recognition using dlib and OpenCV.
- **Real-time Alerts:** Immediate Discord notifications with snapshots on every access attempt.
- **Physical Automation:** Automated relay triggering for electronic door locks.
- **Hardware Feedback:** Visual status updates via I2C SSD1306 OLED display.

## System Architecture

```text
.
├── AccessControl/      # ESP32-CAM Firmware (Arduino)
├── cloud_bridge.py     # Python Backend & Discord Integration
├── templates/          # Web interface assets
└── known_faces/        # Authorized biometric database
```

## Quick Start

1. **Backend:** Populate `known_faces/`, update your Discord Webhook in `cloud_bridge.py`, and run the server.
2. **Firmware:** Configure Wi-Fi and API endpoint in `AccessControl.ino` and upload to ESP32.
3. **Hardware:** Connect PIR sensor (capture trigger) and Relay (door lock) to the ESP32 pins.

---

[MIT](./LICENSE) © 2025 hugotakeda
