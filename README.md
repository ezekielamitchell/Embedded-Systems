# REDHAWK ESP32 Embedded Systems Robot

A custom PCB-based robot designed and built for the **Seattle University Embedded Systems & Design** course. The board is centered around an ESP32 microcontroller and includes motor control, sensing, GPS, and interaction peripherals using KiCad 9.0.

![PCB Layout](Mitchel_Ezekiel_Robot/docs/images/Screenshot%202026-02-26%20at%2011.30.25.png)

![PCB 3D Render — Top](Mitchel_Ezekiel_Robot/docs/images/Mitchel_Ezekiel_Robot.png)

![PCB 3D Render — Perspective](Mitchel_Ezekiel_Robot/docs/images/Mitchel_Ezekiel_Robot%201.png)

---

## Project Structure

```
Mitchel_Ezekiel_Robot/
├── Mitchel_Ezekiel_Robot.kicad_pcb                   # PCB layout
├── Mitchel_Ezekiel_Robot.kicad_sch                   # Root schematic
├── Mitchel_Ezekiel_Robot.kicad_pro                   # KiCad project file
├── Mitchel_Ezekiel_Robot.kicad_prl                   # PCB local settings
├── Ezekiel_Mitchell_ESP32_Robot_Schematic.kicad_pro  # Alternate schematic-only project
├── Ezekiel_Mitchell_ESP32_Robot_Schematic.kicad_prl
├── sym-lib-table                                     # Symbol library table
├── docs/
│   ├── ESP32EmeddedRobot_Design-GetStarted-v00-1-10-2025a.pdf
│   └── images/                                       # PCB renders and board photos
├── deliverables/
│   └── Ezekiel_Mitchell_ESP32_Robot_Schematic.kicad_sch
└── downloadables/
    ├── 0-My_Library.kicad_sym                        # Custom symbol library
    └── 0-My-Library.pretty/                          # Custom footprint library
```

---

## Hardware Overview

### Microcontroller
| Part | Description |
|------|-------------|
| ESP32-DEVKITC-32E (U1) | Main MCU — dual-core Xtensa LX6, Wi-Fi + Bluetooth |

### Motor Drive
| Part | Description |
|------|-------------|
| L293D (U3) | Dual H-bridge motor driver IC (DIP-16) |
| Pololu-3675 | Gear motors with quadrature encoders (×2) |
| Pololu-2691 / Ball-Caster-1 | Ball caster (rear support) |

### Sensors & I/O
| Part | Description |
|------|-------------|
| R7 — R_Photo | LDR photoresistor (light sensing) |
| D2 — LD274 | IR LED emitter (proximity sensing) |
| Q1 — SFH300 | IR phototransistor receiver |
| HORN1 — Buzzer | Audio feedback (TDK PS1240P02BT) |
| D1, D3–D6 — LED | Status indicator LEDs (×5, 3 mm) |
| SW1 — SW_SPST | Tactile on/off switch |

### Communication & Debug
| Part | Description |
|------|-------------|
| J2 — GPS MODULE | GPS module connector (UART: GPS_TX / GPS_RX) |
| J1 — JTAG | 1.27 mm 2×5 JTAG/SWD debug header |

### Connectors
| Ref | Value | Description |
|-----|-------|-------------|
| J3 | Barrel_Jack_Switch | Battery input (horizontal barrel jack) |
| J4 | Conn_01x06 | General 6-pin header |
| J5 | Conn_01x06 | General 6-pin header |
| J6 | Conn_01x04 | General 4-pin header |
| J7 | Conn_02x03_Odd_Even | Motor / encoder connector (2×3) |
| — | Conn_02x05_Odd_Even | 2×5 header |
| — | Conn_01x09 | 9-pin header |

### Passives & Power
| Ref | Value | Notes |
|-----|-------|-------|
| C1, C5–C7 | 0.1 µF | Decoupling capacitors |
| C2, C3 | 47 µF | Bulk filter capacitors |
| R1 | 1 kΩ | Pull-up / current limiting |
| R2 | 1 kΩ | Pull-up / current limiting |
| R3 | 47 kΩ | Pull-up for IR receiver circuit |
| R4, R5 | 330 Ω | LED current limiting |
| R6 | 10 kΩ | Voltage divider (LDR / sensor bias) |

**Power rails:** `+BATT` (battery input), `+3V3` (regulated 3.3 V from ESP32 module)

---

## PCB Net Classes

| Net Class | Track Width | Via Diameter | Via Drill | Notes |
|-----------|-------------|--------------|-----------|-------|
| Power | 0.5 mm | 1.0 mm | 0.5 mm | VCC, GND rails |
| Motor | 0.8 mm | 1.2 mm | 0.6 mm | H-bridge output lines |
| High Speed / GPS | 0.2 mm | 0.8 mm | 0.4 mm | GPS UART, high-frequency signals |
| Default | 0.2 mm | 0.8 mm | 0.3 mm | General signals |

---

## Tools & Libraries

- **KiCad 9.0** — schematic capture and PCB layout
- **Custom Symbol Library** — `0-My_Library.kicad_sym`
- **Custom Footprint Library** — `0-My-Library.pretty/`
  - `MODULE_ESP32-DEVKITC-32E`
  - `Pololu-3675-GearMotorWEncoder`
  - `Pololu-2691-Ball_Caster` / `Ball-Caster-1`
  - `Basic-Robot-Outline_1`
  - ECE capacitive and piano touch switch footprints
  - `BAT_2481`, `TRIM_PTA3043-2015DPA104`, and more

---

## Getting Started

1. Install [KiCad 9.0](https://www.kicad.org/)
2. Clone this repository
3. Open `Mitchel_Ezekiel_Robot/Mitchel_Ezekiel_Robot.kicad_pro` in KiCad
4. To use the custom libraries, add `downloadables/0-My-Library.pretty` as a footprint library and `downloadables/0-My_Library.kicad_sym` as a symbol library in KiCad's library manager
5. See `docs/ESP32EmeddedRobot_Design-GetStarted-v00-1-10-2025a.pdf` for the original design brief

---

## Author

**Ezekiel Mitchell** — Seattle University, Embedded Systems & Design
