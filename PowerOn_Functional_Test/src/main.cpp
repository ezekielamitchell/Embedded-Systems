/*
 * ============================================================
 *  Robot Power-On Functional Test Firmware
 *  Platform : ESP32-DevKitC (Arduino-ESP32 core v3.x)
 *  PWM API  : ledcAttach() / ledcWrite()
 *
 *  Test order:
 *    1. Horn          – 5 slow beeps
 *    2. Front LEDs    – 5 blinks
 *    3. Rear LEDs     – 5 blinks
 *    4. Left Motor    – forward 1 s
 *    5. Right Motor   – forward 1 s
 *    6. Photoresistor – 60-second day/night monitor
 *    7. Completion    – 15 fast beeps
 * ============================================================
 */

#include <Arduino.h>   // required by PlatformIO / C++ compilation
#include <WiFi.h>

// ──────────────────────────────────────────────────────────────
// Pin definitions
// ──────────────────────────────────────────────────────────────

// Right motor  (L293D  EN1 / 1A / 2A)
#define RMOTOR_1A    18   // L293 pin 2  (1A)
#define RMOTOR_2A    19   // L293 pin 7  (2A)
#define RPWM_1A2A    26   // L293 pin 1  (EN1) – PWM

// Left motor   (L293D  EN3 / 3A / 4A)
#define LMOTOR_3A    16   // L293 pin 10 (3A)
#define LMOTOR_4A    17   // L293 pin 15 (4A)
#define LPWM_3A4A    25   // L293 pin 9  (EN3) – PWM

// Encoders – quadrature, open-drain → INPUT_PULLUP required
#define RENCODER_A   36   // input-only GPIO
#define RENCODER_B   39   // input-only GPIO
#define LENCODER_A   34   // input-only GPIO
#define LENCODER_B   35   // input-only GPIO

// Horn
#define HORN         21   // piezo element

// Lighting
// NOTE: GPIO0 and GPIO2 are bootstrap pins; safe as outputs after boot
#define FRONTLAMPS    2   // front LED network (HIGH = ON)
#define REARLAMPS     0   // rear  LED network (HIGH = ON)

// Sensors
#define DAYNIGHT     33   // photoresistor – analog input (ADC1_CH5)
#define IR_RECEIVE   32   // line sensor   – digital input

// Ultrasonic (HC-SR04)
#define TRIG          4
#define ECHO         27

// ──────────────────────────────────────────────────────────────
// PWM parameters
// ──────────────────────────────────────────────────────────────
#define HORN_FREQ        1000   // Hz  – audible tone
#define HORN_RES            8   // bits → 0-255 range
#define HORN_DUTY         128   // 50 % duty cycle

#define MOTOR_FREQ       5000   // Hz
#define MOTOR_RES           8   // bits → 0-255 range
#define MOTOR_FULL_DUTY   255   // 100 % duty cycle

// PWM channels (Arduino-ESP32 core uses channel numbers, not pin numbers)
#define HORN_CH            0
#define RMOTOR_CH          1
#define LMOTOR_CH          2

// Status LED (green) — will blink after tests and become solid on Wi‑Fi connect
#define STATUS_LED         5

// Wi-Fi credentials (will be used after tests complete)
const char* WIFI_SSID = "##########";
const char* WIFI_PASS = "##########";

// ──────────────────────────────────────────────────────────────
// Forward declarations (required in C++, unlike .ino sketches)
// ──────────────────────────────────────────────────────────────
void beep(int count, int onMs, int offMs);
void blinkLamp(int pin, int count, int onMs, int offMs);
void motorsStop();

// ──────────────────────────────────────────────────────────────
// Helper: beep the horn
//   count  – number of beeps
//   onMs   – tone duration (ms)
//   offMs  – silence between beeps (ms); skipped after last beep
// ──────────────────────────────────────────────────────────────
void beep(int count, int onMs, int offMs)
{
    for (int i = 0; i < count; i++) {
        ledcWrite(HORN_CH, HORN_DUTY);   // tone ON
        delay(onMs);
        ledcWrite(HORN_CH, 0);           // tone OFF
        if (i < count - 1) delay(offMs);
    }
}

// ──────────────────────────────────────────────────────────────
// Helper: blink a digital lamp pin
// ──────────────────────────────────────────────────────────────
void blinkLamp(int pin, int count, int onMs, int offMs)
{
    for (int i = 0; i < count; i++) {
        digitalWrite(pin, HIGH);
        delay(onMs);
        digitalWrite(pin, LOW);
        if (i < count - 1) delay(offMs);
    }
}

