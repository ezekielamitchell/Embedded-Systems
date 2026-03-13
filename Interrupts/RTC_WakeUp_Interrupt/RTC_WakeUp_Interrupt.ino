/*
  RTC Wake-Up Interrupt - Deep Sleep with External Wake-Up
  RTC-capable GPIO pins: 0, 2, 4, 12-15, 25-27, 32-39

  The ESP32 enters deep sleep.  Pressing a button connected to an RTC pin
  wakes it up via EXT0 (single pin) or EXT1 (multiple pins).

  Two modes demonstrated — uncomment the one you want:
    MODE_EXT0  : wake on a single RTC pin going LOW (e.g. GPIO 33)
    MODE_EXT1  : wake when ANY of several RTC pins goes LOW (bitmask)

  Wiring:
    - EXT0 button: GPIO 33 to GND (10 kΩ pull-up to 3.3 V recommended)
    - EXT1 buttons (optional): GPIO 32 and GPIO 27 to GND
    - LED: GPIO 26 through 330Ω to GND  (shows brief blink on wake)

  After waking, the sketch prints the wake reason, blinks the LED,
  then returns to deep sleep after AWAKE_MS milliseconds.
*/

#include <esp_sleep.h>

#define LED_PIN       26
#define EXT0_PIN      33          // Single-pin wake-up
#define EXT1_MASK     ((1ULL << 32) | (1ULL << 27))  // Multi-pin wake-up
#define AWAKE_MS      3000        // Time awake before returning to sleep

// Choose EXT0 or EXT1
#define USE_EXT0

void printWakeReason() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  Serial.print("Wake-up cause: ");
  switch (cause) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("EXT0 (single RTC pin)");
      break;
    case ESP_SLEEP_WAKEUP_EXT1: {
      uint64_t pins = esp_sleep_get_ext1_wakeup_status();
      Serial.print("EXT1 (multi RTC pin), triggered pin mask: 0x");
      Serial.println((uint32_t)pins, HEX);
      break;
    }
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("Touch pad");
      break;
    default:
      Serial.println("Power-on / reset (not a sleep wake-up)");
      break;
  }
}

void blinkLED(int times, int onMs = 100, int offMs = 100) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(onMs);
    digitalWrite(LED_PIN, LOW);
    delay(offMs);
  }
}

void setup() {
  Serial.begin(115200);
  delay(100); // let UART settle after wake

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("\n=== RTC Wake-Up Interrupt Demo ===");
  printWakeReason();

  blinkLED(3); // visual confirmation of wake

  Serial.print("Staying awake for ");
  Serial.print(AWAKE_MS);
  Serial.println(" ms, then going to deep sleep...");
  delay(AWAKE_MS);

  // --- Configure wake-up source before sleeping ---
#ifdef USE_EXT0
  // EXT0: wake when GPIO 33 is pulled LOW
  esp_sleep_enable_ext0_wakeup((gpio_num_t)EXT0_PIN, 0 /* 0=LOW, 1=HIGH */);
  Serial.println("Sleeping... Press button on GPIO 33 to wake (EXT0).");
#else
  // EXT1: wake when ANY pin in the mask goes LOW
  // ESP_EXT1_WAKEUP_ANY_LOW — wake if any listed pin is LOW
  esp_sleep_enable_ext1_wakeup(EXT1_MASK, ESP_EXT1_WAKEUP_ANY_LOW);
  Serial.println("Sleeping... Press button on GPIO 32 or 27 to wake (EXT1).");
#endif

  Serial.flush();
  esp_deep_sleep_start(); // does not return
}

void loop() {
  // Never reached — deep sleep resets the chip on wake
}
