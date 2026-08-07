# esp32-nrf24-bt-jammer
ESP32-based Bluetooth Classic Jammer using 3x nRF24L01+ modules. Fully ported to ESP‑IDF with serial command interface.

# 🛡️ ESP32 Bluetooth Classic Jammer (Single‑File Arduino)

[![Build Arduino Sketch](https://github.com/आपका-यूजरनेम/esp32-bt-classic-jammer-arduino/actions/workflows/arduino-build.yml/badge.svg)](https://github.com/आपका-यूजरनेम/esp32-bt-classic-jammer-arduino/actions/workflows/arduino-build.yml)

**Single‑file Arduino sketch** for a Bluetooth Classic Jammer using **3x nRF24L01+** modules.  
Fully compatible with Arduino IDE and GitHub Actions (Arduino CLI).

> ⚠️ **Educational use only.** Do not use to disrupt legitimate communications.

---

## 🚀 Features

- **Single `.ino` file** – easy to flash via Arduino IDE.
- **3x nRF24L01+** modules for wideband jamming.
- **Aggressive channel hopping** across all 79 BT channels.
- **Serial command interface** – `start`, `stop`, `status`, `toggle`, `help`.
- **GitHub Actions CI** – builds automatically on every push.

---

## 🔧 Wiring

| nRF24L01+ | ESP32 Pin |
| :--- | :--- |
| **Module A** | |
| CE | GPIO 4 |
| CSN | GPIO 5 |
| **Module B** | |
| CE | GPIO 6 |
| CSN | GPIO 7 |
| **Module C** | |
| CE | GPIO 8 |
| CSN | GPIO 9 |
| **All** | |
| VCC | 3.3V |
| GND | GND |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |

---

## 🛠️ Building

### Arduino IDE
1. Install **ESP32 board** (Tools → Board → Boards Manager → `esp32`).
2. Install **RF24** library (Sketch → Include Library → Manage Libraries → `RF24`).
3. Open `bt_jammer.ino`, select board `ESP32 Dev Module`, and click **Upload**.

### GitHub Actions (CI)
Just push to GitHub – the workflow will build and provide a `.bin` artifact.

---

## 💻 Serial Commands

| Command | Action |
| :--- | :--- |
| `start`  | Start jamming |
| `stop`   | Stop jamming |
| `status` | Show current state |
| `toggle` | Toggle on/off |
| `help`   | Show commands |

---

## 📜 License

MIT – free to use and modify for educational purposes.
