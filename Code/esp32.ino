#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Adafruit_MPU6050 mpu;

//--------------------------
//|   Change Acordingly    |
//--------------------------

const char* ssid = "Wokwi-GUEST";
const char* password = "";


WebServer server(80);


int repCount = 0;
bool movingUp = false;
bool movingDown = false;
float thresholdUp = 2.0; //Adjust accordingly
float thresholdDown = -2.0;

const char homepage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Secure Portal</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background-color: #0e0e0e;
      color: #f5f5f5;
      font-family: Arial, sans-serif;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    h1 {
      margin-top: 20px;
      font-size: 26px;
    }
    .login-box {
      margin-top: 30px;
      background: #1c1c1c;
      padding: 20px;
      border-radius: 10px;
      width: 300px;
      box-shadow: 0px 0px 10px #222;
    }
    input[type=text], input[type=password] {
      width: 100%;
      padding: 10px;
      margin: 8px 0;
      background: #333;
      color: #fff;
      border: none;
      border-radius: 5px;
    }
    button {
      background-color: #4CAF50;
      color: white;
      padding: 10px;
      width: 100%;
      border: none;
      border-radius: 5px;
      font-size: 16px;
      cursor: pointer;
    }
    button:hover {
      background-color: #45a049;
    }
    .error {
      color: #ff4d4d;
      margin-top: 10px;
    }
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
                    
// Secret authorized page
const char secretPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Authorized</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background-color: #000;
      color: #00ff88;
      font-family: Arial, sans-serif;
      text-align: center;
    }
    h1 {
      font-size: 32px;
      margin-top: 40px;
    }
    p {
      font-size: 22px;
      margin-top: 20px;
    }
  </style>
</head>
<body>
  <h1> Access Granted</h1>
  <p>
  <br>
  <br>
  Welcome to the authorized section.</p>
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
  // Secret unlocking mechanism 
  if (repCount >= 7) {
    server.send(200, "text/html", secretPage);
  } else {
    handleRoot(true); // Always "Invalid Credentials"
  }
}


void updateOLED() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Reps:");
  display.setCursor(0, 30);
  display.print(repCount);
  display.display();
}

void setup() {
  Serial.begin(115200);

  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 not found!");
    while (1);
  }
  display.clearDisplay();
  updateOLED();

  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("Web Server IP: ");
  Serial.println(WiFi.localIP());

  
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);


  server.on("/", []() { handleRoot(false); });
  server.on("/login", HTTP_POST, handleLogin);

 
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float zAcc = a.acceleration.z;

  
  if (zAcc > thresholdUp && !movingUp) {
    movingUp = true;
    movingDown = false;
  }

  
  if (zAcc < thresholdDown && movingUp) {
    movingDown = true;
    repCount++;
    movingUp = false;
    updateOLED(); 
    Serial.print("Reps: ");
    Serial.println(repCount);
  }

  
  server.handleClient();
}
