#include <Wire.h>
#include <MPU6050_light.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

MPU6050 mpu(Wire);

//---------------- WiFi Setup ----------------
const char* ssid = "Wokwi-GUEST"; //Change accordingly
const char* password = "";
ESP8266WebServer server(80);

//-- Rep Counter --
int repCount = 0;
bool movingUp = false;
float thresholdUp = 2;    // adjust for your movement
float thresholdDown = 0; // adjust for your movement

const char homepage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP8266 Secure Portal</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { background-color: #0e0e0e; color: #f5f5f5; font-family: Arial, sans-serif; display: flex; flex-direction: column; align-items: center; }
    h1 { margin-top: 20px; font-size: 26px; }
    .login-box { margin-top: 30px; background: #1c1c1c; padding: 20px; border-radius: 10px; width: 300px; box-shadow: 0px 0px 10px #222; }
    input[type=text], input[type=password] { width: 100%; padding: 10px; margin: 8px 0; background: #333; color: #fff; border: none; border-radius: 5px; }
    button { background-color: #4CAF50; color: white; padding: 10px; width: 100%; border: none; border-radius: 5px; font-size: 16px; cursor: pointer; }
    button:hover { background-color: #45a049; }
    .error { color: #ff4d4d; margin-top: 10px; }
  </style>
</head>
<body>
  <h1>Welcome to the Secure Portal</h1>
  <div class="login-box">
    <form action="/login" method="post">
      <input type="text" name="username" placeholder="Enter Username" required>
      <input type="password" name="password" placeholder="Enter Password" required>
      <button type="submit">Authorize</button>
    </form>
    %ERROR%
  </div>
</body>
</html>
)rawliteral";

const char secretPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Authorized</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { background-color: #000; color: #00ff88; font-family: Arial, sans-serif; text-align: center; }
    h1 { font-size: 32px; margin-top: 40px; }
    p { font-size: 22px; margin-top: 20px; }
  </style>
</head>
<body>
  <h1> Access Granted</h1>
  <p>Welcome to the authorized section.</p>
  <p>Your hard work paid off!</p>
</body>
</html>
)rawliteral";

void handleRoot(bool showError = false) {
  String page = homepage;
  page.replace("%ERROR%", showError ? "<p class='error'>Invalid Credentials</p>" : "");
  server.send(200, "text/html", page);
}

void handleLogin() {
  if(repCount >= 7){
    server.send(200, "text/html", secretPage);
  } else {
    handleRoot(true);
  }
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Reps:");
  display.setCursor(0,30);
  display.print(repCount);
  display.display();
}


void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);     
  Wire.setClock(100000);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("SSD1306 not found!");
    while(1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Initializing...");
  display.display();

 
  byte status = mpu.begin();
  if(status != 0){
    Serial.println("MPU6050 init failed!");
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("MPU6050 init failed!");
    display.display();
    while(1);
  }

  Serial.println("Calibrating MPU6050...");
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Calibrating...");
  display.display();
  delay(500);
  mpu.calcOffsets(true); // auto-calibrate
  Serial.println("Calibration done!");
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Calibration done!");
  display.display();
  delay(1000);

  
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while(WiFi.status() != WL_CONNECTED){
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  
  server.on("/", [](){ handleRoot(false); });
  server.on("/login", HTTP_POST, handleLogin);
  server.begin();
  Serial.println("HTTP server started.");

  updateOLED();
}


void loop() {
  mpu.update();
  float zAcc = mpu.getAccZ();

  // Serial 
  Serial.print("Z Acc: ");
  Serial.println(zAcc);

  // Rep counting
  if(zAcc > thresholdUp && !movingUp){
    movingUp = true;
  }
  if(zAcc < thresholdDown && movingUp){
    movingUp = false;
    repCount++;
    updateOLED();
    Serial.print("Reps: ");
    Serial.println(repCount);
  }

  server.handleClient();
  delay(50);
}
