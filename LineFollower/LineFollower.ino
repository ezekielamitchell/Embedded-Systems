// ---------------- Pin Definitions ----------------

// Phototransistor (confirm PIN definitions with the schematic) 
#define PHOTOTRANS_PIN  32

// L293D Motor Driver Pins
#define RMOTOR_1A   18
#define RMOTOR_2A   19
#define RPWM_1A2A   5

#define LMOTOR_3A   16
#define LMOTOR_4A   17
#define LPWM_3A4A   4

// -------------------------------------------------

int lineValue;
int threshold = 2000;   // Intentionally incorrect
String currentState = "STOP";

void setup() {
  Serial.begin(115200);

  pinMode(PHOTOTRANS_PIN, INPUT);

  pinMode(RMOTOR_1A, OUTPUT);
  pinMode(RMOTOR_2A, OUTPUT);
  pinMode(LMOTOR_3A, OUTPUT);
  pinMode(LMOTOR_4A, OUTPUT);

  pinMode(RPWM_1A2A, OUTPUT);
  pinMode(LPWM_3A4A, OUTPUT);

  analogWrite(RPWM_1A2A, 150);
  analogWrite(LPWM_3A4A, 150);
}

// ---------------- Motor Functions ----------------

void moveForward() {
  digitalWrite(RMOTOR_1A, HIGH);
  digitalWrite(RMOTOR_2A, LOW);
  digitalWrite(LMOTOR_3A, HIGH);
  digitalWrite(LMOTOR_4A, LOW);
  currentState = "FORWARD";
}

void searchRight() {
  digitalWrite(RMOTOR_1A, LOW);
  digitalWrite(RMOTOR_2A, HIGH);
  digitalWrite(LMOTOR_3A, HIGH);
  digitalWrite(LMOTOR_4A, LOW);
  currentState = "SEARCH";
}

void stopRobot() {
  digitalWrite(RMOTOR_1A, LOW);
  digitalWrite(RMOTOR_2A, LOW);
  digitalWrite(LMOTOR_3A, LOW);
  digitalWrite(LMOTOR_4A, LOW);
  currentState = "STOP";
}

// ---------------- Main Loop ----------------

void loop() {

  lineValue = analogRead(PHOTOTRANS_PIN);

  Serial.print("Sensor: ");
  Serial.print(lineValue);
  Serial.print("  State: ");
  Serial.println(currentState);

  // -------- FAULTY LOGIC --------

  if (lineValue < threshold) {
    moveForward();
  }
  else {
    searchRight();   // Always searches right (intentional flaw)
  }

  delay(200);   // Slow reaction (intentional flaw)
}