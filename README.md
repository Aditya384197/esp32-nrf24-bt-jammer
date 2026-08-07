esp32-nrf24-bt-jammer
ESP32-based Bluetooth Classic Jammer using 3x nRF24L01+ modules. Fully ported to ESP‑IDF with serial command interface.


# 📡 ESP32 nRF24 Bluetooth Classic Jammer

[![Build Arduino Sketch (ESP32)](https://github.com/Aditya384197/esp32-nrf24-bt-jammer/actions/workflows/arduino-build.yml/badge.svg)](https://github.com/Aditya384197/esp32-nrf24-bt-jammer/actions/workflows/arduino-build.yml)

**An ESP32‑based Bluetooth Classic Jammer using 3x nRF24L01+ modules.**  
Fully optimized for maximum interference with aggressive channel hopping (135 µs) and 5 µs post‑write delays.  
**Single‑file Arduino sketch** – compiles with Arduino CLI and runs on GitHub Actions.

---

## 📋 Table of Contents

- [⚠️ Disclaimer](#️-disclaimer)
- [🚀 Features](#-features)
- [🧠 How It Works](#-how-it-works)
- [🔧 Hardware Requirements](#-hardware-requirements)
- [📡 Wiring Diagram](#-wiring-diagram)
- [🛠️ Building the Firmware](#️-building-the-firmware)
- [💻 Serial Commands](#-serial-commands)
- [🧪 Testing & Verification](#-testing--verification)
- [⚙️ Performance Tuning](#️-performance-tuning)
- [🧠 Technical Deep Dive](#-technical-deep-dive)
- [🐛 Troubleshooting](#-troubleshooting)
- [📂 Repository Structure](#-repository-structure)
- [📜 License](#-license)
- [🤝 Contributing](#-contributing)
- [⭐ Support](#-support)

---

## ⚠️ Disclaimer

**This tool is intended for educational and research purposes only.**

- Use it only on devices you own or have explicit written permission to test.
- Unauthorized jamming of Bluetooth signals is **illegal** in most countries and can result in severe penalties.
- The author is **not responsible** for any misuse, damage, or legal consequences arising from the use of this software.

**By using this software, you agree that you are solely responsible for your actions.**

---

## 🚀 Features

| # | Feature | Description |
| :---: | :--- | :--- |
| 1 | **3x nRF24L01+ Modules** | Wideband coverage using three independent radios. |
| 2 | **Aggressive Channel Hopping** | Switches channels every **135 µs** – 4× faster than BT Classic (625 µs). |
| 3 | **Three Channel Patterns** | Odd, even, and mixed – covering all 79 Bluetooth Classic channels. |
| 4 | **High‑Speed Transmission** | 2 Mbps data rate, max TX power, 32‑byte junk packets. |
| 5 | **Serial Command Interface** | Control via UART (115200 baud): `start`, `stop`, `status`, `toggle`, `help`. |
| 6 | **GitHub Actions CI** | Automatically builds on every push, provides `.bin` artifact. |
| 7 | **Single `.ino` File** | Easy to flash via Arduino IDE or `arduino-cli`. |
| 8 | **Optimized Timing** | 135 µs main delay + 5 µs post‑write delay for stability. |
| 9 | **SPI Mutex Protection** | Safe concurrent access to SPI bus. |
| 10 | **Task Watchdog Disabled** | Prevents premature task termination during heavy load. |

---

## 🧠 How It Works

### The Problem
Bluetooth Classic uses **79 channels** (0–78) and hops at **1600 hops/second** (every 625 µs). To effectively jam a BT connection, you must transmit a signal on the channel the BT device is using before it hops away.

### Our Solution
We use **three nRF24L01+** modules, each transmitting on a different channel pattern:

| Module | Channel Pattern | Description |
| :--- | :--- | :--- |
| **Radio A** | Odd channels (1, 3, 5, … 77) | Covers half the band |
| **Radio B** | Even channels (78, 76, 74, … 0) | Covers the other half in reverse order |
| **Radio C** | Mixed channels (40, 41, 42, … 47) | Provides additional randomness |

Every **~135 µs** (main delay) the channels are updated to the next set in the pattern tables, and a **32‑byte junk packet** is transmitted on each active radio. The total cycle time (including 5 µs delay after each radio) is about **150 µs**, which is **4× faster** than BT’s hop interval – ensuring that at least one packet reaches the BT device’s current channel most of the time.

### Why This Works
- **BT Classic** hops every 625 µs.
- **Our jammer** sends packets on **3 different channels every 150 µs**.
- This means we cover **all 79 channels** multiple times within a single BT hop interval.

---

## 🔧 Hardware Requirements

### Components

| Component | Quantity | Notes |
| :--- | :--- | :--- |
| ESP32 board (any variant) | 1 | Tested on ESP32‑DevKitC |
| nRF24L01+ module | 3 | With antenna for better range |
| Jumper wires | ~20 | For SPI and power connections |
| Breadboard (optional) | 1 | For prototyping |
| 3.3V 1A regulator (e.g., AMS1117) | 1 | **Highly recommended** for stable power |
| 10 µF + 0.1 µF capacitors | 3 each | For decoupling each nRF24 |

### Power Supply Warning
⚠️ **Three nRF24 modules at max power can draw over 100 mA.** The ESP32’s onboard 3.3V regulator (from USB) may not provide enough current, causing resets or erratic behavior.  
👉 **Always use an external 3.3V 1A regulator** to power the nRF24 modules. Connect the regulator’s output to the VCC pins of all modules, and connect GND to the ESP32’s GND.

### Antenna
- For best range, use nRF24 modules with **external antennas**.
- If using PCB‑trace antennas, keep them away from metal surfaces and other RF sources.

---

## 📡 Wiring Diagram

| nRF24L01+ Pin | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **Module A** | | |
| CE | GPIO 4 | Chip Enable |
| CSN | GPIO 5 | Chip Select |
| **Module B** | | |
| CE | GPIO 6 | |
| CSN | GPIO 7 | |
| **Module C** | | |
| CE | GPIO 8 | |
| CSN | GPIO 9 | |
| **All Modules** | | |
| VCC | 3.3V (external regulator) | **Not** ESP32’s 3.3V pin |
| GND | GND | Common ground |
| SCK | GPIO 18 | SPI Clock |
| MOSI | GPIO 23 | SPI Master Out |
| MISO | GPIO 19 | SPI Master In |

> ⚠️ **Important:** The SPI pins (SCK, MOSI, MISO) are shared by all three modules. Each module has its own CE and CSN pins.

---

## 🛠️ Building the Firmware

### Option 1 – Using GitHub Actions (Easiest)
1. Fork/clone this repository.
2. Push any change to the `main` branch – the GitHub Action will automatically build the sketch.
3. Go to the **Actions** tab, click on the latest workflow run, and download the `bt-jammer-firmware` artifact.
4. Extract the `.bin` file.

### Option 2 – Using Arduino IDE
1. Install the **ESP32 board** package (Tools → Board → Boards Manager → `esp32`).
2. Install the **RF24** library (Sketch → Include Library → Manage Libraries → `RF24`).
3. Open `esp32-nrf24-bt-jammer.ino` in Arduino IDE.
4. Select **ESP32 Dev Module** as the board.
5. Click **Upload**.

### Option 3 – Using Arduino CLI (Command Line)
```bash
# Install arduino-cli if not already installed
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Install ESP32 core and RF24 library
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.0.0
arduino-cli lib install "RF24"

# Compile
arduino-cli compile --fqbn esp32:esp32:esp32 --build-path build esp32-nrf24-bt-jammer.ino

# Flash (replace PORT with your device)
arduino-cli upload -p PORT --fqbn esp32:esp32:esp32 esp32-nrf24-bt-jammer.ino
