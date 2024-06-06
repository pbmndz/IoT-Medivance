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
// Preferences
Preferences preferences;

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
// button
const int analogPin = 39; 
int analogValue = 0; 

// Wifi //
// Replace with your network credentials 
// local wifi
const char* ssid = "DoseRx";
const char* password = "DoseRx123!";
// Flash Memory preference saved SSID and Password
String SSID;
String PASS;
WebServer server(80); // Create a WebServer object on port 80

// bool to run once
//for checkAndReconnectWiFi
bool reconnectAttempted = false; 
//for loop
bool wasConnected = false;
bool pressed = false;
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
// qr
bool userConnectedToESP = false;
bool messageDisplayed = false;
bool clearDisplayQR = true;

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
      clearDisplayQR = false;
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

    oled.clearDisplay(); 
    oled.display();

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
      clearDisplayQR = false;
      Serial.println();
      Serial.print("Connected with IP: ");
      Serial.println(WiFi.localIP());
      handleWiFiConnected();
      fireBase();
  } 
}

void checkAndReconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED && !reconnectAttempted) {
    Serial.println("WiFi is disconnected. Trying to reconnect...");
    SavedCredentials(); // Try to reconnect using saved credentials
    reconnectAttempted = true;
    userConnectedToESP = false;
    messageDisplayed = false;
    oled.clearDisplay(); 
    oled.display();
    // Check if reconnection was successful
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconnected successfully!");
      clearDisplayQR = false;
    } else {
      Serial.println("Reconnection failed. Please enter new SSID/PASS.");
    }
    // Reset the flag after the reconnection process
    reconnectAttempted = false;
  }
}

// display
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

// 'qr', 128x64px
const unsigned char epd_bitmap_qr [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x1b, 0xfc, 0x3c, 0x18, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x00, 0x19, 0xf8, 0x1c, 0x18, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x7f, 0x99, 0xc2, 0x00, 0x39, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x7f, 0x99, 0xc2, 0x00, 0x39, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x60, 0x98, 0xf0, 0x6c, 0x99, 0x06, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x60, 0x98, 0x70, 0x0f, 0x99, 0x06, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x60, 0x98, 0x31, 0x8f, 0x99, 0x06, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x60, 0x9c, 0x31, 0xfd, 0x99, 0x06, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x60, 0x9c, 0x31, 0xfd, 0xd9, 0x86, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x7f, 0x98, 0x3e, 0x1f, 0xf9, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x00, 0x18, 0x36, 0x0f, 0xf8, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x00, 0x19, 0x32, 0x6c, 0x98, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x08, 0x12, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x08, 0x13, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x1c, 0x1f, 0x32, 0x6f, 0x1e, 0xc0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x1c, 0x0f, 0xb2, 0x4e, 0x0c, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x93, 0x21, 0xf9, 0x9c, 0x01, 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x44, 0x00, 0x3f, 0xfc, 0x01, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x44, 0x18, 0x3f, 0xfc, 0x19, 0x39, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x82, 0x3c, 0x77, 0xee, 0x18, 0xd8, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x83, 0x3e, 0xf3, 0xe7, 0x18, 0x08, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x73, 0x9f, 0xc8, 0x63, 0x1c, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xe7, 0x3e, 0x00, 0x67, 0x80, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xef, 0x3e, 0x32, 0x7f, 0x81, 0x30, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0x81, 0xc0, 0x0d, 0x80, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x04, 0xc3, 0xc0, 0x04, 0x80, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x03, 0xe7, 0x09, 0xc0, 0x9f, 0xf6, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x03, 0xc3, 0x09, 0xe0, 0x9f, 0xf2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x0f, 0x81, 0x31, 0xe0, 0x79, 0x31, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x33, 0xe1, 0x38, 0x1e, 0x79, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x73, 0xe1, 0x3c, 0x1f, 0x79, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf0, 0x19, 0x3f, 0x9c, 0xf9, 0xc2, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf8, 0x0b, 0x3f, 0x9c, 0xf1, 0xc2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x27, 0x36, 0x63, 0x01, 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x38, 0x83, 0x08, 0x7f, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x10, 0xd9, 0x08, 0x7f, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xf9, 0x13, 0xbf, 0x8f, 0x27, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xfb, 0xa3, 0x1f, 0x9f, 0x27, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x00, 0x1f, 0xc0, 0x10, 0x19, 0x0e, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x00, 0x1c, 0xc0, 0x00, 0x1b, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x7f, 0x98, 0xc9, 0xe0, 0x9f, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x61, 0x99, 0x31, 0xe0, 0x00, 0x02, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x60, 0x99, 0x30, 0xe0, 0x00, 0x02, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x60, 0x9e, 0xf8, 0x7c, 0xe4, 0x41, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x60, 0x9e, 0x7c, 0x7c, 0xe0, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x60, 0x9f, 0x3f, 0x9c, 0x61, 0xc0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x7f, 0x9b, 0x37, 0x4c, 0x01, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x7f, 0x99, 0x32, 0x6c, 0x19, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0x00, 0x19, 0xc8, 0x7c, 0xf8, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x1b, 0xdc, 0x7d, 0xf8, 0x1d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 1040)
const int epd_bitmap_allArray_LEN = 1;
const unsigned char* epd_bitmap_allArray[1] = {
	epd_bitmap_qr
};


