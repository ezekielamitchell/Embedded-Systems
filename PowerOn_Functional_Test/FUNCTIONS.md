# Function Reference — PowerOn Functional Test

Detailed documentation of every function, its parameters, internal logic, and side effects across all source files.

---

## Table of Contents

1. [config.h — Pin & PWM Constants](#configh--pin--pwm-constants)
2. [web_control.h — Public Interface](#web_controlh--public-interface)
3. [main.cpp — Core Firmware](#maincpp--core-firmware)
   - [beep()](#beep)
   - [blinkLamp()](#blinklamp)
   - [motorsStop()](#motorsstop)
   - [testHorn()](#testhorn)
   - [testLEDs()](#testleds)
   - [testLightSensor()](#testlightsensor)
   - [testMotors()](#testmotors)
   - [testLineSensor()](#testlinesensor)
   - [testUltrasonic()](#testultrasonic)
   - [runAllTests()](#runalltests)
   - [printMenu()](#printmenu)
   - [setup()](#setup)
   - [loop()](#loop)
4. [web_control.cpp — HTTP Server](#web_controlcpp--http-server)
   - [handleRoot()](#handleroot)
   - [handleHorn()](#handlehorn)
   - [handleLedsOn()](#handleledson)
   - [handleLedsOff()](#handleledsoff)
   - [handleMotorFwd()](#handlemotorfwd)
   - [handleMotorRev()](#handlemotorrev)
   - [handleMotorStop()](#handlemotorstop)
   - [setupWebServer()](#setupwebserver)
   - [webServerLoop()](#webserverloop)

---

## config.h — Pin & PWM Constants

This header defines every GPIO assignment and PWM parameter used across the project. Nothing is runtime-configurable; all values are `#define` macros resolved at compile time.

| Macro | Value | Purpose |
| --- | --- | --- |
| `RMOTOR_1A` | 18 | Right motor direction pin A (L293D input 1) |
| `RMOTOR_2A` | 19 | Right motor direction pin B (L293D input 2) |
| `RPWM_1A2A` | 26 | Right motor speed PWM (L293D enable 1) |
| `LMOTOR_3A` | 16 | Left motor direction pin A (L293D input 3) |
| `LMOTOR_4A` | 17 | Left motor direction pin B (L293D input 4) |
| `LPWM_3A4A` | 25 | Left motor speed PWM (L293D enable 3) |
| `RENCODER_A/B` | 36, 39 | Right wheel quadrature encoder (input-only GPIOs) |
| `LENCODER_A/B` | 34, 35 | Left wheel quadrature encoder (input-only GPIOs) |
| `HORN` | 21 | Piezo buzzer output |
| `FRONTLAMPS` | 2 | Front LED network (ESP32 bootstrap pin) |
| `REARLAMPS` | 0 | Rear LED network (ESP32 bootstrap pin) |
| `DAYNIGHT` | 33 | Photoresistor ADC input (ADC1_CH5) |
| `IR_RECEIVE` | 32 | IR/line sensor digital input |
| `TRIG` | 4 | HC-SR04 ultrasonic trigger output |
| `ECHO` | 27 | HC-SR04 ultrasonic echo input |
| `STATUS_LED` | 5 | Green status LED |
| `HORN_FREQ` | 1000 | Horn PWM frequency in Hz |
| `HORN_RES` | 8 | Horn PWM resolution in bits (0–255 range) |
| `HORN_DUTY` | 128 | Horn duty cycle (~50%) |
| `MOTOR_FREQ` | 5000 | Motor PWM frequency in Hz |
| `MOTOR_RES` | 8 | Motor PWM resolution in bits |
| `MOTOR_FULL_DUTY` | 255 | 100% motor duty cycle (8-bit max) |
| `HORN_CH` | 0 | LEDC channel assigned to the horn |
| `RMOTOR_CH` | 1 | LEDC channel assigned to the right motor |
| `LMOTOR_CH` | 2 | LEDC channel assigned to the left motor |
| `WIFI_SSID` | `"endr"` | Target Wi-Fi network name |
| `WIFI_PASS` | `"..."` | Target Wi-Fi password |

---

## web_control.h — Public Interface

Declares the shared `testResults` string and the two public functions that `main.cpp` calls to start and run the web server.

```cpp
extern String testResults;
void setupWebServer();
void webServerLoop();
```

`testResults` is written by the individual test functions in `main.cpp` as each test finishes. The web server reads it back and injects it into the HTML dashboard so any browser that opens the page sees the current pass/fail summary. `runAllTests()` clears this string before a full run so the page always shows the most recent results.

---

## main.cpp — Core Firmware

### `beep()`

```cpp
void beep(int count, int onMs, int offMs)
```

**What it does:** Drives the piezo horn (LEDC channel `HORN_CH`) to produce `count` tones.

**How it works:**

1. Iterates `count` times.
2. Each iteration calls `ledcWrite(HORN_CH, HORN_DUTY)` to start a 1 kHz, 50%-duty PWM signal on the horn pin — this is the audible tone.
3. Waits `onMs` milliseconds while the tone plays.
4. Calls `ledcWrite(HORN_CH, 0)` to silence the horn (0% duty).
5. If this is not the last beep, waits `offMs` milliseconds before the next tone.

**Parameters:**

| Parameter | Description |
| --- | --- |
| `count` | Number of beeps to produce |
| `onMs` | Duration each beep is on (ms) |
| `offMs` | Silence gap between beeps (ms); skipped after the last beep |

**Used by:** `testHorn()`, `testLightSensor()`, `testLineSensor()`, `runAllTests()`, `handleHorn()` in `web_control.cpp`.

---

### `blinkLamp()`

```cpp
void blinkLamp(int pin, int count, int onMs, int offMs)
```

**What it does:** Blinks a single digital GPIO pin (a lamp network) `count` times.

**How it works:**

1. Iterates `count` times.
2. Each iteration writes `HIGH` to `pin`, waits `onMs` ms, then writes `LOW`.
3. If not the last blink, waits `offMs` ms before the next cycle.

**Parameters:**

| Parameter | Description |
| --- | --- |
| `pin` | GPIO pin number connected to the LED network |
| `count` | Number of blink cycles |
| `onMs` | Time the LED stays on per cycle (ms) |
| `offMs` | Time the LED stays off between cycles (ms); skipped on the last cycle |

**Note:** This helper operates on a single pin only. The LED test (`testLEDs()`) does not use this function because it requires simultaneous control of two pins for the alternating sequence; that test drives `FRONTLAMPS` and `REARLAMPS` directly with `digitalWrite`.

---

### `motorsStop()`

```cpp
void motorsStop()
```

**What it does:** Brings both motors to a hard stop immediately.

**How it works:**

1. Calls `ledcWrite(RMOTOR_CH, 0)` and `ledcWrite(LMOTOR_CH, 0)` — sets both motor PWM channels to 0% duty, removing all drive power.
2. Sets all four direction pins (`RMOTOR_1A`, `RMOTOR_2A`, `LMOTOR_3A`, `LMOTOR_4A`) `LOW` — puts the L293D H-bridge inputs into a neutral/coast state.

**Parameters:** None.

**Side effects:** Both wheels coast to a stop. The function does not apply active braking; it simply removes drive.

**Used by:** `setup()` (safe initial state), `testMotors()`, `handleMotorFwd()`, `handleMotorRev()`, `handleMotorStop()` in `web_control.cpp`.

---

### `testHorn()`

```cpp
void testHorn()
```

**What it does:** Runs Test 1 — verifies the piezo horn produces audible output.

**How it works:**

1. Prints the test header to Serial.
2. Calls `beep(5, 400, 300)` — five 400 ms tones with 300 ms gaps.
3. Waits 500 ms, then appends `"TEST 1  Horn  PASS"` to `testResults` and prints `PASS` to Serial.

The test always passes (no sensor feedback from the horn) — the tester verifies by listening.

---

### `testLEDs()`

```cpp
void testLEDs()
```

**What it does:** Runs Test 2 — verifies both LED networks through a full alternating and combined pattern sequence.

**How it works:**

1. Prints the test header to Serial.
2. Runs 3 alternating cycles:
   - `FRONTLAMPS` HIGH / `REARLAMPS` LOW for 400 ms.
   - `FRONTLAMPS` LOW / `REARLAMPS` HIGH for 400 ms.
3. Drives both networks HIGH simultaneously for 500 ms (all on).
4. Drives both networks LOW for 500 ms (all off).
5. Appends `"TEST 2  LEDs  PASS"` to `testResults` and prints `PASS` to Serial.

Both pins are controlled directly with `digitalWrite` rather than through `blinkLamp()` because the alternating pattern requires simultaneous two-pin state changes.

---

### `testLightSensor()`

```cpp
void testLightSensor()
```

**What it does:** Runs Test 3 — monitors the photoresistor for 60 seconds, streams ADC readings to the serial monitor, and drives the LEDs as a light/dark indicator.

**How it works:**

1. Calls `beep(2, 600, 400)` — two slow beeps to alert the tester.
2. Samples `DAYNIGHT` (GPIO 33) 10 times over ~1 second and averages them to establish a baseline ADC value. Prints the baseline to Serial.
3. Enters a 60-second loop, sampling every 500 ms:
   - Reads the current ADC value.
   - If `abs(reading - baseline) > 200`, sets `dark = true` — drives both lamp networks HIGH.
   - Otherwise drives both lamp networks LOW.
   - Prints the raw ADC value and `[DARK]` or `[LIGHT]` to Serial.
4. After 60 seconds, forces both lamps LOW, appends PASS to `testResults`, and prints `PASS` to Serial.

The test always passes by timeout — the tester uses the serial output and LED response as the functional indicators. The 500 ms sample interval means approximately 120 readings are printed over the full window.

---

### `testMotors()`

```cpp
void testMotors()
```

**What it does:** Runs Test 4 — drives each motor independently through forward and reverse phases to verify both directions of rotation.

**How it works:**

Runs four sequential phases, each 1 second long with a 300 ms stop between them:

1. **Left forward** — `LMOTOR_3A` HIGH, `LMOTOR_4A` LOW, full PWM on `LMOTOR_CH`.
2. **Left reverse** — `LMOTOR_3A` LOW, `LMOTOR_4A` HIGH, full PWM on `LMOTOR_CH`.
3. **Right forward** — `RMOTOR_1A` HIGH, `RMOTOR_2A` LOW, full PWM on `RMOTOR_CH`.
4. **Right reverse** — `RMOTOR_1A` LOW, `RMOTOR_2A` HIGH, full PWM on `RMOTOR_CH`.

After each phase, `motorsStop()` is called to zero both PWM channels and all direction pins before the next phase begins. A serial message prints the current phase name before each run. Appends PASS to `testResults` and prints `PASS` to Serial after all four phases complete.

---

### `testLineSensor()`

```cpp
void testLineSensor()
```

**What it does:** Runs Test 5 — waits up to 10 seconds for the IR line sensor to change state.

**How it works:**

1. Calls `beep(2, 200, 200)` — two short beeps to alert the tester.
2. Reads `IR_RECEIVE` (GPIO 32) and stores the value as the baseline state.
3. Polls `IR_RECEIVE` every 50 ms for up to 10 seconds.
4. If the pin value differs from the baseline at any point, sets `irChanged = true` and exits the loop immediately.
5. Appends PASS or FAIL to `testResults` and prints the result to Serial.

**Pass** requires the tester to place or remove an object from the IR sensor within the 10-second window. FAIL is recorded if no transition is detected before the timeout.

---

### `testUltrasonic()`

```cpp
void testUltrasonic()
```

**What it does:** Runs Test 6 — fires the HC-SR04 five times and prints measured distances to the serial monitor.

**How it works:**

1. Iterates 5 times.
2. Each iteration:
   - Pulses `TRIG` LOW for 2 µs, HIGH for 10 µs, then LOW again — the standard HC-SR04 trigger sequence.
   - Calls `pulseIn(ECHO, HIGH, 30000UL)` to measure how long the ECHO pin stays HIGH (in microseconds), with a 30 ms timeout.
   - Divides by 58 to convert to centimeters (speed of sound ≈ 340 m/s → 58 µs/cm round trip).
   - Prints the reading or `"no echo"` to Serial.
   - If any reading is > 0, sets `ultrasonicOK = true`.
3. Appends PASS or FAIL to `testResults` based on whether at least one valid echo was received.

---

### `runAllTests()`

```cpp
void runAllTests()
```

**What it does:** Clears `testResults` and runs all six subsystem tests in sequence, then plays the completion fanfare.

**How it works:**

1. Resets `testResults = ""` so the web page shows only the current run.
2. Calls `testHorn()`, `testLEDs()`, `testLightSensor()`, `testMotors()`, `testLineSensor()`, `testUltrasonic()` in order.
3. Calls `beep(15, 80, 60)` — 15 rapid beeps as an audible completion signal.
4. Appends `"TEST 7  Completion  PASS"` to `testResults` and prints the completion banner to Serial.

**Called by:** `setup()` on boot, and by `loop()` when the user sends `'A'` to the serial menu.

---

### `printMenu()`

```cpp
void printMenu()
```

**What it does:** Prints the interactive test selection menu to the serial monitor.

**How it works:** Issues a sequence of `Serial.println()` calls that draw a bordered menu listing all available commands (`1`–`6`, `A`, `M`), then ends with `Serial.print("Enter selection: ")` (no newline) to leave the cursor at the prompt.

**Called by:** `setup()` once after boot setup completes, and by `loop()` after every test or menu command finishes.

---

### `setup()`

```cpp
void setup()
```

**What it does:** The Arduino entry point. Configures all hardware, runs all tests automatically via `runAllTests()`, starts the Wi-Fi web server, then calls `printMenu()` to hand off to the interactive serial menu.

**How it works — step by step:**

#### Initialization

- Opens Serial at 115200 baud and prints a firmware banner.
- Sets all motor direction pins, lamp pins, and the ultrasonic TRIG pin as `OUTPUT`, with TRIG held `LOW`.
- Sets `STATUS_LED` as `OUTPUT` and pulls it `LOW`.
- Sets ECHO, IR_RECEIVE, and all four encoder pins as inputs (`INPUT` or `INPUT_PULLUP`).
- Calls `ledcSetup()` + `ledcAttachPin()` for three LEDC channels: horn (ch 0, 1 kHz, 8-bit), right motor (ch 1, 5 kHz, 8-bit), left motor (ch 2, 5 kHz, 8-bit).
- Writes 0 to all PWM channels and calls `motorsStop()` to ensure a safe startup state.
- Sets ADC resolution to 12 bits (0–4095).
- Waits 1 second for power rails to stabilize.

#### Auto Test Run

Calls `runAllTests()` — this clears `testResults` and executes all six tests plus the completion fanfare.

#### Wi-Fi Connection

1. Starts `WiFi.mode(WIFI_STA)` and `WiFi.begin(WIFI_SSID, WIFI_PASS)`.
2. Toggles `STATUS_LED` every 500 ms while waiting up to 20 seconds for a connection.
3. **On success:** Turns `STATUS_LED` solid, prints the IP address, appends the IP to `testResults`, and calls `setupWebServer()`.
4. **On timeout:** Rapidly blinks `STATUS_LED` 20 times and logs the failure to `testResults`.

#### Menu Handoff

Calls `printMenu()` so the user sees the menu immediately after boot completes.

---

### `loop()`

```cpp
void loop()
```

**What it does:** Services the web server and the serial test menu on every iteration.

**How it works:**

1. Calls `webServerLoop()` to process any pending HTTP requests.
2. Checks `Serial.available()`. If a character is waiting:
   - Reads the character and flushes any remaining bytes on the line.
   - Echoes the character and a blank line to Serial.
   - Dispatches to the appropriate test function via a `switch` statement:

| Input | Action |
| --- | --- |
| `'1'` | `testHorn()` |
| `'2'` | `testLEDs()` |
| `'3'` | `testLightSensor()` |
| `'4'` | `testMotors()` |
| `'5'` | `testLineSensor()` |
| `'6'` | `testUltrasonic()` |
| `'A'` or `'a'` | `runAllTests()` |
| anything else | (no action, falls through) |

1. After the switch, calls `printMenu()` to redisplay the prompt.

**Blocking behavior:** Test functions that have fixed-duration waits (e.g., `testLightSensor()` blocks for 60 s, `testLineSensor()` blocks for up to 10 s) prevent `webServerLoop()` from running during that period. No other web requests are processed while a blocking test is running.

---

## web_control.cpp — HTTP Server

All route handler functions are `static` — they are internal to this translation unit and cannot be called directly from `main.cpp`. The only externally visible symbols are `setupWebServer()` and `webServerLoop()`.

A single `static WebServer server(80)` instance is created at file scope and shared by all functions in this file.

---

### `handleRoot()`

```cpp
static void handleRoot()
```

**Route:** `GET /`

**What it does:** Serves the robot's web control panel as a complete HTML page.

**How it works:**

1. Builds an HTML string using two raw-string literals (`R"rawliteral(...)"`), with a gap in the middle where `testResults` is injected.
2. The page includes:
   - Inline CSS for a dark-themed, mobile-friendly layout (max 480 px wide, dark background, blue accent buttons).
   - An inline JavaScript `cmd(path)` function that issues a `fetch()` request to any path and displays the plain-text response in a `<p id="msg">` status area — no page reload required.
   - A `<pre>` block containing the live `testResults` string (pass/fail table from the last test run).
   - Six control buttons that call `cmd()` with the appropriate API paths.
3. Calls `server.send(200, "text/html", html)` to transmit the page to the client.

**Side effects:** None on hardware. Reads `testResults` (defined in `main.cpp`).

---

### `handleHorn()`

```cpp
static void handleHorn()
```

**Route:** `GET /horn`

**What it does:** Triggers three short horn beeps in response to a browser button press.

**How it works:**

1. Calls `beep(3, 150, 100)` — three 150 ms beeps with 100 ms gaps.
2. Calls `server.send(200, "text/plain", "Beeped!")` to return a confirmation string displayed in the browser status area.

**Blocking behavior:** Blocks for approximately 500 ms while the beeps play before the HTTP response is sent.

---

### `handleLedsOn()`

```cpp
static void handleLedsOn()
```

**Route:** `GET /leds/on`

**What it does:** Turns both front and rear LED networks on.

**How it works:**

1. Writes `HIGH` to `FRONTLAMPS` (GPIO 2) and `REARLAMPS` (GPIO 0).
2. Responds with `server.send(200, "text/plain", "LEDs ON")`.

**Side effects:** Both lamp networks remain on until another request explicitly turns them off.

---

### `handleLedsOff()`

```cpp
static void handleLedsOff()
```

**Route:** `GET /leds/off`

**What it does:** Turns both front and rear LED networks off.

**How it works:**

1. Writes `LOW` to `FRONTLAMPS` and `REARLAMPS`.
2. Responds with `server.send(200, "text/plain", "LEDs OFF")`.

---

### `handleMotorFwd()`

```cpp
static void handleMotorFwd()
```

**Route:** `GET /motor/fwd`

**What it does:** Drives both motors forward at full speed for 1 second, then stops.

**How it works:**

1. Sets direction pins for forward motion on both channels:
   - Left: `LMOTOR_3A` HIGH, `LMOTOR_4A` LOW
   - Right: `RMOTOR_1A` HIGH, `RMOTOR_2A` LOW
2. Calls `ledcWrite()` for both motor PWM channels with `MOTOR_FULL_DUTY` (255).
3. `delay(1000)` — blocks for one second.
4. Calls `motorsStop()` to cut all drive.
5. Responds with `server.send(200, "text/plain", "Forward 1 s done")`.

**Blocking behavior:** Blocks the HTTP server for ~1 second. No other web requests are processed during this time.

---

### `handleMotorRev()`

```cpp
static void handleMotorRev()
```

**Route:** `GET /motor/rev`

**What it does:** Drives both motors in reverse at full speed for 1 second, then stops.

**How it works:** Identical to `handleMotorFwd()` except direction pins are inverted:

- Left: `LMOTOR_3A` LOW, `LMOTOR_4A` HIGH
- Right: `RMOTOR_1A` LOW, `RMOTOR_2A` HIGH

This reverses the current through each H-bridge half, spinning the motors backward. After 1 second, `motorsStop()` is called and `"Reverse 1 s done"` is sent.

---

### `handleMotorStop()`

```cpp
static void handleMotorStop()
```

**Route:** `GET /motor/stop`

**What it does:** Immediately stops both motors.

**How it works:**

1. Calls `motorsStop()` — zeroes PWM and pulls all direction pins LOW.
2. Responds with `server.send(200, "text/plain", "Motors stopped")`.

---

### `setupWebServer()`

```cpp
void setupWebServer()
```

**What it does:** Registers all URL routes and starts the HTTP server on port 80.

**How it works:**

1. Calls `server.on(path, handler)` seven times to map each URL path to its static handler function:

| Path | Handler |
| --- | --- |
| `/` | `handleRoot` |
| `/horn` | `handleHorn` |
| `/leds/on` | `handleLedsOn` |
| `/leds/off` | `handleLedsOff` |
| `/motor/fwd` | `handleMotorFwd` |
| `/motor/rev` | `handleMotorRev` |
| `/motor/stop` | `handleMotorStop` |

1. Calls `server.begin()` to start listening on TCP port 80.
1. Prints a confirmation message to Serial.

**Called by:** `setup()` in `main.cpp`, only if Wi-Fi connects successfully.

---

### `webServerLoop()`

```cpp
void webServerLoop()
```

**What it does:** Processes any pending incoming HTTP requests. Must be called continuously to keep the server responsive.

**How it works:** Calls `server.handleClient()` — the ESP32 WebServer library's polling function. This checks the TCP socket for new connections or data, dispatches requests to the registered route handlers, and sends responses. It returns quickly if there is nothing to process.

**Called by:** `loop()` in `main.cpp` on every iteration.

**Why it exists as a wrapper:** Keeping this call behind a named function in `web_control.cpp` means `main.cpp` does not need to `#include <WebServer.h>` directly; all web-server implementation details stay contained within the `web_control` translation unit.

---

## Data Flow Summary

```text
setup() in main.cpp
  │
  ├── hardware init
  │
  ├── runAllTests()
  │     ├── testHorn()          → appends to testResults
  │     ├── testLEDs()          → appends to testResults
  │     ├── testLightSensor()   → appends to testResults
  │     ├── testMotors()        → appends to testResults
  │     ├── testLineSensor()    → appends to testResults
  │     ├── testUltrasonic()    → appends to testResults
  │     └── completion beeps
  │
  ├── Wi-Fi connect
  │     └── setupWebServer()  → registers routes, starts server on :80
  │
  └── printMenu()

loop() in main.cpp
  ├── webServerLoop()
  │     └── server.handleClient()
  │           ├── GET /           → handleRoot()       → injects testResults into HTML
  │           ├── GET /horn       → handleHorn()       → calls beep() from main.cpp
  │           ├── GET /leds/on    → handleLedsOn()
  │           ├── GET /leds/off   → handleLedsOff()
  │           ├── GET /motor/fwd  → handleMotorFwd()   → calls motorsStop()
  │           ├── GET /motor/rev  → handleMotorRev()   → calls motorsStop()
  │           └── GET /motor/stop → handleMotorStop()  → calls motorsStop()
  │
  └── Serial input dispatch
        ├── '1' → testHorn()
        ├── '2' → testLEDs()
        ├── '3' → testLightSensor()
        ├── '4' → testMotors()
        ├── '5' → testLineSensor()
        ├── '6' → testUltrasonic()
        ├── 'A' → runAllTests()
        └── printMenu()
```

---

## Author

**Ezekiel Mitchell** — Seattle University, Embedded Systems & Design
