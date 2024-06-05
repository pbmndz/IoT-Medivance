#include <WiFi.h>
#include <Wire.h>
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2lib.h>
#include "ani.h"
#include <WebServer.h>
#include <Preferences.h>

Preferences preferences;

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Insert Firebase project API Key
#define API_KEY "AIzaSyBm0URw8vqpjDtho1QIryIANt81f_SQ4qY"
// Insert RTDB URLefine the RTDB URL 
#define DATABASE_URL "https://esp32-iot-39031-default-rtdb.asia-southeast1.firebasedatabase.app/" 

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;


///////////////////////
//debuging station ahhas


///////////////////////

// lcd
#define SCREEN_WIDTH 128 // OLED width,  in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels
// create an OLED display object connected to I2C
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//animation
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE); 
ani myAni;
int counter = 0; 

// Define the control pins for the CD74HC4067
#define S0 15
#define S1 2
#define S2 4
#define S3 0
#define SIG 36

// Replace with your network credentials
const char* ssid = "DoseRx";
const char* password = "DoseRx123!";

// preference saved SSID and Password
String SSID;
String PASS;
// Create a WebServer object on port 80
WebServer server(80);

// website
void handleRoot() {
  String html = "<!DOCTYPE html>\
  <html>\
  <head>\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\">\
    <style>\
      body {\
        font-family: Arial, sans-serif;\
        margin: 0;\
        padding: 0;\
      }\
      .center-form {\
        display: flex;\
        flex-direction: column;\
        justify-content: center;\
        align-items: center;\
        height: 100vh;\
      }\
      .center-form form {\
        width: 80%;\
        max-width: 300px;\
        font-size: 0.9em;\
        text-align: center;\
      }\
      h1 {\
        font-size: 1.5em;\
      }\
      p {\
        font-size: 0.8em;\
        text-align: center;\
      }\
      input[type=\"text\"],\
      input[type=\"password\"] {\
        width: calc(100% - 20px);\
        height: 25px;\
        font-size: 0.9em;\
        margin-bottom: 10px;\
        padding: 5px;\
      }\
      input[type=\"submit\"] {\
        width: 100%;\
        height: 30px;\
        font-size: 0.9em;\
        background-color: #4CAF50;\
        color: white;\
        border: none;\
        cursor: pointer;\
      }\
      @media only screen and (max-width: 600px) {\
        .center-form form {\
          font-size: 0.8em;\
        }\
        h1 {\
          font-size: 1.2em;\
        }\
        p {\
          font-size: 0.7em;\
        }\
        input[type=\"text\"],\
        input[type=\"password\"] {\
          height: 30px;\
          font-size: 0.8em;\
        }\
        input[type=\"submit\"] {\
          height: 40px;\
          font-size: 0.8em;\
        }\
      }\
    </style>\
  </head>\
  <body>\
    <div class=\"center-form\">\
      <h1>Please Enter your Wifi Credentials</h1>\
      <p>Please enter the correct Wifi SSID and Password:</p>\
      <form action=\"/submit\" method=\"POST\">\
        Wifi SSID:<br>\
        <input type=\"text\" name=\"Wifi SSID\"><br>\
        Password:<br>\
        <input type=\"text\" name=\"Password\"><br><br>\
        <input type=\"submit\" value=\"Submit\">\
      </form>\
    </div>\
  </body>\
  </html>";

  server.send(200, "text/html", html);
}

