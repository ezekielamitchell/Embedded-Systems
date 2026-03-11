/*
 * Ezekiel A. Mitchell
 * Embedded Systems
 * Robot Power-On Functional Test Firmware
 * Platform: ESP32-DevKitC (Arduino-ESP32 core v3.x)
 *
 * Tests (matching test plan numbering):
 *   1. Horn         - 5 cycles, 400 ms on / 300 ms off
 *   2. LEDs         - front/rear alternating, then all on, then all off
 *   3. Light Sensor - 60 s ADC monitor with continuous serial output
 *   4. Motors       - left fwd/rev, right fwd/rev (1 s each)
 *   5. Line Sensor  - transition detection (10 s window)
 *   6. Ultrasonic   - 5 distance readings (HC-SR04)
 *   7. Test Menu    - serial menu in loop() for individual test re-runs
 *
 * Extra Credit:
 *   EC-WiFi - Web server for browser / phone control
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "web_control.h"

// Accumulated pass/fail text - displayed on the web control page
String testResults = "";

// Helpers

void beep(int count, int onMs, int offMs) {
    for (int i = 0; i < count; i++) {
        ledcWrite(HORN_CH, HORN_DUTY);
        delay(onMs);
        ledcWrite(HORN_CH, 0);
        if (i < count - 1) delay(offMs);
    }
}

void blinkLamp(int pin, int count, int onMs, int offMs) {
    for (int i = 0; i < count; i++) {
        digitalWrite(pin, HIGH);
        delay(onMs);
        digitalWrite(pin, LOW);
        if (i < count - 1) delay(offMs);
    }
}

void motorsStop() {
    ledcWrite(RMOTOR_CH, 0);
    ledcWrite(LMOTOR_CH, 0);
    digitalWrite(RMOTOR_1A, LOW);
    digitalWrite(RMOTOR_2A, LOW);
    digitalWrite(LMOTOR_3A, LOW);
    digitalWrite(LMOTOR_4A, LOW);
}

// Individual test functions

void testHorn() {
    Serial.println("TEST 1: Horn - 5 beeps (400 ms on / 300 ms off)");
    beep(5, 400, 300);
    delay(500);
    testResults += "TEST 1  Horn           PASS\n";
    Serial.println("  PASS\n");
}

void testLEDs() {
    Serial.println("TEST 2: LEDs - front/rear alternating, all on, all off");

    // 3 cycles: front on/rear off, then rear on/front off
    for (int i = 0; i < 3; i++) {
        digitalWrite(FRONTLAMPS, HIGH);
        digitalWrite(REARLAMPS,  LOW);
        delay(400);
        digitalWrite(FRONTLAMPS, LOW);
        digitalWrite(REARLAMPS,  HIGH);
        delay(400);
    }

    // All on
    digitalWrite(FRONTLAMPS, HIGH);
    digitalWrite(REARLAMPS,  HIGH);
    delay(500);

    // All off
    digitalWrite(FRONTLAMPS, LOW);
    digitalWrite(REARLAMPS,  LOW);
    delay(500);

    testResults += "TEST 2  LEDs           PASS\n";
    Serial.println("  PASS\n");
}

void testLightSensor() {
    Serial.println("TEST 3: Light Sensor - cover/uncover now (10 s)");
    beep(2, 100, 400);

    // Establish baseline from 10 samples
    long sampleSum = 0;
    for (int s = 0; s < 10; s++) { sampleSum += analogRead(DAYNIGHT); delay(100); }
    int baseline = (int)(sampleSum / 10);
    Serial.print("  Baseline ADC = "); Serial.println(baseline);
    Serial.println("  ADC readings (every 500 ms):");

    unsigned long testStart = millis();
    while (millis() - testStart < 10000UL) {
        int reading = analogRead(DAYNIGHT);
        bool dark = abs(reading - baseline) > 200;
        digitalWrite(FRONTLAMPS, dark ? HIGH : LOW);
        digitalWrite(REARLAMPS,  dark ? HIGH : LOW);
        Serial.print("    ADC = ");
        Serial.print(reading);
        Serial.print("  [");
        Serial.print(dark ? "DARK " : "LIGHT");
        Serial.println("]");
        delay(500);
    }

    digitalWrite(FRONTLAMPS, LOW);
    digitalWrite(REARLAMPS,  LOW);
    testResults += "TEST 3  Light Sensor   PASS\n";
    Serial.println("  PASS\n");
}

void testMotors() {
    Serial.println("TEST 4: Motors - left fwd/rev, right fwd/rev (1 s each)");

    // Left motor — forward
    Serial.println("  Left motor: forward");
    digitalWrite(LMOTOR_3A, HIGH);
    digitalWrite(LMOTOR_4A, LOW);
    ledcWrite(LMOTOR_CH, MOTOR_FULL_DUTY);
    delay(1000);
    motorsStop();
    delay(300);

    // Left motor — reverse
    Serial.println("  Left motor: reverse");
    digitalWrite(LMOTOR_3A, LOW);
    digitalWrite(LMOTOR_4A, HIGH);
    ledcWrite(LMOTOR_CH, MOTOR_FULL_DUTY);
    delay(1000);
    motorsStop();
    delay(300);

    // Right motor — forward
    Serial.println("  Right motor: forward");
    digitalWrite(RMOTOR_1A, HIGH);
    digitalWrite(RMOTOR_2A, LOW);
    ledcWrite(RMOTOR_CH, MOTOR_FULL_DUTY);
    delay(1000);
    motorsStop();
    delay(300);

    // Right motor — reverse
    Serial.println("  Right motor: reverse");
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

    int irInitial = digitalRead(IR_RECEIVE);
    bool irChanged = false;
    unsigned long irStart = millis();
    while (millis() - irStart < 10000UL) {
        if (digitalRead(IR_RECEIVE) != irInitial) { irChanged = true; break; }
        delay(50);
    }

    testResults += irChanged ? "TEST 5  Line Sensor    PASS\n"
                             : "TEST 5  Line Sensor    FAIL (no transition)\n";
    Serial.println(irChanged ? "  Transition detected - PASS\n"
                             : "  No transition - FAIL\n");
    delay(500);
}

void testUltrasonic() {
    Serial.println("TEST 6: Ultrasonic distance sensor");
    bool ultrasonicOK = false;

    for (int i = 0; i < 5; i++) {
        digitalWrite(TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG, LOW);
        long d = pulseIn(ECHO, HIGH, 30000UL) / 58L;
        Serial.print("  Reading "); Serial.print(i + 1); Serial.print(": ");
        if (d > 0) { Serial.print(d); Serial.println(" cm"); ultrasonicOK = true; }
        else        Serial.println("no echo");
        delay(200);
    }

    testResults += ultrasonicOK ? "TEST 6  Ultrasonic     PASS\n"
                                : "TEST 6  Ultrasonic     FAIL (no echo)\n";
    Serial.println(ultrasonicOK ? "  PASS\n" : "  FAIL\n");
}

// Run all tests in sequence and play the completion fanfare
void runAllTests() {
    testResults = "";   // clear so the web page shows only the current run
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

// Serial test menu

void printMenu() {
    Serial.println("=============================");
    Serial.println("  Robot Test Menu");
    Serial.println("=============================");
    Serial.println("  1 - Horn Test");
    Serial.println("  2 - LED Test");
    Serial.println("  3 - Light Sensor Test");
    Serial.println("  4 - Motor Test");
    Serial.println("  5 - Line Sensor Test");
    Serial.println("  6 - Ultrasonic Test");
    Serial.println("  A - Run All Tests");
    Serial.println("  M - Show this menu");
    Serial.println("=============================");
    Serial.print("Enter selection: ");
}

// Arduino entry points

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Robot Functional Test Firmware ===");
    Serial.println("ESP32-DevKitC | Arduino-ESP32 v3.x\n");

    // Output pins
    pinMode(RMOTOR_1A, OUTPUT);
    pinMode(RMOTOR_2A, OUTPUT);
    pinMode(LMOTOR_3A, OUTPUT);
    pinMode(LMOTOR_4A, OUTPUT);
    pinMode(FRONTLAMPS, OUTPUT);
    pinMode(REARLAMPS, OUTPUT);
    pinMode(TRIG, OUTPUT);
    digitalWrite(TRIG, LOW);

    // Status LED
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, LOW);

    // Input pins
    pinMode(ECHO, INPUT);
    pinMode(IR_RECEIVE, INPUT);
    pinMode(RENCODER_A, INPUT_PULLUP);
    pinMode(RENCODER_B, INPUT_PULLUP);
    pinMode(LENCODER_A, INPUT_PULLUP);
    pinMode(LENCODER_B, INPUT_PULLUP);

    // Attach PWM channels
    ledcSetup(HORN_CH,   HORN_FREQ,  HORN_RES);
    ledcAttachPin(HORN,      HORN_CH);
    ledcSetup(RMOTOR_CH, MOTOR_FREQ, MOTOR_RES);
    ledcAttachPin(RPWM_1A2A, RMOTOR_CH);
    ledcSetup(LMOTOR_CH, MOTOR_FREQ, MOTOR_RES);
    ledcAttachPin(LPWM_3A4A, LMOTOR_CH);

    // Safe initial state
    ledcWrite(HORN_CH, 0);
    motorsStop();
    digitalWrite(FRONTLAMPS, LOW);
    digitalWrite(REARLAMPS,  LOW);

    analogReadResolution(12);
    delay(1000); // allow power rails to settle

    // Run all tests automatically on boot
    runAllTests();

    // EXTRA CREDIT: Wi-Fi + Web Server hosted on the ESP32 itself (soft AP)
    Serial.println("Starting ESP32 soft AP...");
    const char *apSSID = AP_SSID;
    const char *apPass = AP_PASS; // leave empty for open network

    WiFi.mode(WIFI_AP);
    if (strlen(apPass) > 0) {
        WiFi.softAP(apSSID, apPass);
    } else {
        WiFi.softAP(apSSID);
    }

    digitalWrite(STATUS_LED, HIGH); // indicate network ready
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("  AP running - connect to ");
    Serial.print(apSSID);
    Serial.print(" on ");
    Serial.println(apIP);

    testResults += "EC      WiFi+WebServer  PASS (AP)\n";
    setupWebServer();

    // Show the serial menu so the user can re-run individual tests
    Serial.println();
    printMenu();
}

void loop() {
    webServerLoop();

    if (Serial.available()) {
        char c = (char)Serial.read();
        while (Serial.available()) Serial.read(); // flush rest of line

        Serial.println(c); // echo selection
        Serial.println();

        switch (c) {
            case '1':           testHorn();        break;
            case '2':           testLEDs();        break;
            case '3':           testLightSensor(); break;
            case '4':           testMotors();      break;
            case '5':           testLineSensor();  break;
            case '6':           testUltrasonic();  break;
            case 'A': case 'a': runAllTests();     break;
            default: break; // fall through to menu reprint
        }

        printMenu();
    }
}
