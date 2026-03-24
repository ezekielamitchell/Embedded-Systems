#pragma once
#include <Arduino.h>

enum RobotMode { MODE_IDLE, MODE_MANUAL, MODE_MAZE_SOLVE, MODE_LINE_FOLLOW };
extern volatile RobotMode robotMode;

extern volatile int            webSpeed;     
extern volatile int            webThreshold;  
extern volatile unsigned long  lastWebCmd;

// Test results string
extern String testResults;

void setupWebServer();
void webServerLoop();
