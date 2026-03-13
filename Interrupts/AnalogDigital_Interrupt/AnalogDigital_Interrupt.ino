/*
  Analog + Digital Interrupt - GPIO 32-39
  GPIO 32-39 support both ADC (analog read) and digital interrupts.

  A potentiometer on GPIO 34 (ADC-only input, no pull-up/down) is read
  continuously.  A separate digital interrupt on GPIO 35 (also ADC-capable)
  uses a button to log the current ADC reading at the moment of press.

  Wiring:
    - Potentiometer: outer pins to 3.3 V and GND, wiper to GPIO 34
    - Button:        one leg to GPIO 35, other leg to GND
      (GPIO 32-39 have no internal pull-up; add a 10 kΩ pull-up to 3.3 V)
    - LED:           anode to GPIO 26 through 330Ω resistor, cathode to GND

  Note: GPIO 34-39 are INPUT ONLY — do not drive them as outputs.
*/

#define ADC_PIN     34   // Analog read (potentiometer wiper)
#define INT_PIN     35   // Digital interrupt (button), also ADC-capable
#define LED_PIN     26

volatile bool   captureFlag = false;
volatile unsigned long lastISRTime = 0;
const unsigned long DEBOUNCE_MS   = 200;

void IRAM_ATTR handleInterrupt() {
  unsigned long now = millis();
  if (now - lastISRTime > DEBOUNCE_MS) {
    captureFlag  = true;
    lastISRTime  = now;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // GPIO 34-39 are input-only; no pinMode OUTPUT allowed
  pinMode(INT_PIN, INPUT);   // External pull-up required

  attachInterrupt(digitalPinToInterrupt(INT_PIN), handleInterrupt, FALLING);

  Serial.println("Analog+Digital Interrupt ready.");
  Serial.println("Turn pot to set level; press button on GPIO 35 to capture.");
}

void loop() {
  // Continuously show ADC value
  int adcValue    = analogRead(ADC_PIN);
  float voltage   = adcValue * (3.3f / 4095.0f);

  if (captureFlag) {
    captureFlag = false;
    Serial.println("--- INTERRUPT CAPTURED ---");
    Serial.print("ADC raw: "); Serial.println(adcValue);
    Serial.print("Voltage: "); Serial.print(voltage, 3); Serial.println(" V");

    // Blink LED to acknowledge
    digitalWrite(LED_PIN, HIGH);
    delay(150);
    digitalWrite(LED_PIN, LOW);
  }

  // Print live ADC reading every 500 ms
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    Serial.print("Live ADC: "); Serial.print(adcValue);
    Serial.print("  ("); Serial.print(voltage, 3); Serial.println(" V)");
  }
}
