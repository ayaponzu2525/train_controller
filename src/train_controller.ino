#include <WiFi.h>
#include <WebServer.h>
#include <M5Stack.h>
//2 = pwm, 25 = in1, 26 = in2

const char* ssid = "PI-AP";
const char* password = "6v5wh465333px";

// モーターピン
const int PWM_PIN = 5;
const int IN1_PIN = 25;
const int IN2_PIN = 26;

WebServer server(80);
int pwm_value = 200;  // 初期速度（反転前の値）

void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html, body {
        margin: 0;
        padding: 0;
        font-family: sans-serif;
        background-color: #f0f0f0;
        height: 100%;
        overflow: hidden;
      }

      .layout {
        display: flex;
        flex-direction: row;
        justify-content: space-evenly;
        align-items: center;
        height: 100vh;
        padding: 10px;
      }

      .reverser {
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 10px;
      }

      .reverser button {
        font-size: 20px;
        padding: 10px 50px;
        width: 300px;
        height: 70px;
      }

      .throttle {
        display: flex;
        flex-direction: column;
        align-items: center;
      }

      .throttle input[type=range] {
        writing-mode: bt-lr;
        -webkit-appearance: slider-vertical;
        width: 40px;
        height: 240px;
      }

      .throttle .label {
        font-size: 18px;
        margin: 10px 0;
      }

      .stop-button {
        position: absolute;
        bottom: 20px;
        left: 50%;
        transform: translateX(-50%);
        font-size: 20px;
        padding: 10px 40px;
        background: #fff;
        border: 2px solid #000;
      }
    </style>
  </head>
  <body>
    <div class="layout">
      <!-- リバーサー -->
      <div class="reverser">
        <button onclick="setDirection('forward')">🚄 前進</button>
        <button onclick="setDirection('neutral')">⏸ 中立</button>
        <button onclick="setDirection('backward')">🚋 後退</button>
      </div>

      <!-- スロットル -->
      <div class="throttle">
        <div class="label">速度: <span id="speedValue">200</span></div>
        <input type="range" min="0" max="255" value="200" id="speed" onchange="setSpeed(this.value)">
      </div>
    </div>

    <!-- 停止ボタン（画面中央下部） -->
    <button class="stop-button" onclick="sendCommand('stop')">⏹ 停止</button>

    <script>
      function sendCommand(cmd) {
        fetch('/' + cmd);
      }

      function setDirection(dir) {
        sendCommand(dir);
      }

      function setSpeed(val) {
        document.getElementById('speedValue').innerText = val;
        fetch('/speed?val=' + val);
      }
    </script>
  </body>
  </html>

  )rawliteral";

  server.send(200, "text/html", html);
}

void setup() {
  M5.begin();
  Serial.begin(115200);

  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Connecting WiFi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }

  M5.Lcd.println("\nConnected!");
  M5.Lcd.print("IP: ");
  M5.Lcd.println(WiFi.localIP());

  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  ledcSetup(0, 1000, 8);      // channel 0, 1kHz, 8bit
  ledcAttachPin(PWM_PIN, 0);
  ledcWrite(0, 255 - pwm_value);  // 論理反転

  server.on("/", handleRoot);

  server.on("/forward", []() {
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(0, 255 - pwm_value);
    Serial.println("Forward");
    server.send(204);
  });

  server.on("/neutral", []() {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(0, 255 - pwm_value);  // 惰行は速度維持でもOK、または0でも
    Serial.println("Neutral");
    server.send(204);
  });


  server.on("/backward", []() {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, HIGH);
    ledcWrite(0, 255 - pwm_value);
    Serial.println("Backward");
    server.send(204);
  });

  server.on("/stop", []() {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    ledcWrite(0, 0);
    Serial.println("Stop");
    server.send(204);
  });

  server.on("/speed", []() {
    if (server.hasArg("val")) {
      pwm_value = server.arg("val").toInt();
      ledcWrite(0, 255 - pwm_value);  // 論理反転
      Serial.print("Speed set to ");
      Serial.println(pwm_value);
    }
    server.send(204);
  });

  server.begin();
  M5.Lcd.println("Server Ready");
}

void loop() {
  server.handleClient();
}