/*
 * Ezekiel A. Mitchell
 * Embedded Systems — Robot Firmware (Combined)
 * Platform: ESP32-DevKitC (arduino-esp32 v3.x)
 *
 * Functional Tests (matching test-plan numbering):
 *   1. Horn          5 beeps (400 ms on / 300 ms off)
 *   2. LEDs          front/rear alternating, all on, all off
 *   3. Light Sensor  60 s ADC monitor
 *   4. Motors        left fwd/rev, right fwd/rev (1 s each)
 *   5. Line Sensor   transition detection (10 s window)
 *   6. Ultrasonic    5 distance readings
 *   7. Completion    fanfare
 *
 * Autonomous Modes:
 *   Line Follow  — single-sensor left-edge follower (GPIO 32)
 *   Maze Solve   — right-hand-rule with front ultrasonic
 *
 * Driver Mode:
 *   Web UI at http://<robot-ip>/ — hold-to-drive pad, speed slider,
 *   individual test buttons, mode switching, live sensor panel.
 *   Serial menu also available at 115200 baud.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "web_control.h"

// Test results string — displayed on the web page
String testResults = "";

// Motor helpers

void motorsStop() {
    ledcWrite(RMOTOR_CH, 0);
    ledcWrite(LMOTOR_CH, 0);
    digitalWrite(RMOTOR_1A, LOW); 
    digitalWrite(RMOTOR_2A, LOW);
    digitalWrite(LMOTOR_3A, LOW); 
    digitalWrite(LMOTOR_4A, LOW);
}

void driveForward(int spd) {
    digitalWrite(RMOTOR_1A, HIGH); 
    digitalWrite(RMOTOR_2A, LOW);
    digitalWrite(LMOTOR_3A, HIGH); 
    digitalWrite(LMOTOR_4A, LOW);
    ledcWrite(RMOTOR_CH, spd);
    ledcWrite(LMOTOR_CH, spd);
}

void driveReverse(int spd) {
    digitalWrite(RMOTOR_1A, LOW); 
    digitalWrite(RMOTOR_2A, HIGH);
    digitalWrite(LMOTOR_3A, LOW); 
    digitalWrite(LMOTOR_4A, HIGH);
    ledcWrite(RMOTOR_CH, spd);
    ledcWrite(LMOTOR_CH, spd);
}

// Pivot left:  right wheel forward, left wheel reverse
void pivotLeft(int spd) {
    digitalWrite(RMOTOR_1A, HIGH); 
    digitalWrite(RMOTOR_2A, LOW);
    digitalWrite(LMOTOR_3A, LOW);  
    digitalWrite(LMOTOR_4A, HIGH);
    ledcWrite(RMOTOR_CH, spd);
    ledcWrite(LMOTOR_CH, spd);
}

// Pivot right: left wheel forward, right wheel reverse
void pivotRight(int spd) {
    digitalWrite(RMOTOR_1A, LOW);  
    digitalWrite(RMOTOR_2A, HIGH);
    digitalWrite(LMOTOR_3A, HIGH); 
    digitalWrite(LMOTOR_4A, LOW);
    ledcWrite(RMOTOR_CH, spd);
    ledcWrite(LMOTOR_CH, spd);
}

// Gentle curve: inner wheel at half duty
static void curveLeft(int spd) {
    digitalWrite(RMOTOR_1A, HIGH); 
    digitalWrite(RMOTOR_2A, LOW);
    digitalWrite(LMOTOR_3A, HIGH); 
    digitalWrite(LMOTOR_4A, LOW);
    ledcWrite(RMOTOR_CH, spd);
    ledcWrite(LMOTOR_CH, spd / 2);
}
static void curveRight(int spd) {
    digitalWrite(RMOTOR_1A, HIGH); 
    digitalWrite(RMOTOR_2A, LOW);
    digitalWrite(LMOTOR_3A, HIGH); 
    digitalWrite(LMOTOR_4A, LOW);
    ledcWrite(RMOTOR_CH, spd / 2);
    ledcWrite(LMOTOR_CH, spd);
}

// Horn

void beep(int count, int onMs, int offMs) {
    for (int i = 0; i < count; i++) {
        ledcWrite(HORN_CH, HORN_DUTY);
        delay(onMs);
        ledcWrite(HORN_CH, 0);
        if (i < count - 1) delay(offMs);
    }
}

// Sensor helpers

static bool onLine() {
    return digitalRead(IR_RECEIVE) == LINE_DETECT_LEVEL;
}

static long readUltrasonic() {
    digitalWrite(TRIG, LOW);  
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH); 
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    return pulseIn(ECHO, HIGH, 25000UL) / 58L;
}

static void blinkLamp(int pin, int count, int onMs, int offMs) {
    for (int i = 0; i < count; i++) {
        digitalWrite(pin, HIGH); 
        delay(onMs);
        digitalWrite(pin, LOW);
        if (i < count - 1) delay(offMs);
    }
}

// Functional tests (test-plan numbering)

void testHorn() {
    Serial.println("TEST 1: Horn - 5 beeps (400 ms on / 300 ms off)");
    beep(5, 400, 300);
    delay(500);
    testResults += "TEST 1  Horn           PASS\n";
    Serial.println("  PASS\n");
}

void testLEDs() {
    Serial.println("TEST 2: LEDs - front/rear alternating, all on, all off");
    for (int i = 0; i < 3; i++) {
        digitalWrite(FRONTLAMPS, HIGH); 
        digitalWrite(REARLAMPS, LOW);  
        delay(400);
        digitalWrite(FRONTLAMPS, LOW);  
        digitalWrite(REARLAMPS, HIGH); 
        delay(400);
    }
    digitalWrite(FRONTLAMPS, HIGH); 
    digitalWrite(REARLAMPS, HIGH); 
    delay(500);
    digitalWrite(FRONTLAMPS, LOW);  
    digitalWrite(REARLAMPS, LOW);  
    delay(500);
    testResults += "TEST 2  LEDs           PASS\n";
    Serial.println("  PASS\n");
}

void testLightSensor() {
    Serial.println("TEST 3: Light Sensor - cover/uncover now (10 s)");
    beep(2, 100, 400);

    long sum = 0;
    for (int s = 0; s < 10; s++) { 
        sum += analogRead(DAYNIGHT); 
        delay(100); 
    }
    int baseline = (int)(sum / 10);
    Serial.print("  Baseline ADC = "); 
    Serial.println(baseline);

    unsigned long t0 = millis();
    while (millis() - t0 < 10000UL) {
        int reading = analogRead(DAYNIGHT);
        bool dark = abs(reading - baseline) > 200;
        digitalWrite(FRONTLAMPS, dark ? HIGH : LOW);
        digitalWrite(REARLAMPS,  dark ? HIGH : LOW);
        Serial.print("    ADC = "); 
        Serial.print(reading);
        Serial.println(dark ? "  [DARK]" : "  [LIGHT]");
        delay(500);
    }
    digitalWrite(FRONTLAMPS, LOW); 
    digitalWrite(REARLAMPS, LOW);
    testResults += "TEST 3  Light Sensor   PASS\n";
    Serial.println("  PASS\n");
}

void testMotors() {
    Serial.println("TEST 4: Motors - left fwd/rev, right fwd/rev (1 s each)");

    Serial.println("  Left  fwd");
    digitalWrite(LMOTOR_3A, HIGH); 
    digitalWrite(LMOTOR_4A, LOW);
    ledcWrite(LMOTOR_CH, MOTOR_FULL_DUTY); 
    delay(1000); motorsStop(); delay(300);

    Serial.println("  Left  rev");
    digitalWrite(LMOTOR_3A, LOW); 
    digitalWrite(LMOTOR_4A, HIGH);
    ledcWrite(LMOTOR_CH, MOTOR_FULL_DUTY); 
    delay(1000); motorsStop(); delay(300);

    Serial.println("  Right fwd");
    digitalWrite(RMOTOR_1A, HIGH); 
    digitalWrite(RMOTOR_2A, LOW);
    ledcWrite(RMOTOR_CH, MOTOR_FULL_DUTY); 
    delay(1000); 
    motorsStop(); 
    delay(300);

    Serial.println("  Right rev");
    digitalWrite(RMOTOR_1A, LOW); 
    digitalWrite(RMOTOR_2A, HIGH);
    ledcWrite(RMOTOR_CH, MOTOR_FULL_DUTY); 
    delay(1000); 
    motorsStop(); 
    delay(500);

    testResults += "TEST 4  Motors         PASS\n";
    Serial.println("  PASS\n");
}

void testLineSensor() {
    Serial.println("TEST 5: Line Sensor - place/remove line now (10 s)");
    beep(2, 200, 200);

    int initial = digitalRead(IR_RECEIVE);
    bool changed = false;
    unsigned long t0 = millis();
    while (millis() - t0 < 10000UL) {
        if (digitalRead(IR_RECEIVE) != initial) { changed = true; break; }
        delay(50);
    }
    testResults += changed ? "TEST 5  Line Sensor    PASS\n"
                           : "TEST 5  Line Sensor    FAIL (no transition)\n";
    Serial.println(changed ? "  Transition detected - PASS\n" : "  No transition - FAIL\n");
    delay(500);
}

void testUltrasonic() {
    Serial.println("TEST 6: Ultrasonic distance sensor");
    bool ok = false;
    for (int i = 0; i < 5; i++) {
        long d = readUltrasonic();
        Serial.print("  Reading "); 
        Serial.print(i + 1); 
        Serial.print(": ");
        if (d > 0) { Serial.print(d);
            Serial.println(" cm");
            ok = true;
        } else {
            Serial.println("no echo");
        }
        delay(200);
    }
    testResults += ok ? "TEST 6  Ultrasonic     PASS\n"
                      : "TEST 6  Ultrasonic     FAIL (no echo)\n";
    Serial.println(ok ? "  PASS\n" : "  FAIL\n");
}

void runAllTests() {
    testResults = "";
    testHorn();
    testLEDs();
    testLightSensor();
    testMotors();
    testLineSensor();
    testUltrasonic();

    Serial.println("TEST 7: Completion - 15 fast beeps");
    beep(15, 80, 60);
    testResults += "TEST 7  Completion     PASS\n";
    Serial.println("  PASS\n");
    Serial.println("=== ALL TESTS COMPLETE ===\n");
}

// Autonomous: Line Follow
/*
 * Left-edge follower with one IR sensor.
 *   On line  → curve right (moves sensor toward the edge)
 *   Off line → curve left  (steers back over the line)
 * Short delays keep the web server responsive between steps.
 */
