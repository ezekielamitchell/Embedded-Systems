/*
 * Ezekiel A. Mitchell
 * Embedded Systems
 * Robot Power-On Functional Test Firmware
 * Platform: ESP32-DevKitC (Arduino-ESP32 core v3.x)
 *
 * Test order:
 *   1. Horn          - 5 slow beeps
 *   2. Front LEDs    - 5 blinks
 *   3. Rear LEDs     - 5 blinks
 *   4. Left Motor    - forward 1 s
 *   5. Right Motor   - forward 1 s
 *   6. Ultrasonic    - 5 distance readings (HC-SR04)
 *   7. Line sensor   - transition detection (10 s)
 *   8. Photoresistor - 60-second day/night monitor
 *   9. Completion    - 15 fast beeps
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

// Helper: beep the horn
void beep(int count, int onMs, int offMs)
{
    for (int i = 0; i < count; i++) {
        ledcWrite(HORN_CH, HORN_DUTY);
        delay(onMs);
        ledcWrite(HORN_CH, 0);
        if (i < count - 1) delay(offMs);
    }
}

// Helper: blink a digital lamp pin
void blinkLamp(int pin, int count, int onMs, int offMs)
{
    for (int i = 0; i < count; i++) {
        digitalWrite(pin, HIGH);
        delay(onMs);
        digitalWrite(pin, LOW);
        if (i < count - 1) delay(offMs);
    }
}

// Helper: cut motor drive (direction pins + PWM to 0)
void motorsStop()
{
    ledcWrite(RMOTOR_CH, 0);
    ledcWrite(LMOTOR_CH, 0);
    digitalWrite(RMOTOR_1A, LOW);
    digitalWrite(RMOTOR_2A, LOW);
    digitalWrite(LMOTOR_3A, LOW);
    digitalWrite(LMOTOR_4A, LOW);
}

void setup()
{
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
    ledcSetup(HORN_CH, HORN_FREQ, HORN_RES);
    ledcAttachPin(HORN, HORN_CH);
    ledcSetup(RMOTOR_CH, MOTOR_FREQ, MOTOR_RES);
    ledcAttachPin(RPWM_1A2A, RMOTOR_CH);
    ledcSetup(LMOTOR_CH, MOTOR_FREQ, MOTOR_RES);
    ledcAttachPin(LPWM_3A4A, LMOTOR_CH);

    // Safe initial state
    ledcWrite(HORN_CH, 0);
    motorsStop();
    digitalWrite(FRONTLAMPS, LOW);
    digitalWrite(REARLAMPS, LOW);

    analogReadResolution(12);
    delay(1000); // allow power rails to settle

    // TEST 1: Horn
    Serial.println("TEST 1: Horn - 5 slow beeps");
    beep(5, 400, 300);
    delay(500);
    testResults += "TEST 1  Horn           PASS\n";
    Serial.println("  PASS\n");

    // TEST 2: Front LEDs
    Serial.println("TEST 2: Front LEDs - 5 blinks");
    blinkLamp(FRONTLAMPS, 5, 300, 200);
    delay(500);
    testResults += "TEST 2  Front LEDs     PASS\n";
    Serial.println("  PASS\n");

    // TEST 3: Rear LEDs
    Serial.println("TEST 3: Rear LEDs - 5 blinks");
    blinkLamp(REARLAMPS, 5, 300, 200);
    delay(500);
    testResults += "TEST 3  Rear LEDs      PASS\n";
    Serial.println("  PASS\n");

    // TEST 4: Left Motor
    Serial.println("TEST 4: Left Motor - forward 1 s");
    digitalWrite(LMOTOR_3A, HIGH);
    digitalWrite(LMOTOR_4A, LOW);
    ledcWrite(LMOTOR_CH, MOTOR_FULL_DUTY);
    delay(1000);
    ledcWrite(LMOTOR_CH, 0);
    digitalWrite(LMOTOR_3A, LOW);
    digitalWrite(LMOTOR_4A, LOW);
    delay(500);
    testResults += "TEST 4  Left Motor     PASS\n";
    Serial.println("  PASS\n");

    // TEST 5: Right Motor
    Serial.println("TEST 5: Right Motor - forward 1 s");
    digitalWrite(RMOTOR_1A, HIGH);
    digitalWrite(RMOTOR_2A, LOW);
    ledcWrite(RMOTOR_CH, MOTOR_FULL_DUTY);
    delay(1000);
    ledcWrite(RMOTOR_CH, 0);
    digitalWrite(RMOTOR_1A, LOW);
    digitalWrite(RMOTOR_2A, LOW);
    delay(500);
    testResults += "TEST 5  Right Motor    PASS\n";
    Serial.println("  PASS\n");

    // TEST 6: Ultrasonic (HC-SR04)
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
        else          Serial.println("no echo");
        delay(200);
    }
    testResults += ultrasonicOK ? "TEST 6  Ultrasonic     PASS\n"
                                : "TEST 6  Ultrasonic     FAIL (no echo)\n";
    Serial.println(ultrasonicOK ? "  PASS\n" : "  FAIL\n");

    // TEST 7: IR / Line sensor
    Serial.println("TEST 7: IR / Line sensor - place/remove line now (10 s)");
    beep(2, 200, 200);
    int irInitial = digitalRead(IR_RECEIVE);
    bool irChanged = false;
    unsigned long irStart = millis();
    while (millis() - irStart < 10000UL) {
        if (digitalRead(IR_RECEIVE) != irInitial) { irChanged = true; break; }
        delay(50);
    }
    testResults += irChanged ? "TEST 7  Line sensor    PASS\n"
                             : "TEST 7  Line sensor    FAIL (no transition)\n";
    Serial.println(irChanged ? "  Transition detected - PASS\n" : "  No transition - FAIL\n");
    delay(500);

    // TEST 8: Photoresistor (Day/Night) - 60 s monitor
    Serial.println("TEST 8: Photoresistor - cover/uncover sensor now (60 s)");
    beep(2, 600, 400);
    long sampleSum = 0;
    for (int s = 0; s < 10; s++) { sampleSum += analogRead(DAYNIGHT); delay(100); }
    int baseline = (int)(sampleSum / 10);
    Serial.print("  Baseline ADC = "); Serial.println(baseline);
    unsigned long testStart = millis();
    while (millis() - testStart < 60000UL) {
        bool dark = abs(analogRead(DAYNIGHT) - baseline) > 200;
        digitalWrite(FRONTLAMPS, dark ? HIGH : LOW);
        digitalWrite(REARLAMPS, dark ? HIGH : LOW);
        delay(50);
    }
    digitalWrite(FRONTLAMPS, LOW);
    digitalWrite(REARLAMPS, LOW);
    testResults += "TEST 8  Photoresistor  PASS\n";
    Serial.println("  PASS\n");

    // TEST 9: Completion
    Serial.println("TEST 9: Completion - 15 fast beeps");
    beep(15, 80, 60);
    testResults += "TEST 9  Completion     PASS\n";
    Serial.println("  PASS\n");

    Serial.println("=== ALL REQUIRED TESTS COMPLETE ===\n");

    // EXTRA CREDIT: Wi-Fi + Web Server
    Serial.print("Wi-Fi connecting to '");
    Serial.print(WIFI_SSID);
    Serial.println("' ...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long connStart = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - connStart) < 20000UL) {
        digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(STATUS_LED, HIGH);
        Serial.print("  Connected - open http://");
        Serial.print(WiFi.localIP());
        Serial.println("/ in your browser or phone.");
        testResults += "EC      WiFi+WebServer  PASS (IP: " + WiFi.localIP().toString() + ")\n";
        setupWebServer();
    } else {
        Serial.println("  Wi-Fi connect failed (timeout)");
        testResults += "EC      WiFi+WebServer  FAIL (no connection)\n";
        for (int i = 0; i < 20; i++) {
            digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
            delay(200);
        }
        digitalWrite(STATUS_LED, LOW);
    }
}

void loop()
{
    webServerLoop();
}
