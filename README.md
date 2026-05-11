# 7.25" Gauge Level Crossing Control System based on Arduino Nano

Level crossing controller designed for a 7.25" gauge miniature railway.
Built around an Arduino Nano, it provides automated and manual operation using treadle-based train detection and railway-style signalling logic.

---

## Features

### Train Detection
- Two treadles (A & B) detect train entry and exit
- Automatic bidirectional detection (A → B / B → A)
- Occupancy tracking within the crossing section

### The Signalling System
- Yellow warning phase on approach
- Red wig-wag flashing during occupancy
- Yodalarm active during warning and active phases

### Operating Modes
- **AUTO mode**
  - Fully automated treadle-controlled operation
  - Manual button can stop the crossing in the case of an error
- **MANUAL mode**
  - Button-controlled activation
  - Disables the treadle input
  - First press → starts full crossing sequence
  - Second press → shutdown and reset
---

## Hardware

- Arduino Nano
- 2 × treadle sensors
- 12V relay module
- 12V power supply
- 5V buck converter for Arduino
- Yellow warning lamps
- Red wig-wag signal lamps
- Yodalarm for sound
