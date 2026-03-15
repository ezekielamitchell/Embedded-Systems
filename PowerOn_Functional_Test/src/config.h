#pragma once

// Right motor (L293D EN1 / 1A / 2A)
#define RMOTOR_1A 18
#define RMOTOR_2A 19
#define RPWM_1A2A 26

// Left motor (L293D EN3 / 3A / 4A)
#define LMOTOR_3A 16
#define LMOTOR_4A 17
#define LPWM_3A4A 25

// Encoders - quadrature, open-drain, INPUT_PULLUP required
#define RENCODER_A 36
#define RENCODER_B 39
#define LENCODER_A 34
#define LENCODER_B 35

// Horn
#define HORN 21

// Lighting
#define FRONTLAMPS 2
#define REARLAMPS 0

// Sensors
#define DAYNIGHT 33
#define IR_RECEIVE 32

// Ultrasonic (HC-SR04) — TRIG=4, ECHO=27
// Wiring: VCC → 5 V, GND → GND, TRIG → GPIO 4, ECHO → GPIO 27
#define TRIG 4
#define ECHO 27

// Status LED
#define STATUS_LED 5

// PWM parameters
#define HORN_FREQ 1000
#define HORN_RES 8
#define HORN_DUTY 128

#define MOTOR_FREQ 5000
#define MOTOR_RES 8
#define MOTOR_FULL_DUTY 255

#define HORN_CH 0
#define RMOTOR_CH 1
#define LMOTOR_CH 2

// Wi-Fi credentials (station mode - not used when running as soft AP)
#define WIFI_SSID "SU-ECE-LAB"
#define WIFI_PASS "FaraDay8086!"

// Access point credentials (ESP32 hosts the network)
#define AP_SSID "ezekielRobot"
#define AP_PASS "password123"  // leave empty for open network

// Autonomous mode tuning

// Line sensor polarity: 0 = LOW over line, 1 = HIGH over line
#define LINE_DETECT_LEVEL  0

// Motor duty cycles (0-255)
#define SPEED_SLOW 140
#define SPEED_MED 180
#define SPEED_FAST 220
#define SPEED_TURN 155

// Maze solver
#define MAZE_SPEED SPEED_FAST
#define WALL_STOP_CM 18    // stop & turn when obstacle ≤ this
#define TURN_90_MS 430    // time for ~90° pivot at SPEED_TURN

// Line sensor analog threshold
// Sensor outputs HIGHER analog values over black tape (tape absorbs IR, transistor conducts less).
// Readings above this value are treated as "on line".
// Tune by running test 5 and observing on-tape vs off-tape analog values.
#define LINE_SENSOR_THRESHOLD  2000

// Line follower (single IR sensor — bang-bang edge tracking)
#define LINE_FOLLOW_SPEED  SPEED_MED  // base drive speed while following
#define LINE_INNER_RATIO   4          // inner-wheel divisor for gentle curves (1=pivot, 2=moderate, 4=gentle)

// Web watchdog: stop motors if no /drive command within this window (ms)
#define WEB_WATCHDOG_MS 600
