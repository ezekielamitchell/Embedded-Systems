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
|---|---|---|
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

`testResults` is written by `setup()` in `main.cpp` as each test finishes. The web server reads it back and injects it into the HTML dashboard so any browser that opens the page can see the full pass/fail summary.

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
|---|---|
| `count` | Number of beeps to produce |
| `onMs` | Duration each beep is on (ms) |
| `offMs` | Silence gap between beeps (ms); skipped after the last beep |

**Used by:** `setup()` (Tests 1, 7, 8, 9), `handleHorn()` in `web_control.cpp`.

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
|---|---|
| `pin` | GPIO pin number connected to the LED network |
| `count` | Number of blink cycles |
| `onMs` | Time the LED stays on per cycle (ms) |
| `offMs` | Time the LED stays off between cycles (ms); skipped on the last cycle |

**Used by:** `setup()` (Tests 2 and 3, front and rear LEDs).

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

**Side effects:** Both wheels coast to a stop. The function does not apply active braking (shorting both motor terminals); it simply removes drive.

**Used by:** `setup()` (safe initial state), `handleMotorFwd()`, `handleMotorRev()`, `handleMotorStop()` in `web_control.cpp`.

---

### `setup()`

```cpp
void setup()
```

**What it does:** The Arduino entry point. Configures all hardware and runs the entire nine-test diagnostic sequence, then starts the Wi-Fi web server.

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

#### Test 1 — Horn
Calls `beep(5, 400, 300)` — five 400 ms beeps with 300 ms gaps. Appends `"TEST 1  Horn  PASS"` to `testResults`.

#### Test 2 — Front LEDs
Calls `blinkLamp(FRONTLAMPS, 5, 300, 200)` — five blinks of the front lamp network. Appends result to `testResults`.

#### Test 3 — Rear LEDs
Calls `blinkLamp(REARLAMPS, 5, 300, 200)` — five blinks of the rear lamp network. Appends result to `testResults`.

#### Test 4 — Left Motor (forward 1 s)
1. Sets `LMOTOR_3A` HIGH and `LMOTOR_4A` LOW — forward direction on the L293D.
2. Calls `ledcWrite(LMOTOR_CH, MOTOR_FULL_DUTY)` (255) — full speed.
3. `delay(1000)` — runs for one second.
4. Cuts PWM to 0 and pulls direction pins LOW — stops the motor.
5. Appends PASS to `testResults`.

#### Test 5 — Right Motor (forward 1 s)
Same as Test 4 but uses `RMOTOR_1A`, `RMOTOR_2A`, and `RMOTOR_CH` for the right-side H-bridge channel.

#### Test 6 — Ultrasonic (HC-SR04)
Performs 5 distance readings:
1. Pulses TRIG LOW for 2 µs, then HIGH for 10 µs, then LOW — the standard HC-SR04 trigger sequence.
2. Calls `pulseIn(ECHO, HIGH, 30000UL)` — times how long the ECHO pin stays HIGH (in microseconds), with a 30 ms timeout.
3. Divides by 58 to convert the round-trip time to centimeters (speed of sound ~340 m/s → 58 µs/cm round-trip).
4. If any reading is > 0, `ultrasonicOK` is set `true`.
5. Appends PASS or FAIL to `testResults` depending on whether at least one valid echo was received.

#### Test 7 — IR / Line Sensor (10 s window)
1. Calls `beep(2, 200, 200)` — two short beeps to alert the tester.
2. Reads the current state of `IR_RECEIVE` as the baseline.
3. Polls `IR_RECEIVE` every 50 ms for up to 10 seconds.
4. If the pin value ever differs from the baseline, `irChanged` is set `true` and the loop breaks immediately.
5. Appends PASS (transition detected) or FAIL (no transition) to `testResults`.

#### Test 8 — Photoresistor / Day-Night Monitor (60 s window)
1. Calls `beep(2, 600, 400)` — two slow beeps to signal the start of the monitoring window.
2. Takes 10 ADC samples from `DAYNIGHT` (GPIO 33) over ~1 second and averages them to set a baseline.
3. Prints the baseline ADC value to Serial.
4. Loops for 60 seconds, sampling `DAYNIGHT` every 50 ms.
5. If `abs(reading - baseline) > 200`, drives both lamp networks `HIGH` (dark detected — headlights on). Otherwise drives them `LOW`.
6. After the loop, forces both lamps `LOW` and appends PASS to `testResults`.

