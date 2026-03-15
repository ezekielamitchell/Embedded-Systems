#pragma once
#include <Arduino.h>

// ── Robot operating mode ───────────────────────────────────────────────────
enum RobotMode { MODE_IDLE, MODE_MANUAL, MODE_MAZE_SOLVE };
extern volatile RobotMode robotMode;

// ── Shared state written by the web server, read by main.cpp ──────────────
extern volatile int            webSpeed;    // 0-255, set by speed slider
extern volatile unsigned long  lastWebCmd;  // millis() of last /drive request

// ── Test results string – populated by test functions in main.cpp ─────────
extern String testResults;

// ── Public API ─────────────────────────────────────────────────────────────
void setupWebServer();   // call once after Wi-Fi connects
void webServerLoop();    // call every loop() iteration