// 'qr website', 128x64px
const unsigned char epd_bitmap_qr_website [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x07, 0x9f, 0xf8, 0xe0, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x07, 0x9f, 0xf8, 0x60, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x3f, 0xe6, 0x18, 0x18, 0x6f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x3f, 0xf6, 0x18, 0x18, 0xef, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x37, 0x81, 0x9f, 0xec, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x37, 0x81, 0x9f, 0xec, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x36, 0x19, 0x81, 0xec, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x37, 0x19, 0x81, 0xec, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x37, 0xe6, 0x7e, 0x6c, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x77, 0xe7, 0x3f, 0xee, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x3f, 0xf6, 0x67, 0x9f, 0xef, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x1f, 0xc6, 0x67, 0x1f, 0xe7, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x06, 0x66, 0x66, 0x60, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xfe, 0x67, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xfe, 0x67, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x70, 0x06, 0x7b, 0xbb, 0xb3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x70, 0x06, 0x79, 0x93, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf8, 0xf9, 0x9f, 0xe1, 0x8f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xfc, 0xf9, 0x9f, 0xe1, 0x8f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x01, 0x98, 0x7e, 0x40, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x01, 0x98, 0x7e, 0x60, 0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xf9, 0x81, 0xff, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xf9, 0x81, 0xff, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0f, 0xc7, 0x99, 0xfb, 0xf0, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0f, 0x87, 0x99, 0xf9, 0xf0, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x33, 0x89, 0x86, 0x1e, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x33, 0x89, 0x86, 0x1c, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x86, 0x67, 0x80, 0x6c, 0xcc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x86, 0x67, 0x80, 0x6c, 0xcc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x33, 0x3f, 0x9e, 0x1f, 0x9f, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x32, 0x7f, 0x8e, 0x1f, 0x1f, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0xf1, 0x86, 0x60, 0x00, 0x3c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf1, 0xf1, 0x87, 0xe0, 0x00, 0x7c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xfe, 0x7f, 0xfe, 0x7c, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xfe, 0x3f, 0xfc, 0x7c, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x06, 0x00, 0x78, 0x6c, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x3f, 0xe7, 0x89, 0xe0, 0x7c, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x3f, 0xf7, 0x99, 0xe0, 0x7c, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x76, 0x61, 0xf8, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x36, 0x61, 0xf8, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x36, 0x66, 0x04, 0x40, 0x31, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x36, 0x66, 0x06, 0xe0, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x36, 0x67, 0x81, 0xf0, 0x4c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x30, 0x36, 0x67, 0x81, 0xf0, 0xcc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x3f, 0xf6, 0x7e, 0x18, 0x6c, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x3f, 0xe6, 0x7e, 0x18, 0x6c, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x06, 0x66, 0x64, 0x7f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x06, 0x66, 0x66, 0xff, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};






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



void loop() {
  int currentStationCount = WiFi.softAPgetStationNum();
  bool isConnected = WiFi.status() == WL_CONNECTED; 
  server.handleClient();

// save values for CD74HC4067 
  int value0 = channel0();
// read the value from pins
  analogValue = analogRead(analogPin);

  // If a user has connected
  if (clearDisplayQR == false){
    oled.clearDisplay(); 
    oled.display();
    clearDisplayQR = true;
  }
  else if (currentStationCount >= 1 && !isConnected && userConnectedToESP == false) {
    Serial.println("connected");
    oled.clearDisplay(); 
    oled.display();
    delay(1000);
    oled.drawBitmap(0, 0, epd_bitmap_qr_website, 128, 64, 1);
    oled.display();
    userConnectedToESP = true;
    messageDisplayed = false;
  } else if (currentStationCount < 1 && !isConnected && messageDisplayed == false){
    Serial.println("user has not connected to the ESP32 WiFi.");
    oled.drawBitmap(0, 0, epd_bitmap_qr, 128, 64, 1);
    oled.display();
    messageDisplayed = true; // Set the flag to true after displaying the message
    userConnectedToESP = false;
  }

// check if wifi is interupted
  if (wasConnected && !isConnected) {
    Serial.println("Internet connection was interrupted.");
    checkAndReconnectWiFi();
  }
// button force wifi reset 
  if (analogValue > 2300 && analogValue < 2800 && pressed == false && WiFi.status() == WL_CONNECTED){
    Serial.print("network dissconnected");
    Serial.print(" button value: ");
    Serial.println(analogValue);
    preferences.begin("credentials", false);
    preferences.clear();
    preferences.end();
    WiFi.disconnect();
    pressed = true;
  }

  


  if (WiFi.status() == WL_CONNECTED && Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0 )){
    sendDataPrevMillis = millis();
    int value0 = channel0();
    pressed = false;
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
  // Update the the data 
  wasConnected = isConnected;

}