void Wrong() {
  String tryAgain = "<!DOCTYPE html>\
  <html>\
  <head>\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\">\
    <style>\
      body {\
        font-family: Arial, sans-serif;\
        margin: 0;\
        padding: 0;\
      }\
      .center-form {\
        display: flex;\
        flex-direction: column;\
        justify-content: center;\
        align-items: center;\
        height: 100vh;\
      }\
      .center-form form {\
        width: 80%;\
        max-width: 300px;\
        font-size: 0.9em;\
        text-align: center;\
      }\
      h1 {\
        font-size: 1.5em;\
      }\
      p {\
        font-size: 0.8em;\
        text-align: center;\
      }\
      input[type=\"text\"],\
      input[type=\"password\"] {\
        width: calc(100% - 20px);\
        height: 25px;\
        font-size: 0.9em;\
        margin-bottom: 10px;\
        padding: 5px;\
      }\
      input[type=\"submit\"] {\
        width: 100%;\
        height: 30px;\
        font-size: 0.9em;\
        background-color: #4CAF50;\
        color: white;\
        border: none;\
        cursor: pointer;\
      }\
      @media only screen and (max-width: 600px) {\
        .center-form form {\
          font-size: 0.8em;\
        }\
        h1 {\
          font-size: 1.2em;\
        }\
        p {\
          font-size: 0.7em;\
        }\
        input[type=\"text\"],\
        input[type=\"password\"] {\
          height: 30px;\
          font-size: 0.8em;\
        }\
        input[type=\"submit\"] {\
          height: 40px;\
          font-size: 0.8em;\
        }\
      }\
    </style>\
  </head>\
  <body>\
    <div class=\"center-form\">\
      <h1>Try Again Wrong SSID/Password</h1>\
      <p>Please enter the correct Wifi SSID and Password:</p>\
      <form action=\"/submit\" method=\"POST\">\
        Wifi SSID:<br>\
        <input type=\"text\" name=\"Wifi SSID\"><br>\
        Password:<br>\
        <input type=\"text\" name=\"Password\"><br><br>\
        <input type=\"submit\" value=\"Submit\">\
      </form>\
    </div>\
  </body>\
  </html>";
  server.send(200, "text/html", tryAgain);
}

void handleWiFiConnected() {
  String success = "<!DOCTYPE html>\
  <html>\
  <head>\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\">\
    <style>\
      body {\
        font-family: Arial, sans-serif;\
        margin: 0;\
        padding: 0;\
      }\
      .center-form {\
        display: flex;\
        flex-direction: column;\
        justify-content: center;\
        align-items: center;\
        height: 100vh;\
      }\
      .center-form form {\
        width: 80%;\
        max-width: 300px;\
        font-size: 0.9em;\
        text-align: center;\
      }\
      h1 {\
        font-size: 2em;\
      }\
      p {\
        font-size: 1em;\
        text-align: center;\
      }\
      @media only screen and (max-width: 600px) {\
        .center-form form {\
          font-size: 0.8em;\
        }\
        h1 {\
          font-size: 1.5em;\
        }\
        p {\
          font-size: 0.9 em;\
        }\
    </style>\
  </head>\
  <body>\
    <div class=\"center-form\">\
      <h1>DoseRx is Connected</h1>\
      <p>you may now close the browser and reconnect to your wifi</p>\
    </div>\
  </body>\
  </html>";
  server.send(200, "text/html", success);
}
// end website

// start webserver
void fireBase() {
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    // Sign up 
    if (Firebase.signUp(&config, &auth, "", "")){
      Serial.println("ok");
      signupOK = true;
    }
    else{
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }
    config.token_status_callback = tokenStatusCallback; 
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void handleSubmit() {
  if(server.method() == HTTP_POST) {
    String SSID = server.arg("Wifi SSID");
    String PASS = server.arg("Password");
    Serial.print("SSID: ");
    Serial.println(SSID);
    Serial.print("PASS: ");
    Serial.println(PASS);

    preferences.begin("credentials", false);
    preferences.putString("ssid", SSID);
    preferences.putString("pass", PASS);
    preferences.end();

    WiFi.begin(SSID.c_str(), PASS.c_str());
    unsigned long startMillis = millis();  
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - startMillis >= 15000) {  // If 15 seconds have passed
        Serial.println("Failed to connect to WiFi after 15 seconds.");
        Wrong();
        break;
      }
      Serial.print(".");
      delay(300);
    } 
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("handleSubmit");
      Serial.print("Connected with IP: ");
      Serial.println(WiFi.localIP());
      handleWiFiConnected();
      fireBase();
    } 
  }
}

