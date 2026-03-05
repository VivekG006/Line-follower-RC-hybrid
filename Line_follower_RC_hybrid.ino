#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32_RC";
const char* password = "12345678";

WiFiServer server(80);

// Motor pins
int IN1 = 26;
int IN2 = 27;
int IN3 = 14;
int IN4 = 12;
int ENA = 25;
int ENB = 33;

#define IR1 34
#define IR2 35
#define IR3 32

void setup() {

  Serial.begin(115200);
  delay(100);

  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  pinMode(IR3, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  ledcAttach(ENA, 1000, 8);
  ledcAttach(ENB, 1000, 8);

  WiFi.softAP(ssid, password);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.begin();
  Serial.println("Server started.");
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    Serial.print("Request: ");
    Serial.println(request);

    if (request.indexOf("/control") != -1) {
      int fIndex = request.indexOf("f=");
      int tIndex = request.indexOf("t=");

      if (fIndex != -1 && tIndex != -1) {
        int ampIndex = request.indexOf("&", fIndex);
        int forward = request.substring(fIndex + 2, ampIndex).toInt();
        int turn = request.substring(tIndex + 2).toInt();

        Serial.print("Forward: ");
        Serial.print(forward);
        Serial.print(" | Turn: ");
        Serial.println(turn);

        controlMotors(forward, turn);
      }
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();
    client.println(htmlPage());

    client.stop();
  }
}

void controlMotors(int forward, int turn) {
  int leftSpeed = forward + turn;
  int rightSpeed = forward - turn;

  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  setMotor(IN1, IN2, ENA, leftSpeed);
  setMotor(IN3, IN4, ENB, rightSpeed);
}

void setMotor(int pin1, int pin2, int pwmPin, int speed) {
  if (speed > 0) {
    digitalWrite(pin1, HIGH);
    digitalWrite(pin2, LOW);
    ledcWrite(pwmPin, speed);
  }
  else if (speed < 0) {
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, HIGH);
    ledcWrite(pwmPin, -speed);
  }
  else {
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, LOW);
    ledcWrite(pwmPin, 0);
  }
}

String htmlPage() {
return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<style>
  body {
    background-color: #000;
    margin: 0;
    padding: 0;
    touch-action: none;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    height: 100vh;
    width: 100vw;
    overflow: hidden;
    color: white;
    font-family: sans-serif;
  }

  #rotate-msg {
    display: none;
    text-align: center;
    font-size: 20px;
    padding: 20px;
  }

  .container {
    display: flex;
    justify-content: space-around;
    align-items: center;
    width: 90%;
    max-width: 800px;
  }

  @media screen and (orientation: portrait) {
    #rotate-msg { display: block; }
    .container { display: none; }
  }

  .joystick-zone {
    width: 180px;
    height: 180px;
    background: radial-gradient(circle, #0e3b2a 0%, #000000 70%);
    border-radius: 50%;
    position: relative;
    box-shadow: inset 0 0 15px rgba(0,0,0,0.8), 0 0 20px rgba(0, 255, 150, 0.1);
  }

  .stick {
    width: 60px;
    height: 60px;
    background: #ffffff;
    border-radius: 50%;
    position: absolute;
    top: 60px;
    left: 60px;
    cursor: grab;
    box-shadow: 0 4px 10px rgba(0,0,0,0.6);
  }

  .stick:active { cursor: grabbing; }

</style>
</head>
<body>

<div id="rotate-msg">Please rotate your phone sideways (Landscape Mode) to use the controller.</div>

<div class="container">

  <div class="joystick-zone" id="zone-left">
    <div class="stick" id="stick-left"></div>
  </div>

  <div class="joystick-zone" id="zone-right">
    <div class="stick" id="stick-right"></div>
  </div>

</div>

<script>

let forwardVal = 0;
let turnVal = 0;
let lastTime = 0;
const throttleMs = 50;

function setupStick(stick, zone, axis) {

  const center = { x: 90, y: 90 };
  const maxDist = 60;
  let activeTouchId = null;

  function getPos(e) {
    if (e.touches) {
      for (let i = 0; i < e.touches.length; i++) {
        if (e.touches[i].identifier === activeTouchId) {
          return { x: e.touches[i].clientX, y: e.touches[i].clientY };
        }
      }
      return null;
    }
    return { x: e.clientX, y: e.clientY };
  }

  function applyPosition(cX, cY) {

    let rect = zone.getBoundingClientRect();
    let x = cX - rect.left - center.x;
    let y = cY - rect.top - center.y;

    if (axis === 'y') x = 0;
    if (axis === 'x') y = 0;

    let dist = Math.sqrt(x * x + y * y);

    if (dist > maxDist) {
      if (axis === 'y') y = (y / Math.abs(y)) * maxDist;
      if (axis === 'x') x = (x / Math.abs(x)) * maxDist;
    }

    stick.style.transform = `translate(${x}px, ${y}px)`;

    if (axis === 'y') forwardVal = Math.round(-(y / maxDist) * 255);
    if (axis === 'x') turnVal = Math.round((x / maxDist) * 255);

    let now = Date.now();

    if (now - lastTime > throttleMs) {
      sendData();
      lastTime = now;
    }

  }

  zone.addEventListener('touchstart', function(e) {

    e.preventDefault();

    if (activeTouchId !== null) return;

    for (let i = 0; i < e.changedTouches.length; i++) {

      let t = e.changedTouches[i];
      let rect = zone.getBoundingClientRect();

      if (
        t.clientX >= rect.left &&
        t.clientX <= rect.right &&
        t.clientY >= rect.top &&
        t.clientY <= rect.bottom
      ) {
        activeTouchId = t.identifier;
        applyPosition(t.clientX, t.clientY);
        break;
      }
    }

  }, { passive: false });


  document.addEventListener('touchmove', function(e) {

    if (activeTouchId === null) return;

    e.preventDefault();

    let pos = getPos(e);

    if (pos) applyPosition(pos.x, pos.y);

  }, { passive: false });


  document.addEventListener('touchend', function(e) {

    for (let i = 0; i < e.changedTouches.length; i++) {

      if (e.changedTouches[i].identifier === activeTouchId) {

        activeTouchId = null;
        resetStick();
        break;

      }

    }

  });

  document.addEventListener('touchcancel', function(e) {

    for (let i = 0; i < e.changedTouches.length; i++) {

      if (e.changedTouches[i].identifier === activeTouchId) {

        activeTouchId = null;
        resetStick();
        break;

      }

    }

  });

  zone.addEventListener('mousedown', function(e) {

    function onMove(e) { applyPosition(e.clientX, e.clientY); }

    function onUp() {
      document.removeEventListener('mousemove', onMove);
      document.removeEventListener('mouseup', onUp);
      resetStick();
    }

    document.addEventListener('mousemove', onMove);
    document.addEventListener('mouseup', onUp);

    applyPosition(e.clientX, e.clientY);

  });

  function resetStick() {

    stick.style.transform = 'translate(0px, 0px)';

    if (axis === 'y') forwardVal = 0;
    if (axis === 'x') turnVal = 0;

    sendData();

  }

}

setupStick(
  document.getElementById('stick-left'),
  document.getElementById('zone-left'),
  'y'
);

setupStick(
  document.getElementById('stick-right'),
  document.getElementById('zone-right'),
  'x'
);

function sendData() {

  fetch(`/control?f=${forwardVal}&t=${turnVal}`).catch(e => console.log("Connection busy"));

}

</script>

</body>
</html>
)rawliteral";
}