# Power-On Functional Test

Firmware that exercises every major peripheral on the REDHAWK ESP32 robot board in a fixed, sequential test sequence. Flash it once, watch the board run through all tests, and check the serial monitor for pass/fail output.

All tests run inside `setup()`. `loop()` is intentionally empty — the firmware is a one-shot diagnostic, not an application.

---

## Prerequisites

| Requirement | Details |
|---|---|
| **PlatformIO** | Install via [platformio.org](https://platformio.org) or the VS Code extension |
| **ESP32-DevKitC** board | Connected via USB |
| **Arduino-ESP32 core v3.x** | Pulled in automatically by `espressif32@^6.0.0` in `platformio.ini` |
| **Fully-assembled robot PCB** | Motors, LEDs, horn, and sensors must be populated and wired |

---

## Project Structure

```
PowerOn_Functional_Test/
├── platformio.ini    # PlatformIO project config
└── src/
    └── main.cpp      # All test firmware
```

---

## Hardware & Pin Map

| Signal | GPIO | Notes |
|---|---|---|
| Right Motor 1A | 18 | L293D pin 2 — direction |
| Right Motor 2A | 19 | L293D pin 7 — direction |
| Right Motor EN1 (PWM) | 26 | L293D pin 1 — speed |
| Left Motor 3A | 16 | L293D pin 10 — direction |
| Left Motor 4A | 17 | L293D pin 15 — direction |
| Left Motor EN3 (PWM) | 25 | L293D pin 9 — speed |
| Right Encoder A / B | 36, 39 | Input-only GPIOs, INPUT_PULLUP |
| Left Encoder A / B | 34, 35 | Input-only GPIOs, INPUT_PULLUP |
| Horn (piezo) | 21 | PWM — 1 kHz, 50% duty |
| Front LEDs | 2 | Bootstrap pin — safe as output after boot |
| Rear LEDs | 0 | Bootstrap pin — safe as output after boot |
| Photoresistor | 33 | ADC1_CH5 — analog input, 12-bit (0–4095) |
| IR Line Sensor | 32 | Digital input |
| Ultrasonic TRIG | 4 | Output (held LOW, not tested by firmware) |
| Ultrasonic ECHO | 27 | Input (not tested by firmware) |
| Status LED | 5 | Green LED — used during Wi-Fi connection phase |

---

## Configuration

Before flashing, open `src/main.cpp` and update the Wi-Fi credentials near the top of the file:

```cpp
const char* WIFI_SSID = "your-network-name";
const char* WIFI_PASS = "your-password";
```

The Wi-Fi connection is non-critical to the hardware tests — if it times out, the board still completes the full test sequence.

---

## Build & Flash

```bash
# Build
pio run

# Build and upload
pio run --target upload

# Open serial monitor (115200 baud)
pio device monitor
```

Or use the PlatformIO toolbar buttons in VS Code: **Build → Upload → Monitor**.

---

## Test Sequence

The firmware runs 7 tests in order, printing status to the serial monitor at 115200 baud. Tests 6 and 7 require brief manual interaction — see the instructions below.

### Test 1 — Horn
**What it does:** Emits 5 slow beeps (400 ms on / 300 ms off) via the piezo horn on GPIO 21.

**Pass criteria:** You hear 5 audible beeps. Serial output shows `PASS`.

**Fail indicators:** Silence; check that the piezo is connected to GPIO 21 and that the PWM channel is correctly attached.

---

### Test 2 — Front LEDs
**What it does:** Blinks the front LED network (GPIO 2) 5 times (300 ms on / 200 ms off).

**Pass criteria:** Front LEDs flash 5 times. Serial output shows `PASS`.

---

### Test 3 — Rear LEDs
**What it does:** Blinks the rear LED network (GPIO 0) 5 times (300 ms on / 200 ms off).

**Pass criteria:** Rear LEDs flash 5 times. Serial output shows `PASS`.

> **Note:** GPIO 0 and GPIO 2 are ESP32 bootstrap pins. The firmware configures them as outputs **after** boot completes, so they are safe to drive here.

---

### Test 4 — Left Motor
**What it does:** Drives the left motor forward at 100% duty cycle for 1 second, then stops.

- Sets 3A = HIGH, 4A = LOW (forward direction)
- Writes full duty to EN3 PWM channel (GPIO 25)
- After 1 s, cuts PWM and sets direction pins LOW

**Pass criteria:** Left wheel spins forward for ~1 second. Serial output shows `PASS`.

**Fail indicators:** No movement — verify L293D wiring on pins 10, 15, and 9; check motor supply voltage.

---

### Test 5 — Right Motor
**What it does:** Drives the right motor forward at 100% duty cycle for 1 second, then stops.

- Sets 1A = HIGH, 2A = LOW (forward direction)
- Writes full duty to EN1 PWM channel (GPIO 26)
- After 1 s, cuts PWM and sets direction pins LOW

**Pass criteria:** Right wheel spins forward for ~1 second. Serial output shows `PASS`.

---

### Test 6 — IR / Line Sensor *(user interaction required)*
**What it does:** Reads the digital IR line sensor (GPIO 32) and waits up to **10 seconds** for the signal to change state.

**Sequence:**
1. Two short beeps signal the start of the test window.
2. The firmware captures the initial sensor reading.
3. **Place an object over the IR sensor** (or remove one if it is already present) to trigger a transition.
4. The firmware detects any change and immediately records a PASS.
5. If no transition is detected within 10 seconds, it records a FAIL.

**Pass criteria:** Serial output shows `IR sensor: transition detected — PASS`.

**Fail indicators:**
- `IR sensor: no transition detected — FAIL` — check wiring on GPIO 32; verify the IR emitter (D2/LD274) and receiver (Q1/SFH300) are populated.
- Ensure you interact with the sensor during the 10-second window (listen for the two short beeps).

---

### Test 7 — Photoresistor / Day-Night Monitor *(user interaction required)*
**What it does:** Monitors the LDR (GPIO 33 / ADC1_CH5) for 60 seconds and toggles the front and rear LEDs based on detected light changes.

**Sequence:**
1. Firmware samples 10 ADC readings (~1 second) and sets a baseline.
2. Two slow beeps signal the start of the 60-second monitoring window.
3. **Cover and uncover the photoresistor** during this window:
   - If the reading deviates from baseline by more than **±200 ADC counts**, both lamp networks turn ON.
   - When the reading returns near baseline, lamps turn OFF.
4. After 60 seconds, lamps are forced off and the test is marked complete.

**Pass criteria:** Lamps visibly respond to light changes during the window. Serial output shows `PASS`. (The test always passes by timeout — use the lamp response as the functional indicator.)

**Fail indicators:** Lamps never respond — verify R6 (10 kΩ bias resistor) and the LDR are installed correctly; check the ADC baseline value printed to serial.

---

### Completion — 15 Fast Beeps
After all tests, the horn emits 15 rapid beeps (80 ms on / 60 ms off) as an audible completion signal.

---

## Wi-Fi Connection (Post-Test)
After the completion beeps, the firmware attempts to connect to the configured Wi-Fi network:

- **Status LED (GPIO 5)** blinks at 0.5 Hz while connecting.
- On success: Status LED goes **solid**; IP address is printed to serial.
- On timeout (20 s): Status LED blinks rapidly for ~4 seconds; serial prints `Wi‑Fi connect failed (timeout)`.

Wi-Fi failure does not affect the hardware test results.

---

## Serial Monitor Output (Expected)

```
=== Robot Functional Test Firmware ===
    ESP32-DevKitC  |  Arduino-ESP32 v3.x

TEST 1: Horn – 5 slow beeps
  PASS

TEST 2: Front LEDs – 5 blinks
  PASS

TEST 3: Rear LEDs – 5 blinks
  PASS

TEST 4: Left Motor – forward 1 s
  PASS

TEST 5: Right Motor – forward 1 s
  PASS

TEST 6: IR / Line sensor
  6.1 Prompt – place/remove line now (10 s)
  6.2 IR sensor: transition detected — PASS

TEST 7: Photoresistor (Day/Night)
  6.1 Measuring baseline light level...
       Baseline ADC = <value>
  6.2 Signalling user – cover/uncover sensor now...
  6.3 Monitoring for 60 s (threshold = ±200 ADC)
  6.5 Photoresistor test complete.
  PASS

TEST 7: Completion – 15 fast beeps
  PASS

=== ALL TESTS COMPLETE ===
Connecting to Wi‑Fi '...' ...
Wi‑Fi connected. IP: 192.168.x.x
```

---

## Troubleshooting

| Symptom | Likely Cause |
|---|---|
| Upload fails | Wrong COM port selected; driver not installed; board not in bootload mode (hold BOOT, tap EN) |
| `ledcAttach`/`ledcSetup` compile errors | Wrong arduino-esp32 core version — ensure `espressif32@^6.0.0` in `platformio.ini` |
| Horn silent | Piezo not connected to GPIO 21; PWM channel conflict |
| Motors don't spin | Motor supply voltage absent; L293D enable/direction wiring incorrect |
| IR test always fails | GPIO 32 floating or sensor not wired; no interaction during 10-second window |
| ADC reads 0 or 4095 constantly | LDR open/short; R6 missing |
| Wi-Fi always times out | Wrong SSID/password in `main.cpp`; network out of range |

---

## Author

**Ezekiel Mitchell** — Seattle University, Embedded Systems & Design