void webServerStart(){
  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP Address: ");
  Serial.println(IP);
// Set up server routes
  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
// Start server
  server.begin();
  Serial.println("HTTP server started");  
// clear wifi cred
  preferences.begin("credentials", false);
  preferences.clear();
  preferences.end();
  WiFi.disconnect();
}

void SavedCredentials(){
    preferences.begin("credentials", false);
    String savedSSID = preferences.getString("ssid");
    String savedPASS = preferences.getString("pass");
    preferences.end();

    Serial.println();
    Serial.println("trying connecting to saved credentials .........");
    Serial.println(savedSSID);
    Serial.println(savedPASS);

    WiFi.begin(savedSSID.c_str(), savedPASS.c_str());
    unsigned long startMillis = millis();  
    while (WiFi.status() != WL_CONNECTED) {
      if (savedSSID == "" || savedPASS == "" or millis() - startMillis >= 15000){
        
        webServerStart();
        break;
      }
      Serial.print(".");
      delay(300);

    } 
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("Connected with IP: ");
      Serial.println(WiFi.localIP());
      handleWiFiConnected();
      fireBase();
  } 
}


// animation
void animation(){
  // animation
  while(true) { 
    u8g2.clearBuffer(); 
    u8g2.drawXBMP(0, 0, 128, 64, myAni.getBitmap(counter));
    u8g2.sendBuffer(); 
    counter = (counter + 1) % 42; 
    if (counter == 41) { 
      delay(1000);
      break;
    }
  }
}

// sensor value
void selectMuxChannel(int channel) {
  digitalWrite(S0, bitRead(channel, 0));
  digitalWrite(S1, bitRead(channel, 1));
  digitalWrite(S2, bitRead(channel, 2));
  digitalWrite(S3, bitRead(channel, 3));
}

bool channel0() {
    selectMuxChannel(0);
    int analogValue = analogRead(SIG);
    // return analogValue;
    if(analogValue > 350){
      return true;
    } else{
      return false;
    }
}

bool reconnectAttempted = false;
void checkAndReconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED && !reconnectAttempted) {
    Serial.println("WiFi is disconnected. Trying to reconnect...");
    SavedCredentials(); // Try to reconnect using saved credentials
    reconnectAttempted = true;
    // Check if reconnection was successful
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconnected successfully!");
    } else {
      Serial.println("Reconnection failed. Please enter new SSID/PASS.");
      webServerStart();
    }
    // Reset the flag after the reconnection process
    reconnectAttempted = false;
  }
}

void setup() {
  Serial.begin(115200);
  u8g2.begin(); // start the u8g2 library
  
  // CD74HC4067
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SIG, INPUT);

  // initialize with the I2C addr 0x3D (for the 128x64)
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("failed to start SSD1306 OLED"));
    while (1);
  }
  
  SavedCredentials();
  animation();

  oled.clearDisplay();         // clear display
  oled.setTextSize(1);         // set text size
  oled.setTextColor(WHITE);    // set text color
}

bool wasConnected = false;

void loop() {
  bool isConnected = WiFi.status() == WL_CONNECTED; 

  if (wasConnected && !isConnected) {
    Serial.println("Internet connection was interrupted.");
    checkAndReconnectWiFi();
  }

  server.handleClient();
  int value0 = channel0();
  if (WiFi.status() == WL_CONNECTED && Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0 )){
    sendDataPrevMillis = millis();

    int value0 = channel0();

    if (Firebase.RTDB.setInt(&fbdo, "sensor/slot-1", value0)){
      Serial.println();
      Serial.print(value0);
      Serial.print(" success: " );
      Serial.print(fbdo.dataPath());
      Serial.print(" TYPE: " );
      Serial.println(fbdo.dataType());
    }else {
      Serial.print("FAILED");
      Serial.print("REASON: " );
      Serial.println(fbdo.errorReason());
      }
    }
  
  wasConnected = isConnected;
}

