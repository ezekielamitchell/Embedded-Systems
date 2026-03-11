#include "web_control.h"
#include "config.h"
#include <WebServer.h>

// Hardware helper prototypes (defined in main.cpp)
void beep(int count, int onMs, int offMs);
void motorsStop();

// Web server instance
static WebServer server(80);

// Route handlers
static void handleRoot()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html><head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Robot Control</title>
  <style>
    body   { font-family:sans-serif; max-width:480px; margin:auto; padding:16px;
             background:#111; color:#eee; }
    h1     { text-align:center; color:#4cf; }
    h2     { color:#aef; border-bottom:1px solid #444; padding-bottom:4px; }
    button { width:100%; padding:14px; margin:6px 0; font-size:1rem;
             border:none; border-radius:8px; cursor:pointer;
             background:#4cf; color:#111; font-weight:bold; }
    button:active { opacity:0.7; }
    pre    { background:#222; padding:12px; border-radius:6px;
             font-size:0.8rem; white-space:pre-wrap; }
    #msg   { text-align:center; color:#fa4; min-height:1.2em; }
  </style>
  <script>
    function cmd(path) {
      fetch(path).then(r => r.text()).then(t => {
        document.getElementById('msg').innerText = t;
      });
    }
  </script>
</head><body>
  <h1>&#129302; Ezekiel's Robot Control Panel</h1>

  <h2>Test Results</h2>
  <pre>)rawliteral";

    html += testResults;

    html += R"rawliteral(</pre>

  <h2>Controls</h2>
  <button onclick="cmd('/horn')">&#128266; Beep Horn</button>
  <button onclick="cmd('/leds/on')">&#128161; LEDs ON</button>
  <button onclick="cmd('/leds/off')">&#127761; LEDs OFF</button>
  <button onclick="cmd('/motor/fwd')">&#9650; Motors Forward (1 s)</button>
  <button onclick="cmd('/motor/rev')">&#9660; Motors Reverse (1 s)</button>
  <button onclick="cmd('/motor/stop')">&#9632; Motors Stop</button>

  <p id="msg"></p>
</body></html>)rawliteral";

    server.send(200, "text/html", html);
}

static void handleHorn()
{
    beep(3, 150, 100);
    server.send(200, "text/plain", "Beeped!");
}

static void handleLedsOn()
{
    digitalWrite(FRONTLAMPS, HIGH);
    digitalWrite(REARLAMPS, HIGH);
    server.send(200, "text/plain", "LEDs ON");
}

static void handleLedsOff()
{
    digitalWrite(FRONTLAMPS, LOW);
    digitalWrite(REARLAMPS, LOW);
    server.send(200, "text/plain", "LEDs OFF");
}

static void dance() {
  digitalWrite(FRONTLAMPS, HIGH);
  sleep(1);
  digitalWrite(REARLAMPS, HIGH);
  sleep(1);
  digitalWrite(REARLAMPS, LOW);
  sleep(1);
  digitalWrite(FRONTLAMPS, LOW);
}

static void handleMotorFwd()
{
    digitalWrite(LMOTOR_3A, HIGH); digitalWrite(LMOTOR_4A, LOW);
    digitalWrite(RMOTOR_1A, HIGH); digitalWrite(RMOTOR_2A, LOW);
    ledcWrite(LMOTOR_CH, MOTOR_FULL_DUTY);
    ledcWrite(RMOTOR_CH, MOTOR_FULL_DUTY);
    delay(1000);
    motorsStop();
    server.send(200, "text/plain", "Forward 1 s done");
}

static void handleMotorRev()
{
    digitalWrite(LMOTOR_3A, LOW); digitalWrite(LMOTOR_4A, HIGH);
    digitalWrite(RMOTOR_1A, LOW); digitalWrite(RMOTOR_2A, HIGH);
    ledcWrite(LMOTOR_CH, MOTOR_FULL_DUTY);
    ledcWrite(RMOTOR_CH, MOTOR_FULL_DUTY);
    delay(1000);
    motorsStop();
    server.send(200, "text/plain", "Reverse 1 s done");
}

static void handleMotorStop()
{
    motorsStop();
    server.send(200, "text/plain", "Motors stopped");
}

// Public API
void setupWebServer()
{
    server.on("/", handleRoot);
    server.on("/horn", handleHorn);
    server.on("/leds/on", handleLedsOn);
    server.on("/leds/off", handleLedsOff);
    server.on("/motor/fwd", handleMotorFwd);
    server.on("/motor/rev", handleMotorRev);
    server.on("/motor/stop", handleMotorStop);
    server.begin();
    Serial.println("Web server started — open the IP above in a browser or phone.");
}

void webServerLoop()
{
    server.handleClient();
}