static void runLineFollow() {
    if (onLine()) {
        curveRight(SPEED_SLOW);
        delay(LF_STRAIGHT_MS);
    } else {
        curveLeft(SPEED_SLOW);
        delay(LF_SEARCH_MS);
    }
}

// Autonomous: Maze Solve
/*
 * Right-hand-rule with front ultrasonic.
 *   Clear ahead → drive forward.
 *   Obstacle    → pivot right ~90°, recheck; after 8 failed turns back up.
 */
static int mazeStuckCount = 0;

static void runMazeSolve() {
    long dist = readUltrasonic();
    bool blocked = (dist > 0 && dist <= WALL_STOP_CM);

    if (!blocked) {
        driveForward(MAZE_SPEED);
        mazeStuckCount = 0;
        delay(80);
    } else {
        motorsStop(); 
        delay(100);
        mazeStuckCount++;

        if (mazeStuckCount > 8) {
            driveReverse(SPEED_SLOW); 
            delay(500);
            motorsStop();
            mazeStuckCount = 0;
        } else {
            pivotRight(SPEED_TURN); 
            delay(TURN_90_MS);
            motorsStop(); 
            delay(150);
        }
        Serial.print("  Maze: obstacle "); 
        Serial.print(dist);
        Serial.print(" cm, turn "); 
        Serial.println(mazeStuckCount);
    }
}

