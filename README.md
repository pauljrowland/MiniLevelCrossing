# 🚦 7.25" Gauge Level Crossing Control System (Arduino Nano)

A robust, fail-safe level crossing controller designed for a 7.25" gauge miniature railway. Built around an Arduino Nano, it provides automated and manual operation using treadle-based train detection and railway-style signalling logic.

---

## 🔧 Features

### 🚆 Train Detection
- Two treadles (A & B) detect train entry and exit
- Automatic bidirectional detection (A → B / B → A)
- Occupancy tracking within the crossing section

### 🚦 Signalling System
- Yellow warning phase on approach
- Red wig-wag flashing during occupancy
- Audible buzzer active during warning and active phases

### 🎛️ Operating Modes
- **AUTO mode**
  - Fully automated treadle-controlled operation
- **MANUAL mode**
  - Button-controlled activation
  - Same signalling sequence as AUTO mode
  - Hardware mode selection via input pin

### 🔘 Manual Control
- Toggle button (no hold required)
- First press → starts full crossing sequence
- Second press → safe shutdown and reset

### 🛡️ Fail-Safe Design
- Safe OFF state on startup
- Automatic reset after clearance
- Occupancy-based protection logic
- Debounced sensor and button inputs
- Prevents premature signal clearance

---

## ⚡ Hardware Overview

- Arduino Nano
- 2 × treadle sensors (active LOW inputs with internal pull-ups)
- 12V relay module (optically isolated)
- 12V power supply
- 5V buck converter for Arduino
- Yellow warning lamp
- Dual red wig-wag signal lamps
- Audible buzzer

---

## 🔌 Relay Logic

The system uses **active-HIGH relay control logic**:

- `HIGH = Relay ON`
- `LOW = Relay OFF`

Ensures safe default OFF state during boot and resets.
