#pragma once

#include <Arduino.h>

// Populated in main.cpp setup(); read by the web page handler
extern String testResults;

// Call once after Wi-Fi connects
void setupWebServer();

// Call every loop() iteration to process HTTP requests
void webServerLoop();