// Serial menu

static void printMenu() {;
    Serial.println("    Robot Serial Menu:    ");
    Serial.println("|========================|");
    Serial.println("1-6  Run individual test");
    Serial.println("A    Run all tests");
    Serial.println("M    Manual (web driver)");
    Serial.println("L    Line-follow mode");
    Serial.println("Z    Maze-solve mode");
    Serial.println("S    Stop / idle");
    Serial.print("Select: ");
}

static void handleSerial() {
    if (!Serial.available()) return;
    char c = (char)Serial.read();
    while (Serial.available()) Serial.read();
    Serial.println(c); Serial.println();

    robotMode = MODE_IDLE;
    motorsStop();

    switch (c) {
        case '1': testHorn(); 
        break;
        case '2': testLEDs(); 
        break;
        case '3': testLightSensor(); 
        break;
        case '4': testMotors(); 
        break;
        case '5': testLineSensor(); 
        break;
        case '6': testUltrasonic(); 
        break;
        case 'A': case 'a': runAllTests(); 
        break;
        case 'M': case 'm':
            robotMode = MODE_MANUAL;
            Serial.println(">> Manual (driver) mode"); break;
        case 'L': case 'l':
            robotMode = MODE_LINE_FOLLOW;
            beep(2, 100, 100);
            Serial.println(">> Line-follow mode started"); break;
        case 'Z': case 'z':
            robotMode = MODE_MAZE_SOLVE;
            mazeStuckCount = 0;
            beep(3, 80, 80);
            Serial.println(">> Maze-solve mode started"); break;
        case 'S': case 's':
            Serial.println(">> Stopped / idle"); break;
        default: break;
    }
    printMenu();
}

