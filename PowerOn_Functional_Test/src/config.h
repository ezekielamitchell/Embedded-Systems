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

// Ultrasonic (HC-SR04)
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
#define WIFI_SSID "endr"
#define WIFI_PASS "SeattleUniversity01$$"

// Access point credentials (ESP32 hosts the network)
#define AP_SSID "ezekielRobot"
#define AP_PASS "password123"  // leave empty for open network
