#include <cstddef>
#include <Arduino.h>
#include <esp32-hal-timer.h>

#define WDT_PIN 26

hw_timer_t *timer = NULL;

void IRAM_ATTR feedWatchdog() {
    digitalWrite(WDT_PIN, !digitalRead(WDT_PIN));
}

void setup() {
    pinMode(WDT_PIN, OUTPUT);
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &feedWatchdog, true);
    timerAlarmWrite(timer, 100000, true);
    timerAlarmEnable(timer);
}

void loop() {
}