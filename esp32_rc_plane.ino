#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Servo.h>

#define SERVO_L_PIN 18
#define SERVO_R_PIN 19
#define THROTTLE_PIN 21

#define SERVO_MIN_PULSE 500
#define SERVO_MAX_PULSE 2500

const char *ssid = "ESP32_RC_Plane";
const char *password = "";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

Servo servoLeft, servoRight, throttle;

volatile int targetLeft = 90, targetRight = 90, targetThrottle = 0; // input degrees (0-180 for servos, 0-180 for throttle)
int currentLeft = 90, currentRight = 90, currentThrottle = 0;

unsigned long lastUpdate = 0;
const int updateInterval = 15; // ms (smoother movement at 60+ FPS)
const int stepPerInterval = 2; // controls "speed" of movement

String getWebPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset=\"UTF-8\">
  <title>ESP32 RC Plane Controller</title>
  <style>
    body { font-family: Arial; text-align:center; margin:20px; }
    #joystickContainer { width:220px; height:220px; background:#ddd; border-radius:10px; margin:auto; position:relative;}
    #joystick { width:80px; height:80px; background:#66a; position:absolute; top:70px; left:70px; border-radius:40px; }
    #slider { width: 200px; }
  </style>
</head>
<body>
  <h2>ESP32 RC Plane Controller</h2>
  <div id=\"joystickContainer\">
    <div id=\"joystick\"></div>
  </div>
  <br>
  <label>Throttle:</label><br>
  <input type=\"range\" min=\"0\" max=\"180\" value=\"0\" id=\"slider\"/>
  <span id=\"throttle_val\">0</span>
  <script>
    const ws = new WebSocket('ws://' + location.host + '/ws');
    let joy = document.getElementById("joystick");
    let container = document.getElementById("joystickContainer");
    let dragging = false;
    let centerX = container.offsetLeft + container.offsetWidth/2;
    let centerY = container.offsetTop + container.offsetHeight/2;
    let joyPos = {x:110, y:110}; // middle, in px

    function sendJoy(pos) {
      // Normalize to -90..+90 for each axis
      let dx = Math.round((pos.x-110)/1.1); // -100..100 => -90..90
      let dy = Math.round((110-pos.y)/1.1);
      ws.send(JSON.stringify({lx: dx, rx: dy}));
    }
    function sendThrottle(val) {
      ws.send(JSON.stringify({throttle: val}));
    }
    function moveJoy(x, y) {
      // Clamp inside container (radius 100)
      let dx = x-110, dy = y-110, dist = Math.sqrt(dx*dx + dy*dy);
      if(dist > 100) { dx*=100/dist; dy*=100/dist;}
      joy.style.left = (110+dx-40)+'px';
      joy.style.top = (110+dy-40)+'px';
      sendJoy({x:110+dx, y:110+dy});
    }
    joy.onmousedown = e => { dragging=true; };
    window.onmouseup = e => { dragging=false; };
    container.onmousemove = e => {
      if(dragging) {
        let rect = container.getBoundingClientRect();
        let x = e.clientX - rect.left, y = e.clientY - rect.top;
        moveJoy(x, y);
      }
    };
    // For touch screens
    joy.ontouchstart = e => { dragging=true; };
    window.ontouchend = e => { dragging=false; };
    container.ontouchmove = e => {
      e.preventDefault();
      if(dragging && e.touches.length === 1) {
        let rect = container.getBoundingClientRect();
        let x = e.touches[0].clientX - rect.left, y = e.touches[0].clientY - rect.top;
        moveJoy(x, y);
      }
    };

    // Static joystick, does not return to center. On click in container, moves there
    container.onclick = e => {
      let rect = container.getBoundingClientRect();
      let x = e.clientX - rect.left, y = e.clientY - rect.top;
      moveJoy(x, y);
    }

    let slider = document.getElementById("slider");
    let tval = document.getElementById("throttle_val");
    slider.oninput = e => {
      tval.innerText = slider.value;
      sendThrottle(Number(slider.value));
    };
  </script>
</body>
</html>
)rawliteral";
}

void notifyClients() { /* For two-way if needed */ }

void handleWsMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String msg = "";
    for(size_t i=0;i<len;i++) msg+=(char)data[i];
    if(msg.indexOf("lx")!=-1) {
      int sep1 = msg.indexOf(':');
      int sep2 = msg.indexOf(',', sep1+1);
      int sep3 = msg.indexOf("rx");
      int lx = msg.substring(sep1+1, sep2).toInt();
      int rx = msg.substring(msg.lastIndexOf(':')+1, msg.indexOf('}')).toInt();
      // Map lx to servo left (wing), rx to right
      targetLeft = constrain(90 + lx, 0, 180);
      targetRight = constrain(90 + rx, 0, 180);
    } else if(msg.indexOf("throttle")!=-1) {
      int sep = msg.indexOf(':');
      int val = msg.substring(sep+1, msg.indexOf('}')).toInt();
      targetThrottle = constrain(val, 0, 180);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                        void *arg, uint8_t *data, size_t len) {
  switch(type) {
    case WS_EVT_CONNECT: break;
    case WS_EVT_DISCONNECT: break;
    case WS_EVT_DATA: handleWsMessage(arg, data, len); break;
    case WS_EVT_PONG: break;
    case WS_EVT_ERROR: break;
  }
}

void setup() {
  Serial.begin(115200);

  // 1. Start WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  // 2. Attach servos & throttle
  servoLeft.attach(SERVO_L_PIN, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
  servoRight.attach(SERVO_R_PIN, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
  throttle.attach(THROTTLE_PIN, SERVO_MIN_PULSE, SERVO_MAX_PULSE);

  // 3. Web server, WebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200,"text/html",getWebPage());
  });
  server.begin();
}

void loop() {
  // Smooth transition for each channel
  unsigned long now = millis();
  if(now - lastUpdate > updateInterval) {
    if(currentLeft < targetLeft) currentLeft = min(currentLeft+stepPerInterval, targetLeft);
    else if(currentLeft > targetLeft) currentLeft = max(currentLeft-stepPerInterval, targetLeft);

    if(currentRight < targetRight) currentRight = min(currentRight+stepPerInterval, targetRight);
    else if(currentRight > targetRight) currentRight = max(currentRight-stepPerInterval, targetRight);

    if(currentThrottle < targetThrottle) currentThrottle = min(currentThrottle+stepPerInterval, targetThrottle);
    else if(currentThrottle > targetThrottle) currentThrottle = max(currentThrottle-stepPerInterval, targetThrottle);

    servoLeft.write(currentLeft);
    servoRight.write(currentRight);
    throttle.write(currentThrottle);

    lastUpdate = now;
  }
}