// ──────────────────────────────────────────────────────────────
// Helper: cut motor drive (direction pins + PWM to 0)
// ──────────────────────────────────────────────────────────────
void motorsStop()
{
    // stop PWM channels too
    ledcWrite(RMOTOR_CH, 0);
    ledcWrite(LMOTOR_CH, 0);
    digitalWrite(RMOTOR_1A, LOW);
    digitalWrite(RMOTOR_2A, LOW);
    digitalWrite(LMOTOR_3A, LOW);
    digitalWrite(LMOTOR_4A, LOW);
}

// ══════════════════════════════════════════════════════════════
void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Robot Functional Test Firmware ===");
    Serial.println("    ESP32-DevKitC  |  Arduino-ESP32 v3.x\n");

    // ── Output pins ──────────────────────────────────────────
    pinMode(RMOTOR_1A,  OUTPUT);
    pinMode(RMOTOR_2A,  OUTPUT);
    pinMode(LMOTOR_3A,  OUTPUT);
    pinMode(LMOTOR_4A,  OUTPUT);
    pinMode(FRONTLAMPS, OUTPUT);
    pinMode(REARLAMPS,  OUTPUT);
    pinMode(TRIG,       OUTPUT);
    digitalWrite(TRIG, LOW);

    // Status LED
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);

    // ── Input pins ───────────────────────────────────────────
    pinMode(ECHO,       INPUT);
    pinMode(IR_RECEIVE, INPUT);
    pinMode(RENCODER_A, INPUT_PULLUP);   // open-drain encoder
    pinMode(RENCODER_B, INPUT_PULLUP);
    pinMode(LENCODER_A, INPUT_PULLUP);
    pinMode(LENCODER_B, INPUT_PULLUP);
    // GPIO33 is analog-only; analogRead() needs no pinMode call

    // ── Attach PWM channels ──────────────────────────────────
    // use ledcSetup + ledcAttachPin (new API in Arduino-ESP32)
    ledcSetup(HORN_CH,     HORN_FREQ,  HORN_RES);
    ledcAttachPin(HORN,    HORN_CH);

    ledcSetup(RMOTOR_CH,   MOTOR_FREQ, MOTOR_RES);
    ledcAttachPin(RPWM_1A2A, RMOTOR_CH);

    ledcSetup(LMOTOR_CH,   MOTOR_FREQ, MOTOR_RES);
    ledcAttachPin(LPWM_3A4A, LMOTOR_CH);

    // ── Safe initial state ───────────────────────────────────
    // ensure nothing is output on any channel
    ledcWrite(HORN_CH, 0);
    motorsStop();
    digitalWrite(FRONTLAMPS, LOW);
    digitalWrite(REARLAMPS,  LOW);

    // Set ADC to 12-bit (default, but explicit is clearer)
    analogReadResolution(12);   // 0-4095 range

    delay(1000);   // allow power rails to settle

    // ══════════════════════════════════════════════════════════
    // TEST 1 – Horn: 5 slow beeps
    // ══════════════════════════════════════════════════════════
    Serial.println("TEST 1: Horn – 5 slow beeps");
    beep(5, 400, 300);
    delay(500);
    Serial.println("  PASS\n");

    // ══════════════════════════════════════════════════════════
    // TEST 2 – Front LEDs: 5 blinks
    // ══════════════════════════════════════════════════════════
    Serial.println("TEST 2: Front LEDs – 5 blinks");
    blinkLamp(FRONTLAMPS, 5, 300, 200);
    delay(500);
    Serial.println("  PASS\n");

    // ══════════════════════════════════════════════════════════
    // TEST 3 – Rear LEDs: 5 blinks
    // ══════════════════════════════════════════════════════════
    Serial.println("TEST 3: Rear LEDs – 5 blinks");
    blinkLamp(REARLAMPS, 5, 300, 200);
    delay(500);
    Serial.println("  PASS\n");

    // ══════════════════════════════════════════════════════════
    // TEST 4 – Left Motor: forward 1 s then stop
    // ══════════════════════════════════════════════════════════
    Serial.println("TEST 4: Left Motor – forward 1 s");
    digitalWrite(LMOTOR_3A, HIGH);          // 3A=H, 4A=L → forward
    digitalWrite(LMOTOR_4A, LOW);
    ledcWrite(LMOTOR_CH, MOTOR_FULL_DUTY);  // 100 % duty
    delay(1000);
    ledcWrite(LMOTOR_CH, 0);                // PWM off
    digitalWrite(LMOTOR_3A, LOW);           // direction pins idle
    digitalWrite(LMOTOR_4A, LOW);
    delay(500);
    Serial.println("  PASS\n");

    // ══════════════════════════════════════════════════════════
    // TEST 5 – Right Motor: forward 1 s then stop
    // ══════════════════════════════════════════════════════════
    Serial.println("TEST 5: Right Motor – forward 1 s");
    digitalWrite(RMOTOR_1A, HIGH);          // 1A=H, 2A=L → forward
    digitalWrite(RMOTOR_2A, LOW);
    ledcWrite(RMOTOR_CH, MOTOR_FULL_DUTY);  // 100 % duty
    delay(1000);
    ledcWrite(RMOTOR_CH, 0);
    digitalWrite(RMOTOR_1A, LOW);
    digitalWrite(RMOTOR_2A, LOW);
    delay(500);
    Serial.println("  PASS\n");

    // ══════════════════════════════════════════════════════════
    // TEST 6 – IR / Line sensor
    // ══════════════════════════════════════════════════════════
    Serial.println("TEST 6: IR / Line sensor");

    // Prompt user to place/remove a line over the sensor: 2 short beeps
    Serial.println("  6.1 Prompt – place/remove line now (10 s)");
    beep(2, 200, 200);

    int irInitial = digitalRead(IR_RECEIVE);
    bool irChanged = false;
    unsigned long irStart = millis();
    while (millis() - irStart < 10000UL) {
        int v = digitalRead(IR_RECEIVE);
        if (v != irInitial) {
            irChanged = true;
            break;
        }
        delay(50);
    }

    if (irChanged) {
        Serial.println("  6.2 IR sensor: transition detected — PASS");
    } else {
        Serial.println("  6.2 IR sensor: no transition detected — FAIL");
    }
    delay(500);

    // ══════════════════════════════════════════════════════════
    // TEST 7 – Photoresistor (Day / Night)
    // ══════════════════════════════════════════════════════════
    Serial.println("TEST 7: Photoresistor (Day/Night)");

    // ── 6.1  Baseline: average 10 samples over ~1 second ─────
    Serial.println("  6.1 Measuring baseline light level...");
    long sampleSum = 0;
    for (int s = 0; s < 10; s++) {
        sampleSum += analogRead(DAYNIGHT);
        delay(100);
    }
    int dayNightBaseline = (int)(sampleSum / 10);
    Serial.print("       Baseline ADC = ");
    Serial.println(dayNightBaseline);

    // ── 6.2  User prompt: 2 slow beeps ───────────────────────
    Serial.println("  6.2 Signalling user – cover/uncover sensor now...");
    beep(2, 600, 400);

    // ── 6.3 / 6.4  Monitor for 60 seconds ───────────────────
    Serial.println("  6.3 Monitoring for 60 s (threshold = ±200 ADC)");
    unsigned long testStart = millis();

    while (millis() - testStart < 60000UL) {
        int current = analogRead(DAYNIGHT);

        if (abs(current - dayNightBaseline) > 200) {
            // Large change detected → lights ON
            digitalWrite(FRONTLAMPS, HIGH);
            digitalWrite(REARLAMPS,  HIGH);
        } else {
            // Back near baseline → lights OFF
            digitalWrite(FRONTLAMPS, LOW);
            digitalWrite(REARLAMPS,  LOW);
        }

        delay(50);   // ~20 Hz sample rate
    }

    // ── 6.5  Exit: ensure lamps off ──────────────────────────
    digitalWrite(FRONTLAMPS, LOW);
    digitalWrite(REARLAMPS,  LOW);
    Serial.println("  6.5 Photoresistor test complete.");
    delay(500);
    Serial.println("  PASS\n");

    // ══════════════════════════════════════════════════════════
    // TEST 7 – Completion indicator: 15 fast beeps
    // ══════════════════════════════════════════════════════════
    Serial.println("TEST 7: Completion – 15 fast beeps");
    beep(15, 80, 60);
    Serial.println("  PASS\n");

    Serial.println("=== ALL TESTS COMPLETE ===");

    // ===== Wi‑Fi connect with status LED =====
    Serial.print("Connecting to Wi‑Fi '");
    Serial.print(WIFI_SSID);
    Serial.println("' ...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long connStart = millis();
    const unsigned long connTimeout = 20000UL; // 20 s

    // Blink STATUS_LED while attempting to connect
    while (WiFi.status() != WL_CONNECTED && (millis() - connStart) < connTimeout) {
        digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(STATUS_LED, HIGH); // solid green
        Serial.print("Wi‑Fi connected. IP: ");
        Serial.println(WiFi.localIP());
    } else {
        // Connection failed — indicate by faster blink
        Serial.println("Wi‑Fi connect failed (timeout)");
        for (int i = 0; i < 20; i++) {
            digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
            delay(200);
        }
    }

}

// ──────────────────────────────────────────────────────────────
// loop() is intentionally empty.
// All tests run once inside setup().
// ──────────────────────────────────────────────────────────────
void loop() { }
