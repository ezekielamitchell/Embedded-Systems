/*
  Digital Interrupt - GPIO 0-39
  Demonstrates RISING, FALLING, CHANGE, LOW, and HIGH interrupt modes
  using a button on GPIO 14 to toggle an LED on GPIO 26.

  Wiring:
    - Button: one leg to GPIO 14, other leg to GND (uses INPUT_PULLUP)
    - LED:    anode to GPIO 26 through a 330Ω resistor, cathode to GND
*/

#define BUTTON_PIN 14
#define LED_PIN    26

volatile bool ledState = false;
volatile unsigned long lastISRTime = 0;
const unsigned long DEBOUNCE_MS = 200;

// Interrupt Service Routine — kept minimal (no Serial inside ISR)
void IRAM_ATTR handleInterrupt() {
  unsigned long now = millis();
  if (now - lastISRTime > DEBOUNCE_MS) {
    ledState = !ledState;
    lastISRTime = now;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // --- Choose ONE of the following interrupt modes to test ---

  // RISING  : triggers when pin goes LOW → HIGH
  // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleInterrupt, RISING);

  // FALLING : triggers when pin goes HIGH → LOW  (active-low button press)
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleInterrupt, FALLING);

  // CHANGE  : triggers on any edge (both press and release)
  // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleInterrupt, CHANGE);

  // LOW     : fires continuously while pin is LOW — use with care
  // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleInterrupt, LOW);

  // HIGH    : fires continuously while pin is HIGH — use with care
  // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleInterrupt, HIGH);

  Serial.println("Digital Interrupt ready. Press the button to toggle LED.");
}

void loop() {
  // Apply the state change flagged by the ISR
  static bool lastApplied = false;
  if (ledState != lastApplied) {
    lastApplied = ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    Serial.print("LED is now: ");
    Serial.println(ledState ? "ON" : "OFF");
  }
}