#### Test 9 — Completion
Calls `beep(15, 80, 60)` — 15 rapid beeps as an audible "all done" signal. Appends PASS to `testResults`.

#### Wi-Fi Connection
1. Starts `WiFi.mode(WIFI_STA)` and `WiFi.begin(WIFI_SSID, WIFI_PASS)`.
2. Toggles `STATUS_LED` every 500 ms while waiting up to 20 seconds for a connection.
3. **On success:** Turns `STATUS_LED` solid, prints the IP address, appends the IP to `testResults`, and calls `setupWebServer()`.
4. **On timeout:** Rapidly blinks `STATUS_LED` 20 times and logs the failure to `testResults`.

---

### `loop()`

```cpp
void loop()
```

**What it does:** The Arduino main loop. Delegates entirely to the web server.

**How it works:** Calls `webServerLoop()` on every iteration. Since all diagnostic tests complete inside `setup()`, `loop()` exists solely to keep the HTTP server alive so the web control panel remains responsive after boot.

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
   - A `<pre>` block containing the live `testResults` string (pass/fail table from `setup()`).
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
1. Calls `beep(3, 150, 100)` — three 150 ms beeps with 100 ms gaps (defined in `main.cpp`, declared as an extern prototype at the top of `web_control.cpp`).
2. Calls `server.send(200, "text/plain", "Beeped!")` to return a confirmation string to the browser, which the JavaScript `cmd()` function displays in the status area.

**Blocking behavior:** The function blocks for approximately 500 ms while the beeps play before the HTTP response is sent.

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

**Blocking behavior:** The function blocks the HTTP server for ~1 second. No other web requests are processed during this time.

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

This reverses the current through each H-bridge half, spinning the motors backward. After 1 second, `motorsStop()` is called and the response `"Reverse 1 s done"` is sent.

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

**Use case:** Acts as an emergency stop if a forward or reverse command was issued and the user wants to stop early (e.g., before the 1-second auto-stop).

---

### `setupWebServer()`

```cpp
void setupWebServer()
```

**What it does:** Registers all URL routes and starts the HTTP server on port 80.

**How it works:**
1. Calls `server.on(path, handler)` seven times to map each URL path to its static handler function:

| Path | Handler |
|---|---|
| `/` | `handleRoot` |
| `/horn` | `handleHorn` |
| `/leds/on` | `handleLedsOn` |
| `/leds/off` | `handleLedsOff` |
| `/motor/fwd` | `handleMotorFwd` |
| `/motor/rev` | `handleMotorRev` |
| `/motor/stop` | `handleMotorStop` |

2. Calls `server.begin()` to start listening on TCP port 80.
3. Prints a confirmation message to Serial.

**Called by:** `setup()` in `main.cpp`, only if Wi-Fi connects successfully.

---

### `webServerLoop()`

```cpp
void webServerLoop()
```

**What it does:** Processes any pending incoming HTTP requests. Must be called continuously to keep the server responsive.

**How it works:** Calls `server.handleClient()` — the ESP32 WebServer library's polling function. This function checks the TCP socket for new connections or data, dispatches requests to the registered route handlers, and sends responses. It returns quickly if there is nothing to process.

**Called by:** `loop()` in `main.cpp` on every iteration.

**Why it exists as a wrapper:** Keeping this call behind a named function in `web_control.cpp` means `main.cpp` does not need to `#include <WebServer.h>` directly; all web-server implementation details stay contained within the `web_control` translation unit.

---

## Data Flow Summary

```
setup() in main.cpp
  │
  ├── runs tests 1–9
  │     └── appends to testResults (global String)
  │
  ├── connects to Wi-Fi
  │
  └── calls setupWebServer()
        └── registers routes, starts server on :80

loop() in main.cpp
  └── calls webServerLoop()
        └── server.handleClient()
              ├── GET /          → handleRoot()  → injects testResults into HTML
              ├── GET /horn      → handleHorn()  → calls beep() from main.cpp
              ├── GET /leds/on   → handleLedsOn()
              ├── GET /leds/off  → handleLedsOff()
              ├── GET /motor/fwd → handleMotorFwd() → calls motorsStop() from main.cpp
              ├── GET /motor/rev → handleMotorRev() → calls motorsStop() from main.cpp
              └── GET /motor/stop→ handleMotorStop()→ calls motorsStop() from main.cpp
```

---

## Author

**Ezekiel Mitchell** — Seattle University, Embedded Systems & Design
