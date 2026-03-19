# Function Reference — Robot Firmware (Combined)

Detailed documentation of every function, its parameters, internal logic, and side effects across all source files.

---

## Table of Contents

- [config.h — Pin, PWM & Tuning Constants](#configh--pin-pwm--tuning-constants)
- [web\_control.h — Public Interface & Shared State](#web_controlh--public-interface--shared-state)
- [main.cpp — Core Firmware](#maincpp--core-firmware)
  - UDP: [`dance`](#dance) · [`handleCommand`](#handlecommand)
  - Motor Helpers: [`motorsStop`](#motorsstop) · [`driveForward`](#driveforward) · [`driveReverse`](#drivereverse) · [`pivotLeft`](#pivotleft) · [`pivotRight`](#pivotright) · [`curveLeft`](#curveleft) · [`curveRight`](#curveright)
  - Sensor Helpers: [`onLine`](#online) · [`readUltrasonic`](#readultrasonic)
  - Horn: [`beep`](#beep)
  - Utility: [`blinkLamp`](#blinklamp)
  - Functional Tests: [`testHorn`](#testhorn) · [`testLEDs`](#testleds) · [`testLightSensor`](#testlightsensor) · [`testMotors`](#testmotors) · [`testLineSensor`](#testlinesensor) · [`testUltrasonic`](#testultrasonic) · [`runAllTests`](#runalltests)
  - Autonomous Modes: [`runLineFollow`](#runlinefollow) · [`runMazeSolve`](#runmazesolve)
  - Serial Menu: [`printMenu`](#printmenu) · [`handleSerial`](#handleserial)
  - Arduino Entry Points: [`setup`](#setup) · [`loop`](#loop)
- [web\_control.cpp — HTTP Server](#web_controlcpp--http-server)
  - [`handleRoot`](#handleroot) · [`handleSensors`](#handlesensors)
  - [`handleDrive`](#handledrive) · [`handleMode`](#handlemode) · [`handleTest`](#handletest)
  - [`handleHorn`](#handlehorn) · [`handleLedsOn`](#handleledson) · [`handleLedsOff`](#handleledsoff)
  - [`setupWebServer`](#setupwebserver) · [`webServerLoop`](#webserverloop)
- [Data Flow Summary](#data-flow-summary)
- [Author](#author)

---

## config.h — Pin, PWM & Tuning Constants

All values are `#define` macros resolved at compile time. Nothing is runtime-configurable from this file.

### Pin Assignments

| Macro | GPIO | Purpose |
| --- | --- | --- |
| `RMOTOR_1A` | 18 | Right motor direction A (L293D input 1) |
| `RMOTOR_2A` | 19 | Right motor direction B (L293D input 2) |
| `RPWM_1A2A` | 26 | Right motor PWM speed (L293D enable 1) |
| `LMOTOR_3A` | 16 | Left motor direction A (L293D input 3) |
| `LMOTOR_4A` | 17 | Left motor direction B (L293D input 4) |
| `LPWM_3A4A` | 25 | Left motor PWM speed (L293D enable 3) |
| `RENCODER_A/B` | 36, 39 | Right wheel quadrature encoder (input-only) |
| `LENCODER_A/B` | 34, 35 | Left wheel quadrature encoder (input-only) |
| `HORN` | 21 | Piezo buzzer output |
| `FRONTLAMPS` | 2 | Front LED network (bootstrap pin, safe after boot) |
| `REARLAMPS` | 0 | Rear LED network (bootstrap pin, safe after boot) |
| `DAYNIGHT` | 33 | Photoresistor ADC input (ADC1_CH5, 12-bit) |
| `IR_RECEIVE` | 32 | IR line sensor digital input |
| `TRIG` | 4 | HC-SR04 ultrasonic trigger output |
| `ECHO` | 27 | HC-SR04 ultrasonic echo input |
| `STATUS_LED` | 5 | Status LED — solid when Wi-Fi connected |

### PWM / LEDC Constants

| Macro | Value | Purpose |
| --- | --- | --- |
| `HORN_CH` | 0 | LEDC channel for horn |
| `RMOTOR_CH` | 1 | LEDC channel for right motor |
| `LMOTOR_CH` | 2 | LEDC channel for left motor |
| `HORN_FREQ` | 1000 | Horn frequency (Hz) |
| `HORN_RES` | 8 | Horn PWM resolution (bits) |
| `HORN_DUTY` | 128 | Horn duty cycle (~50%) |
| `MOTOR_FREQ` | 5000 | Motor PWM frequency (Hz) |
| `MOTOR_RES` | 8 | Motor PWM resolution (bits) |
| `MOTOR_FULL_DUTY` | 255 | 100% duty cycle (8-bit max) |

### Autonomous Mode Tuning

| Macro | Value | Purpose |
| --- | --- | --- |
| `LINE_DETECT_LEVEL` | 0 | Sensor polarity: `0` = LOW over black line, `1` = HIGH over black line |
| `SPEED_SLOW` | 140 | Motor duty for slow movement |
| `SPEED_MED` | 180 | Motor duty for moderate movement |
| `SPEED_FAST` | 220 | Motor duty for fast movement |
| `SPEED_TURN` | 155 | Motor duty used during pivot turns |
| `LF_STRAIGHT_MS` | 60 | Line follow: drive time (ms) while sensor is on the line |
| `LF_SEARCH_MS` | 220 | Line follow: search turn time (ms) when sensor loses the line |
| `MAZE_SPEED` | `SPEED_FAST` | Maze solver: forward drive speed |
| `WALL_STOP_CM` | 18 | Maze solver: obstacle distance threshold (cm) |
| `TURN_90_MS` | 430 | Maze solver: pivot duration (ms) for ~90° at `SPEED_TURN` |
| `WEB_WATCHDOG_MS` | 600 | Driver mode: stop motors if no `/drive` command received in this window |

### Wi-Fi Credentials

| Macro | Value |
| --- | --- |
| `WIFI_SSID` | `"SU-ECE-LAB"` |
| `WIFI_PASS` | `"FaraDay8086!"` |
| `AP_SSID` | `"ezekielRobot"` |
| `AP_PASS` | `"password123"` |

---

## web_control.h — Public Interface & Shared State

Declares the `RobotMode` enum and all shared variables that `main.cpp` and `web_control.cpp` exchange at runtime.

### `RobotMode` enum

```cpp
enum RobotMode { MODE_IDLE, MODE_MANUAL, MODE_LINE_FOLLOW, MODE_MAZE_SOLVE };
```

| Value | Meaning |
| --- | --- |
| `MODE_IDLE` | Motors stopped, no autonomous algorithm running |
| `MODE_MANUAL` | Web/serial driver control; watchdog active |
| `MODE_LINE_FOLLOW` | `runLineFollow()` called every `loop()` iteration |
| `MODE_MAZE_SOLVE` | `runMazeSolve()` called every `loop()` iteration |

### Shared `extern` variables

| Variable | Type | Owner | Purpose |
| --- | --- | --- | --- |
| `robotMode` | `volatile RobotMode` | `web_control.cpp` | Current operating mode; read by `loop()`, written by web handlers and `handleSerial()` |
| `webSpeed` | `volatile int` | `web_control.cpp` | Motor duty cycle set by the speed slider (0–255); read by `handleDrive()` |
| `lastWebCmd` | `volatile unsigned long` | `web_control.cpp` | `millis()` timestamp of the most recent `/drive` request; used by the watchdog in `loop()` |
| `testResults` | `String` | `main.cpp` | Accumulated pass/fail text; injected into the web page by `handleRoot()` |

---

## main.cpp — Core Firmware

---

### UDP Globals

| Symbol | Type | Value | Purpose |
| --- | --- | --- | --- |
| `robotID` | `int` | `3` | Unique robot identifier; checked against the numeric suffix in `DANCE_<id>` commands |
| `UDP_PORT` | `#define` | `4210` | UDP port number the robot listens on for instructor broadcast commands |
| `udp` | `WiFiUDP` | — | Arduino UDP socket object; opened with `udp.begin(UDP_PORT)` in `setup()` |
| `incomingPacket` | `char[255]` | — | Receive buffer for incoming UDP datagrams; null-terminated after each read |

---

### `dance()`

```cpp
void dance()
```

Performs a visual + audible dance routine triggered by a UDP broadcast command.

Runs 6 cycles (×400 ms each = ~2.4 s total):

1. `FRONTLAMPS` HIGH, `REARLAMPS` LOW, horn on (`HORN_CH` at `HORN_DUTY`) for 200 ms.
2. `FRONTLAMPS` LOW, `REARLAMPS` HIGH, horn off for 200 ms.

After the loop, `REARLAMPS` is driven `LOW` to leave both LED banks off.

Prints `"Robot Dancing!"` to serial at the start.

**Called by:** `handleCommand()`.

---

### `handleCommand()`

```cpp
void handleCommand(String cmd)
```

Parses a UDP command string and dispatches the appropriate action.

| Command | Condition | Action |
| --- | --- | --- |
| `"DANCE_ALL"` | Always | Calls `dance()` |
| `"DANCE_<N>"` | `N == robotID` | Calls `dance()`; ignored if `N` does not match |

Both checks are independent `if` statements, so `"DANCE_ALL"` triggers dance once regardless of `robotID`.

**Called by:** `loop()` when a UDP packet is received.

---

### `motorsStop()`

```cpp
void motorsStop()
```

Brings both motors to an immediate coast stop.

1. Writes `0` to both `RMOTOR_CH` and `LMOTOR_CH` — removes all PWM drive.
2. Sets all four direction pins (`RMOTOR_1A`, `RMOTOR_2A`, `LMOTOR_3A`, `LMOTOR_4A`) `LOW` — neutral state on the L293D H-bridge.

**Used by:** `setup()`, all test functions that run motors, `handleSerial()`, `handleDrive()`, `handleMode()`, `handleTest()`, `runMazeSolve()`.

---

### `driveForward()`

```cpp
void driveForward(int spd)
```

Drives both wheels forward at duty cycle `spd` (0–255).

Sets `RMOTOR_1A` / `LMOTOR_3A` HIGH and `RMOTOR_2A` / `LMOTOR_4A` LOW on both channels, then writes `spd` to both PWM channels.

---

### `driveReverse()`

```cpp
void driveReverse(int spd)
```

Drives both wheels in reverse at duty cycle `spd`.

Inverts the direction pins relative to `driveForward()`: `RMOTOR_1A` / `LMOTOR_3A` LOW, `RMOTOR_2A` / `LMOTOR_4A` HIGH.

---

### `pivotLeft()`

```cpp
void pivotLeft(int spd)
```

Spins the robot left in place. Right wheel drives forward, left wheel drives reverse, both at duty cycle `spd`. Results in a zero-radius left turn.

---

### `pivotRight()`

```cpp
void pivotRight(int spd)
```

Spins the robot right in place. Left wheel drives forward, right wheel drives reverse, both at duty cycle `spd`. Results in a zero-radius right turn.

---

### `curveLeft()`

```cpp
static void curveLeft(int spd)
```

Curves the robot to the left while moving forward. Both wheels drive forward; the left (inner) wheel runs at `spd / 2` and the right (outer) wheel at `spd`. The speed differential produces a gradual arc to the left.

**Used by:** `runLineFollow()` when the sensor is off the line.

---

### `curveRight()`

```cpp
static void curveRight(int spd)
```

Curves the robot to the right while moving forward. Both wheels drive forward; the right (inner) wheel runs at `spd / 2` and the left (outer) wheel at `spd`.

**Used by:** `runLineFollow()` when the sensor is on the line.

---

### `onLine()`

```cpp
static bool onLine()
```

Returns `true` if the IR line sensor currently detects the line.

Reads `IR_RECEIVE` (GPIO 32) and compares to `LINE_DETECT_LEVEL`. If the sensor outputs `LOW` over a black line, set `LINE_DETECT_LEVEL 0`; if it outputs `HIGH`, set it to `1`.

---

### `readUltrasonic()`

```cpp
static long readUltrasonic()
```

Fires the HC-SR04 and returns the measured distance in centimetres. Returns `0` if no echo is received within the 25 ms timeout.

1. Pulls `TRIG` LOW for 2 µs, HIGH for 10 µs, then LOW — the standard trigger sequence.
2. Calls `pulseIn(ECHO, HIGH, 25000UL)` to measure the echo pulse duration in microseconds.
3. Divides by 58 to convert to cm (speed of sound ≈ 340 m/s → 58 µs/cm round trip).

**Used by:** `testUltrasonic()`, `runMazeSolve()`, `handleSensors()`.

---

### `beep()`

```cpp
void beep(int count, int onMs, int offMs)
```

Produces `count` audible tones on the piezo horn.

Each iteration writes `HORN_DUTY` (128) to LEDC channel `HORN_CH` for `onMs` ms, then writes `0` to silence it. If not the last beep, waits `offMs` ms before the next tone.

| Parameter | Description |
| --- | --- |
| `count` | Number of beeps |
| `onMs` | Duration of each beep (ms) |
| `offMs` | Gap between beeps (ms); skipped after the last beep |

---

### `blinkLamp()`

```cpp
static void blinkLamp(int pin, int count, int onMs, int offMs)
```

Blinks a single GPIO pin `count` times. Defined for single-pin blinking; the LED tests drive two pins simultaneously with direct `digitalWrite` calls instead of using this helper.

---

### `testHorn()`

```cpp
void testHorn()
```

**Test 1.** Calls `beep(5, 400, 300)` — five 400 ms tones with 300 ms gaps. Always records PASS (audible verification by the tester).

---

### `testLEDs()`

```cpp
void testLEDs()
```

**Test 2.** Runs three alternating front/rear cycles (400 ms each), then all-on for 500 ms, then all-off. Drives both pins directly with `digitalWrite`. Always records PASS.

---

### `testLightSensor()`

```cpp
void testLightSensor()
```

**Test 3.** Monitors the photoresistor on GPIO 33 for 10 seconds.

1. Beeps twice, then samples 10 ADC readings to establish a baseline.
2. Loops for 10 seconds, sampling every 500 ms.
3. If `abs(reading - baseline) > 200`, activates both LED banks and prints `[DARK]`; otherwise prints `[LIGHT]`.
4. Turns LEDs off and records PASS on timeout.

---

### `testMotors()`

```cpp
void testMotors()
```

**Test 4.** Runs four 1-second phases — left forward, left reverse, right forward, right reverse — with `motorsStop()` and a 300 ms pause between each. Uses `MOTOR_FULL_DUTY` (255) for all phases.

---

### `testLineSensor()`

```cpp
void testLineSensor()
```

**Test 5.** Beeps twice, captures the initial GPIO 32 state, then polls every 50 ms for up to 10 seconds. Records PASS on the first state change; records FAIL if none occurs before the timeout.

---

### `testUltrasonic()`

```cpp
void testUltrasonic()
```

**Test 6.** Calls `readUltrasonic()` five times, printing each distance (or `"no echo"`) to serial with a 200 ms gap between readings. Records PASS if at least one reading is > 0.

---

### `runAllTests()`

```cpp
void runAllTests()
```

Clears `testResults`, then calls Tests 1–6 in sequence. After all pass, calls `beep(15, 80, 60)` as the completion fanfare and appends Test 7 PASS.

---

### `runLineFollow()`

```cpp
static void runLineFollow()
```

**Autonomous line-follow step** — called once per `loop()` iteration while `robotMode == MODE_LINE_FOLLOW`.

Implements a **single-sensor left-edge follower**:

| Sensor state | Action | Delay |
| --- | --- | --- |
| `onLine()` is `true` | `curveRight(SPEED_SLOW)` — steers sensor toward edge | `LF_STRAIGHT_MS` |
| `onLine()` is `false` | `curveLeft(SPEED_SLOW)` — steers back over the line | `LF_SEARCH_MS` |

The robot tracks the left edge of the black line, producing a controlled zig-zag. Short delays keep `webServerLoop()` responsive between steps.

**Starting position:** The IR sensor must start on or near the left edge of the tape. If started fully off the line, the robot will search left until the line is found.

**Tuning:** Adjust `LF_STRAIGHT_MS`, `LF_SEARCH_MS`, and speed in `config.h`. If the robot avoids the line instead of following it, flip `LINE_DETECT_LEVEL`.

---

### `runMazeSolve()`

```cpp
static void runMazeSolve()
```

**Autonomous maze-solve step** — called once per `loop()` iteration while `robotMode == MODE_MAZE_SOLVE`.

Implements a **right-hand-rule** using the front ultrasonic sensor.

| Condition | Action |
| --- | --- |
| No obstacle (`dist > WALL_STOP_CM` or no echo) | `driveForward(MAZE_SPEED)` for 80 ms |
| Obstacle within `WALL_STOP_CM` | Stop, then `pivotRight(SPEED_TURN)` for `TURN_90_MS` |
| Obstacle on 9th consecutive turn (`mazeStuckCount > 8`) | `driveReverse(SPEED_MED)` for 500 ms, reset counter |

`mazeStuckCount` is a file-scope `static int` that resets to `0` whenever a clear path is found or when `handleSerial()` starts the mode. Each blocked turn increments it; the reverse-and-reset recovery prevents infinite spinning in dead ends.

---

### `printMenu()`

```cpp
static void printMenu()
```

Prints the bordered serial menu to the monitor, listing all keys (`1`–`6`, `A`, `M`, `L`, `Z`, `S`), then prints `"Select: "` without a newline to leave the cursor at the prompt.

**Called by:** `setup()` once after boot, and by `handleSerial()` after every command.

---

### `handleSerial()`

```cpp
static void handleSerial()
```

Reads one character from the serial buffer, flushes the rest, and dispatches the command.

For test commands (`1`–`6`, `A`): sets `robotMode = MODE_IDLE` and calls `motorsStop()` first, then calls the appropriate test function.

For mode commands:

| Key | Action |
| --- | --- |
| `M` / `m` | `robotMode = MODE_MANUAL` |
| `L` / `l` | `robotMode = MODE_LINE_FOLLOW`, beeps twice |
| `Z` / `z` | `robotMode = MODE_MAZE_SOLVE`, resets `mazeStuckCount`, beeps three times |
| `S` / `s` | `robotMode = MODE_IDLE` |

Calls `printMenu()` after every command. Returns immediately if no serial data is available.

---

### `setup()`

```cpp
void setup()
```

Arduino entry point. Configures all hardware and connects to Wi-Fi.

**Hardware initialisation:**

- Serial at 115200 baud.
- All motor direction pins, lamp pins, TRIG, and `STATUS_LED` as `OUTPUT`. TRIG and STATUS_LED initialised `LOW`.
- ECHO and `IR_RECEIVE` as `INPUT`; encoder pins as `INPUT_PULLUP`.
- Three LEDC channels configured with `ledcSetup()` and attached with `ledcAttachPin()`.
- All PWM channels zeroed; `motorsStop()` called; ADC resolution set to 12-bit.
- 1-second settling delay.

**Wi-Fi (station mode):**

Connects to `WIFI_SSID` with a 20-second timeout. `STATUS_LED` blinks at 0.5 Hz while connecting:

- **Success:** LED goes solid; IP printed to serial; `setupWebServer()` called; `udp.begin(UDP_PORT)` opens the UDP socket on port 4210.
- **Failure:** LED blinks rapidly for ~4 s, then goes off; firmware continues without a web server or UDP listener.

Unlike the original firmware, tests are **not** run automatically on boot. The serial menu is presented immediately after Wi-Fi setup.

---

### `loop()`

```cpp
void loop()
```

Main execution loop. Runs four things on every iteration:

1. **UDP poll** — calls `udp.parsePacket()`; if a datagram has arrived, reads it into `incomingPacket`, null-terminates it, and passes it to `handleCommand()`.
2. `webServerLoop()` — services pending HTTP requests.
3. `handleSerial()` — processes one serial character if available.
4. Mode dispatch:

| `robotMode` | Action |
| --- | --- |
| `MODE_IDLE` | Nothing |
| `MODE_MANUAL` | Watchdog: calls `motorsStop()` if `millis() - lastWebCmd > WEB_WATCHDOG_MS` |
| `MODE_LINE_FOLLOW` | Calls `runLineFollow()` |
| `MODE_MAZE_SOLVE` | Calls `runMazeSolve()` |

---

## web_control.cpp — HTTP Server

All route handlers are `static` (internal to this translation unit). Shared state (`robotMode`, `webSpeed`, `lastWebCmd`) is defined here and declared `extern` in `web_control.h`. A single `static WebServer server(80)` instance is shared by all functions.

---

### `handleRoot()`

**Route:** `GET /`

Builds and sends the complete HTML control page. The page is assembled from raw-string literals with `testResults` injected at the appropriate point. It contains five sections: Mode Selection, Driver Controls, Quick Controls, Tests, and Live Sensors. JavaScript on the page uses `fetch()` for all button actions (no page reload), and polls `/sensors` every 1.8 seconds.

---

### `handleSensors()`

**Route:** `GET /sensors`

Returns a plain-text sensor snapshot, called by the page's auto-poll every 1.8 s.

1. Fires `TRIG` and measures the HC-SR04 echo.
2. Reads `IR_RECEIVE` and `DAYNIGHT`.
3. Sends a formatted multi-line string: current mode, IR value + on/off-line label, ADC value, and distance.

---

### `handleDrive()`

**Route:** `GET /drive/{fwd|rev|left|right|stop}?spd=N`

Only acts when `robotMode == MODE_MANUAL`; returns an error string otherwise.

Parses the direction from the URI (`server.uri().substring(7)`) and the optional `spd` query parameter (defaults to `webSpeed` if absent, clamped to 0–255). Updates `webSpeed` and `lastWebCmd`, then calls the appropriate motor helper:

| URI segment | Motor call |
| --- | --- |
| `fwd` | `driveForward(spd)` |
| `rev` | `driveReverse(spd)` |
| `left` | `pivotLeft(spd)` |
| `right` | `pivotRight(spd)` |
| `stop` | `motorsStop()` |

The browser holds a direction button → JavaScript sends this request every 380 ms. The server-side watchdog (`WEB_WATCHDOG_MS = 600 ms`) stops the motors if the stream of requests stops.

---

### `handleMode()`

**Route:** `GET /mode/{idle|manual|line|maze}`

Calls `motorsStop()` first (safe transition regardless of current state), then sets `robotMode`:

| URI segment | `robotMode` set to |
| --- | --- |
| `idle` | `MODE_IDLE` |
| `manual` | `MODE_MANUAL` |
| `line` | `MODE_LINE_FOLLOW` |
| `maze` | `MODE_MAZE_SOLVE` |

---

### `handleTest()`

**Route:** `GET /test/{1–6|all}`

Forces `robotMode = MODE_IDLE` and calls `motorsStop()`, then dispatches to the appropriate test function. Blocks until the test completes before sending the HTTP response.

| URI segment | Function called |
| --- | --- |
| `1` | `testHorn()` |
| `2` | `testLEDs()` |
| `3` | `testLightSensor()` |
| `4` | `testMotors()` |
| `5` | `testLineSensor()` |
| `6` | `testUltrasonic()` |
| `all` | `runAllTests()` |

---

### `handleHorn()`

**Route:** `GET /horn`

Calls `beep(3, 150, 100)` then responds `"Beeped!"`. Blocks ~500 ms while beeping.

---

### `handleLedsOn()`

**Route:** `GET /leds/on`

Writes `HIGH` to `FRONTLAMPS` and `REARLAMPS`. Responds `"LEDs ON"`.

---

### `handleLedsOff()`

**Route:** `GET /leds/off`

Writes `LOW` to `FRONTLAMPS` and `REARLAMPS`. Responds `"LEDs OFF"`.

---

### `setupWebServer()`

```cpp
void setupWebServer()
```

Registers all routes with `server.on()` and calls `server.begin()`. Called once from `setup()` if Wi-Fi connects.

| Path | Handler |
| --- | --- |
| `/` | `handleRoot` |
| `/sensors` | `handleSensors` |
| `/drive/fwd` | `handleDrive` |
| `/drive/rev` | `handleDrive` |
| `/drive/left` | `handleDrive` |
| `/drive/right` | `handleDrive` |
| `/drive/stop` | `handleDrive` |
| `/mode/idle` | `handleMode` |
| `/mode/manual` | `handleMode` |
| `/mode/line` | `handleMode` |
| `/mode/maze` | `handleMode` |
| `/test/1` – `/test/6` | `handleTest` |
| `/test/all` | `handleTest` |
| `/horn` | `handleHorn` |
| `/leds/on` | `handleLedsOn` |
| `/leds/off` | `handleLedsOff` |

---

### `webServerLoop()`

```cpp
void webServerLoop()
```

Calls `server.handleClient()` — the ESP32 WebServer library's polling function. Returns immediately if no request is pending. Must be called on every `loop()` iteration to keep the server responsive. Keeping this behind a named function means `main.cpp` does not need to `#include <WebServer.h>`.

---

## Data Flow Summary

```text
setup()
  ├── hardware init (GPIO, LEDC, ADC)
  ├── Wi-Fi STA connect → setupWebServer() if connected
  └── printMenu()

loop()  [runs continuously]
  ├── udp.parsePacket()  [non-blocking]
  │     └── handleCommand()
  │           ├── "DANCE_ALL"     → dance()
  │           └── "DANCE_<id>"   → dance() if id == robotID
  │
  ├── webServerLoop()
  │     └── server.handleClient()
  │           ├── GET /              → handleRoot()       → injects testResults
  │           ├── GET /sensors       → handleSensors()    → live sensor snapshot
  │           ├── GET /drive/*       → handleDrive()      → motor helpers (MANUAL only)
  │           ├── GET /mode/*        → handleMode()       → sets robotMode
  │           ├── GET /test/*        → handleTest()       → calls test functions
  │           ├── GET /horn          → handleHorn()       → beep()
  │           ├── GET /leds/on       → handleLedsOn()
  │           └── GET /leds/off      → handleLedsOff()
  │
  ├── handleSerial()
  │     ├── '1'–'6'  → testXxx()    → appends to testResults
  │     ├── 'A'/'a'  → runAllTests() → appends to testResults
  │     ├── 'M'/'m'  → MODE_MANUAL
  │     ├── 'L'/'l'  → MODE_LINE_FOLLOW
  │     ├── 'Z'/'z'  → MODE_MAZE_SOLVE
  │     └── 'S'/'s'  → MODE_IDLE
  │
  └── mode dispatch
        ├── MODE_IDLE         → nothing
        ├── MODE_MANUAL       → watchdog → motorsStop() if stale
        ├── MODE_LINE_FOLLOW  → runLineFollow()
        │                          onLine() → curveRight / curveLeft
        └── MODE_MAZE_SOLVE   → runMazeSolve()
                                   readUltrasonic() → driveForward / pivotRight / driveReverse
```

---

## Author

**Ezekiel Mitchell** — Seattle University, Embedded Systems & Design
