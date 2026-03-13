/*
  Touch Interrupt - Capacitive Touch Detection
  Valid touch-capable pins: GPIO 0, 2, 3, 12, 13, 14, 15, 27, 32, 33

  Touching a wire/foil pad connected to GPIO 27 toggles an LED on GPIO 26.
  The ESP32's capacitive touch sensor measures charge/discharge time;
  a lower reading means touch detected.

  Wiring:
    - Touch pad: a short wire or piece of aluminium foil on GPIO 27
    - LED: anode to GPIO 26 through 330Ω resistor, cathode to GND
*/

#define TOUCH_PIN  T7   // GPIO 27  (T0=GPIO4, T2=GPIO2, T3=GPIO15,
                        //           T4=GPIO13, T5=GPIO12, T6=GPIO14,
                        //           T7=GPIO27, T8=GPIO33, T9=GPIO32)
#define LED_PIN    26

volatile bool ledState   = false;
volatile bool touchFlag  = false;

// ISR called when touch threshold is crossed
void IRAM_ATTR touchISR() {
  touchFlag = true;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Attach touch interrupt: fires when sensor value drops below threshold (40)
  // Lower threshold = less sensitive; raise if false-triggers
  touchAttachInterrupt(TOUCH_PIN, touchISR, 40);

  Serial.println("Touch Interrupt ready. Touch GPIO 27 pad to toggle LED.");
}

void loop() {
  if (touchFlag) {
    touchFlag = false;
    ledState  = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);

    Serial.print("Touch detected! LED: ");
    Serial.println(ledState ? "ON" : "OFF");
    Serial.print("Raw touch value: ");
    Serial.println(touchRead(TOUCH_PIN));

    delay(300); // simple debounce — touch pads tend to ring
  }
}