// setup

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Robot Firmware (Combined) ===");

    // Outputs
    pinMode(RMOTOR_1A, OUTPUT); 
    pinMode(RMOTOR_2A, OUTPUT);
    pinMode(LMOTOR_3A, OUTPUT); 
    pinMode(LMOTOR_4A, OUTPUT);
    pinMode(FRONTLAMPS, OUTPUT); 
    pinMode(REARLAMPS, OUTPUT);
    pinMode(STATUS_LED, OUTPUT); 
    digitalWrite(STATUS_LED, LOW);
    pinMode(TRIG, OUTPUT);       
    digitalWrite(TRIG, LOW);

    // Inputs
    pinMode(ECHO, INPUT); 
    pinMode(IR_RECEIVE, INPUT);
    pinMode(RENCODER_A, INPUT_PULLUP); 
    pinMode(RENCODER_B, INPUT_PULLUP);
    pinMode(LENCODER_A, INPUT_PULLUP); 
    pinMode(LENCODER_B, INPUT_PULLUP);

    // LEDC PWM channels
    ledcSetup(HORN_CH, HORN_FREQ, HORN_RES);  
    ledcAttachPin(HORN, HORN_CH);
    ledcSetup(RMOTOR_CH, MOTOR_FREQ, MOTOR_RES); 
    ledcAttachPin(RPWM_1A2A, RMOTOR_CH);
    ledcSetup(LMOTOR_CH, MOTOR_FREQ, MOTOR_RES); 
    ledcAttachPin(LPWM_3A4A, LMOTOR_CH);

    ledcWrite(HORN_CH, 0);
    motorsStop();
    digitalWrite(FRONTLAMPS, LOW); 
    digitalWrite(REARLAMPS, LOW);
    analogReadResolution(12);
    delay(1000);

    // Wi-Fi
    Serial.print("Connecting to '"); 
    Serial.print(WIFI_SSID); Serial.println("'...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000UL) {
        digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(STATUS_LED, HIGH);
        Serial.print("  Connected → open http://");
        Serial.print(WiFi.localIP());
        Serial.println("/ in your browser or phone.");
        testResults += "EC      WiFi+WebServer  PASS (IP: " + WiFi.localIP().toString() + ")\n";
        setupWebServer();
    } else {
        Serial.println("  Wi-Fi failed — continuing offline.");
        testResults += "EC      WiFi+WebServer  FAIL (timeout)\n";
        for (int i = 0; i < 20; i++) {
            digitalWrite(STATUS_LED, !digitalRead(STATUS_LED)); delay(200);
        }
        digitalWrite(STATUS_LED, LOW);
    }

    Serial.println();
    printMenu();
}

// loop

void loop() {
    webServerLoop();
    handleSerial();

    switch (robotMode) {

        case MODE_IDLE:
            break;

        case MODE_MANUAL:
            // Watchdog: stop motors if web commands have gone quiet
            if (millis() - lastWebCmd > WEB_WATCHDOG_MS) {
                motorsStop();
            }
            break;

        case MODE_LINE_FOLLOW:
            runLineFollow();
            break;

        case MODE_MAZE_SOLVE:
            runMazeSolve();
            break;
    }
}
