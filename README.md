# SecurIoT-Quantum 🛡️🤖

[![ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://expressif.com)
[![Framework-Arduino](https://img.shields.io/badge/Framework-Arduino-orange.svg)](https://arduino.cc)
[![Post-Quantum-Crypto](https://img.shields.io/badge/Cryptography-ML--KEM--768%20%2F%20Kyber-darkviolet.svg)](https://csrc.nist.gov)
[![License-MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

An AI-powered Post-Quantum Cryptography (PQC) security shield library for the ESP32. This package protects resource-constrained IoT devices from network-level attacks (DDoS, packet rate flooding, out-of-order fragmentation floods) and timing side-channel analysis, while establishing secure post-quantum connections using a portable C Kyber-768 implementation.

---

## 🚀 Key Features

* **Post-Quantum Cryptography (ML-KEM / Kyber-768):** Ephemeral post-quantum key exchanges completed in **9 ms** on the 240 MHz dual-core ESP32 chip.
* **TinyML Threat Guard Classifier:** A nested C++ decision tree classifier executing in **<10 microseconds** with **<1 KB SRAM** footprint. No heavy dependencies (e.g. TensorFlow Lite Micro) required.
* **Sliding Window Statistics:** Dynamically aggregates packet rate, inter-packet gap (IPG), CPU temperature delta, and heap depletion rate over 1-second intervals.
* **Active Threat Mitigation:**
  * **Timing Noise Injection:** Injects random busy-delays (5ms–25ms) under suspicious (`RISK_MEDIUM`) levels to prevent cryptographic execution side-channel attacks.
  * **Active Drop & Lockout:** Automatically drops packets and locks out attackers' IP addresses under critical (`RISK_HIGH`) threat levels.
  * **Hardware Alarm Triggering:** Pulls custom GPIO pins (such as onboard LEDs on GPIO 2 or 5) `HIGH` during active attacks.

---

## 📂 Repository Structure

```
securiot-quantum/
├── include/
│   ├── securiot_quantum.h      # Main public API header
│   ├── pqc_engine.h            # Kyber-768 key exchange interface
│   ├── ml_guard.h              # TinyML classifier definitions
│   └── defense_engine.h        # Active mitigation & GPIO mapping
├── src/
│   ├── pqc/                    # Core CRYSTALS-Kyber implementation files
│   ├── pqc_engine.c            # Portable C keypair, encapsulation, decapsulation
│   ├── ml_guard.cpp            # Nested decision-tree classification logic
│   ├── defense_engine.c        # IP tracking table, timing noise, alarm pins
│   └── securiot_quantum.c      # Sliding-window telemetry metrics accumulator
├── tools/
│   ├── interactive_test.py     # Python CLI console to send commands & UDP packets
│   └── train_guard_model.py    # Python Scikit-Learn training script
└── examples/
    └── secure_node/
        └── secure_node.ino     # Interactive Arduino/ESP32 firmware example
```

---

## 🛠️ Getting Started

### 1. Hardware Setup
* Connect any standard dual-core ESP32 board (e.g., NodeMCU-32S, ESP32 DevKit v1) to your computer's USB port.
* The onboard status LED (typically connected to **GPIO 2** or **GPIO 5**) will act as the alarm indicator.

### 2. Wi-Fi Configuration
Open `examples/secure_node/secure_node.ino` and update the Wi-Fi credentials to match your local network:
```cpp
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
```

### 3. Deploying Firmware
Compile and upload the sketch to your ESP32. 
> [!IMPORTANT]
> **Stack Size Note:** Kyber-768 allocates large polynomial vector arrays. To avoid FreeRTOS stack overflow, the cryptographic operations must be executed inside a custom task with at least **24 KB of stack** (as shown in the `setup()` function of the example sketch).

---

## 🎮 Interactive Verification & Pentesting

We provide a Python interactive console tool to test the shield.

1. Install `pyserial`:
   ```bash
   pip install pyserial
   ```
2. Run the console:
   ```bash
   python3 tools/interactive_test.py
   ```
3. Watch the serial log outputs as you trigger tests:
   * **`[1]`** performs a live **Kyber-768 PQC Key Exchange** on-chip.
   * **`[6]`** sends a single UDP packet over Wi-Fi.
   * **`[7]`** triggers a **UDP Rate-Limiting Flood** (15 packets/100ms). The first 5 are allowed; the rest are actively blocked.
   * **`[8]`** triggers a **UDP DDoS Fragmentation Flood** (mismatched fragments). The TinyML classifier flags this as `RISK_HIGH`, blocks the flood, and **lights up the onboard LED**.
   * **`[5]`** resets the alarm LED.

---

## 📈 Wireshark Inspection

To monitor the active blocking on your network interface, start a capture on your Wi-Fi interface (e.g. `wlan0`) with the display filter:
```text
udp.port == 8080
```
* **Normal Telemetry:** You will see a `Len=32` request followed by a `Len=7` response containing `ALLOWED`.
* **Rate-Limit Attack:** You will see 15 UDP request packets, but the ESP32 will only send back 5 response packets before shutting down the connection for the remainder of the window.
* **DDoS Flood:** You will see 20 incoming mismatched packets, but **zero response packets** will be returned as the ESP32 drops them at the threat-barrier layer.

---

## 📄 License
This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
