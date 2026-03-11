# Power-On Functional Test

Firmware that exercises every major peripheral on the REDHAWK ESP32 robot board. On boot it runs all six subsystem tests automatically, then drops into an interactive serial menu (Test 7 — Full Code Integration) so any test can be re-run individually on demand.

> **Test Plan Compliance:** This firmware is written against the *Robot Platform Test Plan* (ECEGR 3210, March 2026). See the [Test Plan Coverage](#test-plan-coverage) section for a full compliance summary.

---

## Prerequisites

| Requirement | Details |
| --- | --- |
| **PlatformIO** | Install via [platformio.org](https://platformio.org) or the VS Code extension |
| **ESP32-DevKitC** board | Connected via USB |
| **Arduino-ESP32 core v3.x** | Pulled in automatically by `espressif32@^6.0.0` in `platformio.ini` |
| **Fully-assembled robot PCB** | Motors, LEDs, horn, and sensors must be populated and wired |

---

## Project Structure

```text
PowerOn_Functional_Test/
├── platformio.ini       # PlatformIO project config
└── src/
    ├── config.h         # Pin assignments and PWM constants
    ├── web_control.h    # Web server public interface
    ├── web_control.cpp  # HTTP route handlers
    └── main.cpp         # Test firmware
```

---

## Hardware & Pin Map

| Signal | GPIO | Notes |
| --- | --- | --- |
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
| Ultrasonic TRIG | 4 | HC-SR04 trigger output |
| Ultrasonic ECHO | 27 | HC-SR04 echo input |
| Status LED | 5 | Green LED — used during Wi-Fi connection phase |

---

## Configuration

Open `src/config.h` and update the Wi-Fi credentials:

```cpp
#define WIFI_SSID "your-network-name"
#define WIFI_PASS "your-password"
```

The Wi-Fi connection is non-critical to the hardware tests — if it times out, the board still completes the full test sequence and enters the serial menu.

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

## Test Sequence (Auto-run on Boot)

All six tests run automatically in `setup()`, then the board enters the serial menu. Tests 3 and 5 require brief manual interaction — see the instructions below.

---

### Test 1 — Horn

**What it does:** Emits 5 beeps (400 ms on / 300 ms off) via the piezo horn on GPIO 21.

**Pass criteria:** You hear 5 audible beeps. Serial output shows `PASS`.

**Fail indicators:** Silence; check that the piezo is connected to GPIO 21 and that the PWM channel is correctly attached.

---

### Test 2 — LEDs

**What it does:** Runs the front and rear LED networks through a full pattern sequence:

1. Front on / Rear off — 400 ms, repeated 3 times alternating
2. All LEDs on — 500 ms
3. All LEDs off — 500 ms

**Pass criteria:** LEDs illuminate in the correct sequence with stable brightness. Serial output shows `PASS`.

**Fail indicators:** LEDs don't activate or activate at the wrong time — verify GPIO 2 and GPIO 0 wiring.

> **Note:** GPIO 0 and GPIO 2 are ESP32 bootstrap pins. The firmware configures them as outputs **after** boot completes, so they are safe to drive here.

---

### Test 3 — Light Sensor *(user interaction required)*

**What it does:** Monitors the photoresistor (GPIO 33 / ADC1_CH5) for 60 seconds. ADC readings are streamed to the serial monitor every 500 ms, and both LED networks are driven HIGH when darkness is detected.

**Sequence:**

1. Firmware samples 10 ADC readings and sets a baseline.
2. Two slow beeps signal the start of the 60-second monitoring window.
3. **Cover and uncover the photoresistor** during this window.
   - If the reading deviates from baseline by more than **±200 ADC counts**, both lamp networks turn ON and serial prints `[DARK]`.
   - When the reading returns near baseline, lamps turn OFF and serial prints `[LIGHT]`.
4. After 60 seconds, lamps are forced off and the test is marked complete.

**Pass criteria:** ADC values visibly change and LEDs respond. Serial output shows a stream of readings and `PASS`.

**Fail indicators:** ADC reads 0 or 4095 constantly — verify R6 (10 kΩ bias resistor) and the LDR are installed correctly.

---

### Test 4 — Motors

**What it does:** Drives each motor independently through a forward and reverse phase (1 second each):

1. Left motor forward — 1 s
2. Left motor reverse — 1 s
3. Right motor forward — 1 s
4. Right motor reverse — 1 s

A 300 ms pause separates each phase.

**Pass criteria:** Each motor spins in the correct direction for each phase. Serial output shows `PASS`.

**Fail indicators:** No movement — verify L293D wiring; check motor supply voltage.

---

### Test 5 — Line Sensor *(user interaction required)*

**What it does:** Reads the digital IR line sensor (GPIO 32) and waits up to **10 seconds** for the signal to change state.

**Sequence:**

1. Two short beeps signal the start of the test window.
2. The firmware captures the initial sensor reading.
3. **Place an object over the IR sensor** (or remove one if already present) to trigger a transition.
4. The firmware detects any change and immediately records a PASS.
5. If no transition is detected within 10 seconds, it records a FAIL.

**Pass criteria:** Serial output shows `Transition detected - PASS`.

**Fail indicators:**

- `No transition - FAIL` — check wiring on GPIO 32; verify the IR emitter and receiver are populated.
- Ensure you interact with the sensor during the 10-second window (listen for the two short beeps).

---

### Test 6 — Ultrasonic

**What it does:** Fires the HC-SR04 5 times and prints measured distances to the serial monitor.

**Sequence:**

1. Pulses TRIG for 10 µs to trigger a measurement.
2. Times the ECHO pulse duration and divides by 58 to convert to centimeters.
3. Prints each reading to serial.

**Pass criteria:** At least one reading returns a non-zero distance. Serial output shows `PASS`.

**Fail indicators:** All readings show `no echo` — check TRIG/ECHO wiring on GPIO 4 / 27; ensure an object is within range.

---

### Completion — 15 Fast Beeps

After all six tests, the horn emits 15 rapid beeps (80 ms on / 60 ms off) as an audible completion signal.

---

## Test 7 — Serial Menu (Post-Boot)

After `setup()` completes, the firmware enters the serial menu in `loop()`. Open the serial monitor at **115200 baud** and send a character to run any individual test:

```text
=============================
  Robot Test Menu
=============================
  1 - Horn Test
  2 - LED Test
  3 - Light Sensor Test
  4 - Motor Test
  5 - Line Sensor Test
  6 - Ultrasonic Test
  A - Run All Tests
  M - Show this menu
=============================
Enter selection:
```

- Sending `1`–`6` runs the corresponding test immediately.
- Sending `A` clears `testResults` and re-runs all tests in order.
- Sending `M` (or any unrecognized character) reprints the menu.
- The menu is reprinted automatically after every test completes.

> **Note:** Tests 3 (Light Sensor) and 5 (Line Sensor) block execution for 60 s and 10 s respectively while they wait for input. The web server is not serviced during this time.

---

## Wi-Fi Connection (Post-Test)

After the completion beeps, the firmware attempts to connect to the configured Wi-Fi network:

- **Status LED (GPIO 5)** blinks at 0.5 Hz while connecting.
- On success: Status LED goes **solid**; IP address is printed to serial. Open `http://<ip>/` in a browser or phone.
- On timeout (20 s): Status LED blinks rapidly for ~4 seconds; serial prints `Wi-Fi connect failed (timeout)`.

Wi-Fi failure does not affect the hardware test results or the serial menu.

---

## Serial Monitor Output (Expected)

```text
=== Robot Functional Test Firmware ===
ESP32-DevKitC | Arduino-ESP32 v3.x

TEST 1: Horn - 5 beeps (400 ms on / 300 ms off)
  PASS

TEST 2: LEDs - front/rear alternating, all on, all off
  PASS

TEST 3: Light Sensor - cover/uncover now (60 s)
  Baseline ADC = 1842
  ADC readings (every 500 ms):
    ADC = 1850  [LIGHT]
    ADC = 312   [DARK ]
    ...
  PASS

TEST 4: Motors - left fwd/rev, right fwd/rev (1 s each)
  Left motor: forward
  Left motor: reverse
  Right motor: forward
  Right motor: reverse
  PASS

TEST 5: Line Sensor - place/remove line now (10 s)
  Transition detected - PASS

TEST 6: Ultrasonic distance sensor
  Reading 1: 23 cm
  Reading 2: 24 cm
  ...
  PASS

TEST 7: Completion - 15 fast beeps
  PASS

=== ALL TESTS COMPLETE ===

Wi-Fi connecting to '...' ...
  Connected - open http://192.168.x.x/ in your browser or phone.

=============================
  Robot Test Menu
...
```

---

## Troubleshooting

| Symptom | Likely Cause |
| --- | --- |
| Upload fails | Wrong COM port selected; driver not installed; board not in bootload mode (hold BOOT, tap EN) |
| `ledcAttach`/`ledcSetup` compile errors | Wrong arduino-esp32 core version — ensure `espressif32@^6.0.0` in `platformio.ini` |
| Horn silent | Piezo not connected to GPIO 21; PWM channel conflict |
| Motors don't spin | Motor supply voltage absent; L293D enable/direction wiring incorrect |
| Motor spins one direction only | Direction pin wiring swapped on L293D |
| IR test always fails | GPIO 32 floating or sensor not wired; no interaction during 10-second window |
| ADC reads 0 or 4095 constantly | LDR open/short; R6 missing |
| Serial menu doesn't respond | Ensure line ending is set to "No line ending" or "Newline" in the serial monitor |
| Wi-Fi always times out | Wrong SSID/password in `config.h`; network out of range |

---

## Test Plan Coverage

Compliance with the *Robot Platform Test Plan* (ECEGR 3210, March 2026). GPS test excluded per assignment scope.

| # | Test Plan Requirement | Firmware Status | Notes |
| --- | --- | --- | --- |
| 1 | Horn on/off, clear audible sound | **Pass** | 5 beeps at 400 ms / 300 ms |
| 2 | Front/rear alternating + all on + all off | **Pass** | 3 alternating cycles then all-on → all-off |
| 3 | Stream ADC values to serial under dark/bright/normal | **Pass** | Readings printed every 500 ms with `[DARK]`/`[LIGHT]` label |
| 4 | Forward, reverse, each motor independently | **Pass** | Left fwd/rev then right fwd/rev, 1 s each |
| 5 | Line sensor: detect line vs. background | **Pass** | Digital transition detected and printed to serial |
| 6 | Ultrasonic: readings change at known distances | **Pass** | 5 readings printed to serial |
| 7 | Test menu: select and run each test independently | **Pass** | Serial menu in `loop()` — send `1`–`6` or `A` |
| 8 (EC) | WiFi + browser/phone control | **Pass** | Web server on port 80 with horn, LED, and motor controls |

---

## Author

**Ezekiel Mitchell** — Seattle University, Embedded Systems & Design
