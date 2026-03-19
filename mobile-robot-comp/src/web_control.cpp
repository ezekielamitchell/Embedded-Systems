#include "web_control.h"
#include "config.h"
#include <WebServer.h>

// Shared state definitions
volatile RobotMode      robotMode     = MODE_IDLE;
volatile int            webSpeed      = MOTOR_FULL_DUTY;
volatile int            webThreshold  = LINE_SENSOR_THRESHOLD;
volatile unsigned long  lastWebCmd    = 0;

// Hardware helpers defined in main.cpp
void beep(int count, int onMs, int offMs);
void motorsStop();
void lineFollowCalibrate();
void driveForward(int spd);
void driveReverse(int spd);
void pivotLeft(int spd);
void pivotRight(int spd);

// Test functions defined in main.cpp
void testHorn();
void testLEDs();
void testLightSensor();
void testMotors();
void testLineSensor();
void testUltrasonic();
void runAllTests();

static WebServer server(80);

// Root page (unified UI)
static void handleRoot()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html><head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Robot Control</title>
  <style>
    *      { box-sizing:border-box; }
    body   { font-family:sans-serif; background:#111; color:#eee;
             max-width:480px; margin:auto; padding:12px; }
    h1     { text-align:center; color:#4cf; margin:4px 0 10px; }
    h2     { color:#aef; border-bottom:1px solid #333; padding-bottom:3px;
             margin:14px 0 6px; font-size:1rem; }
    .row   { display:flex; gap:6px; margin:4px 0; }
    button { flex:1; padding:13px 4px; font-size:0.9rem; border:none;
             border-radius:8px; cursor:pointer; background:#4cf;
             color:#111; font-weight:bold; user-select:none;
             -webkit-user-select:none; }
    button:active { opacity:0.65; }
    .red   { background:#f55 !important; color:#fff !important; }
    .grn   { background:#5d5 !important; color:#111 !important; }
    .gray  { background:#555 !important; color:#eee !important; }
    .amber { background:#fa4 !important; color:#111 !important; }
    #status{ text-align:center; background:#222; border-radius:6px;
             padding:7px; margin:6px 0; color:#fa4; font-weight:bold; }
    pre    { background:#222; padding:10px; border-radius:6px;
             font-size:0.75rem; white-space:pre-wrap; max-height:180px;
             overflow-y:auto; margin:4px 0; }
    label  { font-size:0.82rem; color:#aaa; display:block; margin-top:6px; }
    input[type=range] { width:100%; }
  </style>
  <script>
    let lastDir = '', lastSpd = 0;
    let lastCmdTime = 0;
    const CMD_INTERVAL_MS = 80; // max ~12 commands/sec

    function cmd(path, cb) {
      fetch(path).then(r => r.text()).then(t => {
        document.getElementById('status').innerText = t;
        if (cb) cb(t);
      });
    }

    // Virtual thumb joystick
    const JOY = (() => {
      let canvas, ctx, cx, cy, outerR, nubR = 32;
      let active = false, jX, jY;

      function draw() {
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        // outer ring
        ctx.beginPath(); ctx.arc(cx, cy, outerR, 0, Math.PI*2);
        ctx.strokeStyle = '#4cf'; ctx.lineWidth = 3; ctx.stroke();
        ctx.fillStyle = 'rgba(68,204,255,0.07)'; ctx.fill();
        // direction labels
        ctx.fillStyle = '#4cf'; ctx.font = '14px sans-serif'; ctx.textAlign = 'center';
        ctx.fillText('\u25b2', cx, cy - outerR + 18);
        ctx.fillText('\u25bc', cx, cy + outerR - 6);
        ctx.fillText('\u25c4', cx - outerR + 12, cy + 5);
        ctx.fillText('\u25ba', cx + outerR - 8, cy + 5);
        // nub
        ctx.beginPath(); ctx.arc(jX, jY, nubR, 0, Math.PI*2);
        ctx.fillStyle = active ? '#4cf' : '#555'; ctx.fill();
        ctx.strokeStyle = active ? '#9ef' : '#888'; ctx.lineWidth = 2; ctx.stroke();
      }

      function calcCmd(dx, dy) {
        const dist = Math.sqrt(dx*dx + dy*dy);
        if (dist < outerR * 0.15) return {dir:'stop', spd:0};
        const maxSpd = parseInt(document.getElementById('spd').value);
        const t = Math.min((dist - outerR * 0.15) / (outerR * 0.85), 1); // smooth ramp from dead zone edge
        const spd = Math.round((60 + (maxSpd - 60) * t) / 5) * 5; // quantize to ±5 to suppress micro-updates
        const dir = Math.abs(dy) >= Math.abs(dx)
          ? (dy < 0 ? 'fwd' : 'rev')
          : (dx > 0 ? 'right' : 'left');
        return {dir, spd};
      }

      function sendCmd(dx, dy) {
        const {dir, spd} = calcCmd(dx, dy);
        const now = Date.now();
        if (dir === lastDir && spd === lastSpd) return;
        if (now - lastCmdTime < CMD_INTERVAL_MS) return; // rate limit
        lastDir = dir; lastSpd = spd; lastCmdTime = now;
        cmd('/drive/' + dir + (dir !== 'stop' ? '?spd=' + spd : ''));
      }

      function move(clientX, clientY) {
        const rect = canvas.getBoundingClientRect();
        let dx = clientX - rect.left - cx;
        let dy = clientY - rect.top - cy;
        const dist = Math.sqrt(dx*dx + dy*dy);
        if (dist > outerR) { dx = dx/dist*outerR; dy = dy/dist*outerR; }
        jX = cx + dx; jY = cy + dy;
        sendCmd(dx, dy); draw();
      }

      function release() {
        active = false; jX = cx; jY = cy;
        if (lastDir !== 'stop') { lastDir = 'stop'; lastSpd = 0; cmd('/drive/stop'); }
        draw();
      }

      return {
        init() {
          canvas = document.getElementById('joystick');
          ctx = canvas.getContext('2d');
          cx = canvas.width/2; cy = canvas.height/2;
          outerR = cx - 14; jX = cx; jY = cy;
          draw();
          canvas.addEventListener('mousedown', e => { active=true; move(e.clientX,e.clientY); });
          canvas.addEventListener('mousemove', e => { if(active) move(e.clientX,e.clientY); });
          ['mouseup','mouseleave'].forEach(ev => canvas.addEventListener(ev, release));
          canvas.addEventListener('touchstart', e => {
            e.preventDefault(); active=true; move(e.touches[0].clientX,e.touches[0].clientY);
          }, {passive:false});
          canvas.addEventListener('touchmove', e => {
            e.preventDefault(); move(e.touches[0].clientX,e.touches[0].clientY);
          }, {passive:false});
          ['touchend','touchcancel'].forEach(ev =>
            canvas.addEventListener(ev, e => { e.preventDefault(); release(); }, {passive:false}));
        }
      };
    })();

    function pollSensors() {
      fetch('/sensors').then(r => r.text()).then(t =>
        document.getElementById('sensorBox').innerText = t);
    }

    window.onload = () => {
      JOY.init();
      setInterval(pollSensors, 1800);
      pollSensors();
    };
  </script>
</head><body>
  <h1>&#129302; Ezekiel's Robot</h1>
  <div id="status">Ready</div>

  <h2>&#127922; Operating Mode</h2>
  <div class="row">
    <button class="gray" onclick="cmd('/mode/manual')">&#128073; Manual</button>
    <button class="grn"  onclick="cmd('/mode/maze')">&#127992; Maze Solve</button>
    <button class="grn"  onclick="cmd('/mode/line')">&#11835; Line Follow</button>
    <button class="amber" onclick="cmd('/calibrate/line')">&#127919; Cal IR</button>
    <button class="red"  onclick="cmd('/mode/idle')">&#9632; Stop</button>
  </div>

  <h2>&#127918; Driver Controls</h2>
  <canvas id="joystick" width="220" height="220"
          style="display:block;margin:10px auto;touch-action:none;cursor:pointer;"></canvas>
  <label>Max Speed: <span id="spdVal">255</span>
    <input id="spd" type="range" min="80" max="255" value="255"
           oninput="document.getElementById('spdVal').innerText=this.value">
  </label>
  <label>IR Threshold: <span id="thrVal">2000</span>
    <input id="thr" type="range" min="0" max="4095" value="2000"
           oninput="document.getElementById('thrVal').innerText=this.value; cmd('/threshold?val='+this.value)">
  </label>

  <h2>&#128295; Quick Controls</h2>
  <div class="row">
    <button onclick="cmd('/horn')">&#128266; Horn</button>
    <button onclick="cmd('/leds/on')">&#128161; LEDs On</button>
    <button onclick="cmd('/leds/off')">&#127761; LEDs Off</button>
  </div>

  <h2>&#9989; Functional Tests</h2>
  <div class="row">
    <button class="amber" onclick="cmd('/test/1')">1 Horn</button>
    <button class="amber" onclick="cmd('/test/2')">2 LEDs</button>
    <button class="amber" onclick="cmd('/test/3')">3 Light</button>
    <button class="amber" onclick="cmd('/test/4')">4 Motors</button>
  </div>
  <div class="row">
    <button class="amber" onclick="cmd('/test/5')">5 Line</button>
    <button class="amber" onclick="cmd('/test/6')">6 Ultrasonic</button>
    <button class="gray"  onclick="cmd('/test/all')">&#9654;&#9654; Run All Tests</button>
  </div>

  <h2>&#128203; Test Results</h2>
  <pre id="results">)rawliteral";

    html += testResults.length() ? testResults : "(no tests run yet)";

    html += R"rawliteral(</pre>

  <h2>&#128268; Live Sensors</h2>
  <pre id="sensorBox">Loading...</pre>

</body></html>)rawliteral";

    server.send(200, "text/html", html);
}

// Drive endpoint 
static void handleDrive()
{
    if (robotMode != MODE_MANUAL) {
        server.send(200, "text/plain", "Switch to Manual mode first");
        return;
    }
    int spd = server.hasArg("spd") ? constrain(server.arg("spd").toInt(), 0, 255)
                                   : webSpeed;
    webSpeed   = spd;
    lastWebCmd = millis();

    String d = server.uri().substring(7); // after "/drive/"
    if      (d == "fwd")   driveForward(spd);
    else if (d == "rev")   driveReverse(spd);
    else if (d == "left")  pivotLeft(spd);
    else if (d == "right") pivotRight(spd);
    else                   motorsStop();

    server.send(200, "text/plain", "Drive: " + d);
}

// Mode endpoint 
static void handleMode()
{
    motorsStop();
    lastWebCmd = 0;  // force watchdog to fire until a fresh drive command arrives
    String m = server.uri().substring(6); // after "/mode/"
    if      (m == "manual") { robotMode = MODE_MANUAL;      server.send(200, "text/plain", "Manual (driver) mode"); }
    else if (m == "maze")   { robotMode = MODE_MAZE_SOLVE;   server.send(200, "text/plain", "Maze-solve autonomous started"); }
    else if (m == "line")   { lineFollowCalibrate(); robotMode = MODE_LINE_FOLLOW;  server.send(200, "text/plain", "Line-follow started (calibrated ON tape) — check serial"); }
    else                    { robotMode = MODE_IDLE;          server.send(200, "text/plain", "Idle / stopped"); }
}

// Test endpoints 
static void handleTest()
{
    robotMode = MODE_IDLE;
    motorsStop();
    String t = server.uri().substring(6); // after "/test/"

    if      (t == "1")   { testHorn();         server.send(200, "text/plain", "TEST 1 Horn done"); }
    else if (t == "2")   { testLEDs();          server.send(200, "text/plain", "TEST 2 LEDs done"); }
    else if (t == "3")   { testLightSensor();   server.send(200, "text/plain", "TEST 3 Light Sensor done"); }
    else if (t == "4")   { testMotors();        server.send(200, "text/plain", "TEST 4 Motors done"); }
    else if (t == "5")   { testLineSensor();    server.send(200, "text/plain", "TEST 5 Line Sensor done"); }
    else if (t == "6")   { testUltrasonic();    server.send(200, "text/plain", "TEST 6 Ultrasonic done"); }
    else if (t == "all") { runAllTests();       server.send(200, "text/plain", "All tests complete — reload to see results"); }
    else                   server.send(400, "text/plain", "Unknown test");
}

// Calibrate line sensor endpoint
static void handleCalibrateIR()
{
    motorsStop();
    lineFollowCalibrate();
    server.send(200, "text/plain", "Calibrated — threshold=" + String(webThreshold));
}

// Threshold endpoint
static void handleThreshold()
{
    if (server.hasArg("val"))
        webThreshold = constrain(server.arg("val").toInt(), 0, 4095);
    server.send(200, "text/plain", "Threshold: " + String(webThreshold));
}

// Quick controls
static void handleHorn()    { beep(3, 150, 100); server.send(200, "text/plain", "Beeped!"); }
static void handleLedsOn()  { digitalWrite(FRONTLAMPS, HIGH); digitalWrite(REARLAMPS, HIGH); server.send(200, "text/plain", "LEDs ON"); }
static void handleLedsOff() { digitalWrite(FRONTLAMPS, LOW);  digitalWrite(REARLAMPS, LOW);  server.send(200, "text/plain", "LEDs OFF"); }

// Sensor snapshot 
static void handleSensors()
{
    digitalWrite(TRIG, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    long dist = pulseIn(ECHO, HIGH, 25000UL) / 58L;

    int ir    = analogRead(IR_RECEIVE);
    int light = analogRead(DAYNIGHT);
    bool onLine = ir > webThreshold;

    const char* modeStr[] = {"IDLE", "MANUAL", "MAZE_SOLVE", "LINE_FOLLOW"};
    String out  = "Mode       : "; out += modeStr[(int)robotMode]; out += "\n";
    out += "IR sensor  : "; out += ir;
    out += (onLine ? "  [ON LINE]\n" : "  [off line]\n");
    out += "IR threshold: "; out += webThreshold; out += "\n";
    out += "Light(ADC) : "; out += light; out += "\n";
    out += "Ultrasonic : ";
    out += (dist > 0 ? String(dist) + " cm" : "no echo"); out += "\n";

    server.send(200, "text/plain", out);
}

// Route registration 
void setupWebServer()
{
    server.on("/",              handleRoot);
    server.on("/sensors",       handleSensors);

    server.on("/horn",          handleHorn);
    server.on("/leds/on",       handleLedsOn);
    server.on("/leds/off",      handleLedsOff);

    server.on("/drive/fwd",     handleDrive);
    server.on("/drive/rev",     handleDrive);
    server.on("/drive/left",    handleDrive);
    server.on("/drive/right",   handleDrive);
    server.on("/drive/stop",    handleDrive);

    server.on("/calibrate/line", handleCalibrateIR);
    server.on("/threshold",     handleThreshold);

    server.on("/mode/idle",     handleMode);
    server.on("/mode/manual",   handleMode);
    server.on("/mode/maze",     handleMode);
    server.on("/mode/line",     handleMode);

    server.on("/test/1",        handleTest);
    server.on("/test/2",        handleTest);
    server.on("/test/3",        handleTest);
    server.on("/test/4",        handleTest);
    server.on("/test/5",        handleTest);
    server.on("/test/6",        handleTest);
    server.on("/test/all",      handleTest);

    server.begin();
    Serial.println("Web server started — open the IP above in a browser or phone.");
}

void webServerLoop()
{
    server.handleClient();
}
