# Robot Firmware — Combined

Unified firmware for the REDHAWK ESP32 robot. Covers all functional tests, a Wi-Fi web control panel for driver mode, and two autonomous modes (line following + maze speed run).

> **Course:** ECEGR 3210 — Embedded Systems & Design, Seattle University, March 2026
> **Author:** Ezekiel A. Mitchell

---

## Features at a Glance

| Feature | Description |
| --- | --- |
| 7 Functional Tests | Horn, LEDs, Light Sensor, Motors, Line Sensor, Ultrasonic, Completion |
| Driver Mode | Hold-to-drive web UI with speed slider and live sensor panel |
| Line Follow (Autonomous) | Single-sensor left-edge follower — BANN 201 circuit |
| Maze Solve (Autonomous) | Right-hand-rule with front ultrasonic — BANN 207 speed run |
| Serial Menu | Full test and mode control at 115200 baud |

---

## Prerequisites

| Requirement | Details |
| --- | --- |
| **PlatformIO** | VS Code extension or CLI from [platformio.org](https://platformio.org) |
| **ESP32-DevKitC** | Connected via USB |
| **arduino-esp32 v3.x** | Pulled in automatically by `espressif32@^6.0.0` in `platformio.ini` |
| **Assembled robot PCB** | Motors, LEDs, horn, and all sensors populated and wired |

---

## Project Structure

```text
PowerOn_Functional_Test/
├── platformio.ini        PlatformIO project config
└── src/
    ├── config.h          Pin assignments, PWM constants, autonomous tuning
    ├── web_control.h     Shared RobotMode enum + extern declarations
    ├── web_control.cpp   All HTTP route handlers and unified web UI
    └── main.cpp          Motor helpers, test functions, autonomous algorithms
```

---

## Hardware & Pin Map

| Signal | GPIO | Direction | Notes |
| --- | --- | --- | --- |
| Right Motor 1A | 18 | Output | L293D pin 2 — direction |
| Right Motor 2A | 19 | Output | L293D pin 7 — direction |
| Right Motor EN1 (PWM) | 26 | Output | L293D pin 1 — speed, LEDC ch 1 |
| Left Motor 3A | 16 | Output | L293D pin 10 — direction |
| Left Motor 4A | 17 | Output | L293D pin 15 — direction |
| Left Motor EN3 (PWM) | 25 | Output | L293D pin 9 — speed, LEDC ch 2 |
| Right Encoder A / B | 36, 39 | Input | Input-only GPIOs, INPUT_PULLUP |
| Left Encoder A / B | 34, 35 | Input | Input-only GPIOs, INPUT_PULLUP |
| Horn (piezo) | 21 | Output | PWM 1 kHz, 50% duty, LEDC ch 0 |
| Front LEDs | 2 | Output | Bootstrap pin — safe after boot |
| Rear LEDs | 0 | Output | Bootstrap pin — safe after boot |
| Light Sensor (LDR) | 33 | Analog In | ADC1_CH5, 12-bit (0–4095) |
| IR Line Sensor | 32 | Digital In | LOW = on black line (default) |
| Ultrasonic TRIG | 4 | Output | HC-SR04 trigger |
| Ultrasonic ECHO | 27 | Input | HC-SR04 echo |
| Status LED | 5 | Output | Solid = Wi-Fi connected |

---

## Configuration

All pin assignments and tuning constants are in `src/config.h`.

**Wi-Fi credentials:**

```cpp
#define WIFI_SSID "SU-ECE-LAB"
#define WIFI_PASS "FaraDay8086!"
```

**Autonomous tuning** (adjust to match your robot's physical characteristics):

```cpp
#define LINE_DETECT_LEVEL  0      // 0 = LOW over line, 1 = HIGH over line
#define SPEED_SLOW        140     // Motor duty 0-255
#define SPEED_MED         180
#define SPEED_FAST        220
#define SPEED_TURN        155
#define LF_STRAIGHT_MS     60     // Line follow: drive time while on-line
#define LF_SEARCH_MS      220     // Line follow: search turn time
#define WALL_STOP_CM       18     // Maze: obstacle threshold (cm)
#define TURN_90_MS        430     // Maze: pivot duration for ~90° at SPEED_TURN
#define WEB_WATCHDOG_MS   600     // Driver: stop if no web command in this window
```

---

## Build & Flash

```bash
pio run                          # Build only
pio run --target upload          # Build and flash
pio device monitor               # Open serial monitor (115200 baud)
```

Or use the PlatformIO toolbar in VS Code: **Build → Upload → Monitor**.

---

## Operating Modes

The firmware uses a mode state machine. Only one mode is active at a time. Any mode switch stops the motors first.

```text
MODE_IDLE        Motors stopped, waiting for input
MODE_MANUAL      Web/serial driver control, 600 ms watchdog
MODE_LINE_FOLLOW Autonomous line following algorithm
MODE_MAZE_SOLVE  Autonomous maze navigation algorithm
```

Switch modes via the **web UI** or the **serial menu** (`M / L / Z / S`).

---

## Web Control UI

After connecting to `SU-ECE-LAB`, the robot prints its IP address to serial:

```text
Connected → open http://192.168.x.x/ in your browser or phone.
```

The single-page UI has five sections:

### Mode Selection Panel

Four buttons: **Manual**, **Line Follow**, **Maze Solve**, **Stop**. Switching modes stops the motors immediately.

### Driver Controls Panel

Directional pad (Forward / Left / Stop / Right / Reverse) with a speed slider (80–255).
Hold a direction button to drive continuously — JavaScript sends a repeat request every 380 ms. Releasing the button sends a stop command. A 600 ms server-side watchdog also stops the motors if the connection drops.

### Quick Controls Panel

**Horn**, **LEDs On**, **LEDs Off** — available in any mode.

### Tests Panel

Individual buttons for Tests 1–6, plus **Run All Tests**. Triggering a test automatically switches to `MODE_IDLE` and stops the motors first.

> Tests 3 (Light Sensor) and 5 (Line Sensor) are blocking and take up to 10 seconds. The web server will not respond during this time.

### Live Sensors Panel

Polls `/sensors` every 1.8 seconds and displays:

- Current mode
- IR line sensor value and on/off-line status
- Light sensor ADC value
- Ultrasonic distance (cm)

---

## Serial Menu

Open the serial monitor at **115200 baud**. The menu is shown on boot and after every command:

```text
╔══════════════════════════════╗
║       Robot Serial Menu      ║
╠══════════════════════════════╣
║  1-6  Run individual test    ║
║  A    Run all tests          ║
║  M    Manual (web driver)    ║
║  L    Line-follow mode       ║
║  Z    Maze-solve mode        ║
║  S    Stop / idle            ║
╚══════════════════════════════╝
```

| Key | Action |
| --- | --- |
| `1` – `6` | Run the corresponding functional test |
| `A` | Clear results and run all tests in sequence |
| `M` | Switch to Manual driver mode |
| `L` | Start autonomous line-follow mode (2 beeps) |
| `Z` | Start autonomous maze-solve mode (3 beeps) |
| `S` | Stop all motion, return to idle |

---

## Functional Tests

All tests run in sequence via **Run All** or can be triggered individually. Results are stored in `testResults` and displayed on the web page.

### Test 1 — Horn

Emits 5 beeps (400 ms on / 300 ms off) via the piezo on GPIO 21.
**Pass:** 5 audible beeps, serial shows `PASS`.

### Test 2 — LEDs

Front/rear alternating (3 cycles × 400 ms), then all on (500 ms), then all off.
**Pass:** LEDs follow the sequence, serial shows `PASS`.

### Test 3 — Light Sensor *(requires interaction)*

Samples a 10-reading baseline, then monitors GPIO 33 for 10 seconds. Cover/uncover the photoresistor during the window. Both LED banks activate when the reading deviates from baseline by more than ±200 ADC counts.
**Pass:** ADC values change visibly, serial shows `[DARK]`/`[LIGHT]` labels and `PASS`.

### Test 4 — Motors

Left forward → left reverse → right forward → right reverse, 1 second each with 300 ms pauses.
**Pass:** Each motor spins the correct direction, serial shows `PASS`.

### Test 5 — Line Sensor *(requires interaction)*

Reads GPIO 32 and waits up to 10 seconds for any state change. Place or remove an object over the IR sensor during the window.
**Pass:** Serial shows `Transition detected - PASS`.

### Test 6 — Ultrasonic

Fires the HC-SR04 five times and prints distances to serial.
**Pass:** At least one reading returns a non-zero distance.

### Test 7 — Completion

15 fast beeps (80 ms on / 60 ms off) as an audible end-of-sequence fanfare.

---

## Autonomous Modes

### Line Follow — BANN 201

**Algorithm:** Single-sensor left-edge follower using the IR sensor on GPIO 32.

| Sensor state | Action |
| --- | --- |
| On line (LOW) | `curveRight` at `SPEED_MED` for `LF_STRAIGHT_MS` |
| Off line (HIGH) | `curveLeft` at `SPEED_MED` for `LF_SEARCH_MS` |

The robot tracks the left edge of the black line, producing a controlled zig-zag. The short step delays keep the web server responsive between iterations.

**To activate:** Press **Line Follow** on the web UI, or send `L` via serial.
**To stop:** Press **Stop** on the web UI, or send `S` via serial.

**Line follow tuning tips:**

- If the robot overshoots and loses the line: decrease `LF_SEARCH_MS`.
- If it barely curves: increase `LF_STRAIGHT_MS` or decrease `SPEED_MED`.
- If the sensor polarity is inverted on your hardware: change `LINE_DETECT_LEVEL` to `1`.

---

### Maze Solve — BANN 207

**Algorithm:** Right-hand-rule using the front HC-SR04 ultrasonic sensor.

```text
1. Drive forward at MAZE_SPEED.
2. Obstacle detected ≤ WALL_STOP_CM → stop, pivot right ~90°.
3. Recheck; if still blocked repeat step 2.
4. After 8 consecutive blocked turns → reverse 500 ms (stuck recovery), reset counter.
```

**To activate:** Press **Maze Solve** on the web UI, or send `Z` via serial.
**To stop:** Press **Stop** on the web UI, or send `S` via serial.

**Maze solve tuning tips:**

- Adjust `WALL_STOP_CM` to change how close the robot gets to a wall before turning.
- Adjust `TURN_90_MS` if the robot over- or under-rotates on turns (measured at `SPEED_TURN = 155`).
- Narrow corridors may need a shorter `WALL_STOP_CM` (e.g. `12`).

---

## Communication & Protocols

| Protocol | Interface | Pins | Purpose |
| --- | --- | --- | --- |
| UART / Serial | USB (built-in) | — | Debug output and serial menu at 115200 baud |
| Wi-Fi 802.11 b/g/n | RF | Antenna | Station mode — joins `SU-ECE-LAB` |
| HTTP / TCP port 80 | Over Wi-Fi | — | Web control UI, sensor endpoint, mode/drive/test routes |
| PWM (LEDC) | Hardware peripheral | 21, 25, 26 | Horn tone (1 kHz) and motor speed (5 kHz) |
| GPIO Digital | I/O | 0,2,4,16–19,27,32 | Motor direction, LEDs, ultrasonic TRIG/ECHO, IR sensor |
| ADC (12-bit SAR) | Hardware peripheral | 33 | Ambient light sensor (0–4095) |
| HC-SR04 Pulse | 2-wire custom | 4 (TRIG), 27 (ECHO) | Time-of-flight distance measurement |

---

## HTTP Routes

| Route | Method | Description |
| --- | --- | --- |
| `/` | GET | Full HTML control page |
| `/sensors` | GET | Live sensor snapshot (plain text) |
| `/drive/{fwd\|rev\|left\|right\|stop}?spd=N` | GET | Driver mode motor command |
| `/mode/{idle\|manual\|line\|maze}` | GET | Switch operating mode |
| `/test/{1–6\|all}` | GET | Trigger a functional test |
| `/horn` | GET | Single horn beep |
| `/leds/on` | GET | Turn both LED banks on |
| `/leds/off` | GET | Turn both LED banks off |

---

## Test Plan Coverage

| # | Requirement | Status | Notes |
| --- | --- | --- | --- |
| 1 | Horn — audible on/off | **PASS** | 5 beeps, 400 ms / 300 ms |
| 2 | LEDs — front/rear alternating, all on, all off | **PASS** | 3 alternating cycles then full on/off |
| 3 | Light sensor — ADC stream under dark/bright/normal | **PASS** | Readings every 500 ms, `[DARK]`/`[LIGHT]` labels |
| 4 | Motors — fwd/rev each motor independently | **PASS** | Left then right, 1 s each direction |
| 5 | Line sensor — detect line vs. background | **PASS** | Digital transition detected within 10 s window |
| 6 | Ultrasonic — distance readings change | **PASS** | 5 readings printed to serial |
| 7 | Serial menu — run each test independently | **PASS** | Keys `1–6`, `A`, `M`, `L`, `Z`, `S` |
| EC | Wi-Fi + web browser/phone control | **PASS** | Full web UI on port 80 |
| EC | Line Following — BANN 201 | **PASS** | Autonomous left-edge follower |
| EC | Maze Speed Run — BANN 207 | **PASS** | Autonomous right-hand-rule solver |

---

## Troubleshooting

| Symptom | Likely Cause |
| --- | --- |
| Upload fails | Wrong COM port; hold BOOT + tap EN to enter bootloader |
| `ledcSetup` compile errors | Wrong core version — confirm `espressif32@^6.0.0` |
| Horn silent | Piezo not on GPIO 21; LEDC channel conflict |
| Motors don't spin | Motor supply missing; L293D direction/enable wiring wrong |
| IR sensor never changes | GPIO 32 floating or sensor not wired; no interaction during window |
| ADC reads 0 or 4095 | LDR open/short circuit; R6 bias resistor missing |
| Robot spins in place (line follow) | `LINE_DETECT_LEVEL` has wrong polarity; increase `LF_SEARCH_MS` |
| Maze robot won't turn | `WALL_STOP_CM` too small for the environment; increase it |
| Wi-Fi timeout | Wrong SSID/password in `config.h`; network out of range |
| Web driver jittery | Network latency; widen `WEB_WATCHDOG_MS` slightly |
