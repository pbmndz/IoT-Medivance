#include <WiFi.h>
#include <Wire.h>
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2lib.h>
#include <WebServer.h>
#include <Preferences.h>
#include "FastLED.h"
#include "ani.h"
#include "esp_sntp.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <ArduinoJson.h>

// Insert Firebase project API Key
#define API_KEY "AIzaSyBmJ0ibz-8MF6HP7H3wJeKhxYx1Rwca17Y"
// Insert RTDB URLefine the RTDB URL 
#define DATABASE_URL "https://mydoserx-app-default-rtdb.firebaseio.com/" 

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
// Preferences
Preferences preferences;

//monitor 
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
//reset
const int reset = 39; 

// stop
const int stopButton = 34; 

// Wifi //
// Replace with your network credentials 
// local wifi
const char* ssid = "DoseRx";
const char* password = "DoseRx123!";
// Flash Memory preference saved SSID and Password
String SSID;
String PASS;
WebServer server(80); // Create a WebServer object on port 80

// flag
bool reconnectAttempted = false;  //for checkAndReconnectWiFi
bool wasConnected = false; //for loop
bool pressed = false;
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
bool userConnectedToESP = false; // qr
bool messageDisplayed = false;
bool clearDisplayQR = true;

// LED Strips
#define NUM_LEDS 5 // How many leds in your strip?
#define DATA_PIN 16
CRGB leds[NUM_LEDS]; // Define the array of leds
//time 
const char* ntpServer = "asia.pool.ntp.org";
const long  gmtOffset_sec = 8 * 3600; // Philippines is GMT +8
const int   daylightOffset_sec = 0; // No daylight saving time in Philippines

void setTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  // Calculate the number of seconds until 5 AM
  struct tm targetTime = timeinfo; // Copy current time structure
  targetTime.tm_hour = 5; // Set target hour to 5 AM
  targetTime.tm_min = 0; // Set target minutes to 0
  targetTime.tm_sec = 0; // Set target seconds to 0

  // Adjust for next day if current hour is past 5 AM
  if(timeinfo.tm_hour >= 5){
    targetTime.tm_mday += 1;
  }

  // Convert both current time and target time to time_t format
  time_t currentTime_t = mktime(&timeinfo);
  time_t targetTime_t = mktime(&targetTime);

  // Calculate difference in seconds
  double secondsUntilTarget = difftime(targetTime_t, currentTime_t);

  // Calculate hours, minutes, and seconds from difference
  int hours = int(secondsUntilTarget) / 3600;
  int minutes = (int(secondsUntilTarget) % 3600) / 60;
  int seconds = int(secondsUntilTarget) % 60;

  // Display current date and time as before
  String AM_PM = (timeinfo.tm_hour >= 12) ? "PM" : "AM";
  
   // Convert hour from the military (24-hour) format to the standard (12-hour) format for display purposes
   int displayHour = timeinfo.tm_hour % 12;
   if(displayHour == 0) displayHour = 12; // Convert '0' hour to '12' for readability

   char dateStringBuff[50]; 
   strftime(dateStringBuff, sizeof(dateStringBuff), "%B, %d, %Y", &timeinfo);
   char timeStringBuff[50]; 
   strftime(timeStringBuff, sizeof(timeStringBuff), "%I:%M:%S", &timeinfo);
   
   u8g2.setFont(u8g2_font_ncenB08_tr);
   u8g2.drawStr(5,25,dateStringBuff);
   u8g2.setCursor(5,35);
   u8g2.print(timeStringBuff);
   u8g2.print(" ");
   u8g2.print(AM_PM.c_str());

   // Display countdown timer
   char countdownStringBuff[50];
   sprintf(countdownStringBuff, "Countdown: %02d:%02d:%02d", hours, minutes, seconds);
   u8g2.drawStr(5,45,countdownStringBuff); 
}

// scan wifi ssid
void handleScanWiFi() {
  WiFi.disconnect();
  delay(100);  // Smal
  int n = WiFi.scanNetworks();
  Serial.print("scaning");
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  for (int i = 0; i < n; ++i) {
    Serial.println(WiFi.SSID(i));
    array.add(WiFi.SSID(i));
  }
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

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
                margin-top: 20px;\
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
            select {\
                width: calc(100% - 20px);\
                height: 30px;\
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
            button.refresh {\
                width: 30%;\
                height: 40px;\
                font-size: 1em;\
                color: black;\
                border-radius: 8px;\
                border: none;\
                cursor: pointer;\
                margin-top: 3px;\
                margin-bottom: 3px;\
            }\
            button.submit:hover {\
                background-color: #45a049; /* Darker green on hover */\
            }\
            #loading {\
                display: none;\
                position: fixed;\
                width: 100%;\
                height: 100%;\
                top: 0;\
                left: 0;\
                right: 0;\
                bottom: 0;\
                background-color: rgba(255, 255, 255, 0.8);\
                z-index: 9999;\
                text-align: center;\
                padding-top: 200px;\
                font-size: 1.5em;\
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
                select {\
                    height: 35px;\
                    font-size: 0.8em;\
                }\
                button.refresh {\
                    height: 30px;\
                    font-size: 0.8em;\
                }\
                button.submit {\
                    width: calc(100% - 20px);\
                    height: 40px; /* Increased height for the submit button */\
                    font-size: 1em; /* Larger font size for better readability */\
                    background-color: #4CAF50;\
                    color: white;\
                    border: none;\
                    cursor: pointer;\
                    margin-top: 10px;\
                }\
            }\
        </style>\
        <script>\
            function refreshNetworks() {\
                document.getElementById('loading').style.display = 'block';\
                var xhr = new XMLHttpRequest();\
                xhr.open('GET', '/scanwifi', true);\
                xhr.onload = function () {\
                    if (xhr.status === 200) {\
                        var networks = JSON.parse(xhr.responseText);\
                        var selectElement = document.getElementById('ssid');\
                        selectElement.innerHTML = '';\
                        networks.forEach(function(network) {\
                            var option = document.createElement('option');\
                            option.text = network;\
                            selectElement.add(option);\
                        });\
                        document.getElementById('loading').style.display = 'none';\
                    } else {\
                        alert('Failed to scan networks.');\
                        document.getElementById('loading').style.display = 'none';\
                    }\
                };\
                xhr.send();\
            }\
            document.addEventListener('DOMContentLoaded', refreshNetworks); // Automatically refresh networks on page load\
        </script>\
    </head>\
    <body>\
        <div id=\"loading\">Scanning for networks...</div>\
        <div class=\"center-form\" id=\"content\">\
            <h1>Please Enter your Wifi Credentials</h1>\
            <p>Please select your Wifi SSID and enter the Password:</p>\
            <form action=\"/submit\" method=\"POST\">\
                Wifi SSID:<br>\
                <select name=\"Wifi SSID\" id=\"ssid\"></select><br>\
                <button type=\"button\" class=\"refresh\" onclick=\"refreshNetworks()\">Refresh</button><br>\
                Password:<br>\
                <input type=\"password\" name=\"Password\"><br><br>\
                <input type=\"submit\" class=\"submit\" value=\"Submit\">\
            </form>\
        </div>\
    </body>\
    </html>";

    server.send(200, "text/html", html);
}

void handleError() {
  String html = "<!DOCTYPE html>\
  <html>\
  <head>\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no\">\
    <style>\
      body {\
        font-family: Arial, sans-serif;\
        display: flex;\
        justify-content: center;\
        align-items: center;\
        height: 100vh;\
        margin: 0;\
      }\
      .center {\
        text-align: center;\
      }\
    </style>\
    <script>\
      setTimeout(function() {\
        window.location.href = '/';\
      }, 1000);\
    </script>\
  </head>\
  <body>\
    <div class=\"center\">\
      <h1>Error</h1>\
      <p>The password is incorrect. Redirecting to the main page...</p>\
    </div>\
  </body>\
  </html>";
  server.send(200, "text/html", html);
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
      if (millis() - startMillis >= 10000) {  //  If 10 seconds have passed
        Serial.println("Failed to connect to WiFi after 15 seconds.");
        server.sendHeader("Location", "/error", true);
        server.send(302, "text/plain", "");
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
    } else {
      Serial.println("Incorrect password or SSID. Re-scanning networks.");
      handleScanWiFi();  // Call handleScanWiFi to rescan networks
    }
  }
}

void webServerStart(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP Address: ");
  Serial.println(IP);
// Set up server routes

    // Set up server routes
    server.on("/", handleRoot);
    server.on("/submit", HTTP_POST, handleSubmit);
    server.on("/error", handleError); // Handle errors
    server.on("/scanwifi", HTTP_GET, handleScanWiFi); // Use handleScanWiFi for /scanwifi

  server.onNotFound([]() {
    server.send(404, "text/plain", "404: Not found");
  });

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

    u8g2.clearBuffer();

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
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      setTime();
  } 
}

void checkAndReconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED && !reconnectAttempted) {
    Serial.println("WiFi is disconnected. Trying to reconnect...");
    SavedCredentials(); // Try to reconnect using saved credentials
    reconnectAttempted = true;
    userConnectedToESP = false;
    messageDisplayed = false;

    u8g2.clearBuffer();

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

int channel0() {
    selectMuxChannel(0);
    int analogValue = analogRead(SIG);
    return analogValue;
    // if(analogValue > 350){
    //   return true;
    // } else{
    //   return false;
    // }
}

const unsigned char epd_bitmap_scan_to_connect [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x00, 0xd8, 0x3f, 0x3c, 0x18, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x00, 0x98, 0x1f, 0x38, 0x18, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0xfe, 0x99, 0x63, 0x00, 0x9e, 0x7f, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0xfe, 0x9b, 0x43, 0x00, 0x9e, 0x7f, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x06, 0x19, 0x0f, 0x36, 0xd9, 0x60, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x06, 0x1b, 0x8c, 0xf1, 0x99, 0x60, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x06, 0x1b, 0x8c, 0xf1, 0x99, 0x60, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x06, 0x79, 0x8c, 0x3f, 0x99, 0x60, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x06, 0x3b, 0x8c, 0x3f, 0x9b, 0x60, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0xfe, 0x19, 0x7c, 0xf8, 0x9f, 0x7f, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x28, 0x18, 0x6c, 0xf6, 0x1f, 0x51, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x00, 0x98, 0x6c, 0x36, 0x19, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x10, 0xc8, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x10, 0xc8, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x38, 0xf8, 0x6c, 0xf6, 0x78, 0x03, 0xfe, 0xff, 0xc3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x38, 0xf8, 0x6d, 0x72, 0x38, 0x03, 0xff, 0xff, 0xd1, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xff, 
	0xff, 0xc9, 0x86, 0x9f, 0x39, 0xc0, 0x8f, 0xff, 0xff, 0xf1, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xff, 
	0x7f, 0x36, 0x18, 0xfc, 0x3f, 0x98, 0x9c, 0xff, 0xff, 0xe3, 0x10, 0x0c, 0x0e, 0xc3, 0xff, 0xff, 
	0x7f, 0x36, 0x18, 0xfc, 0x3f, 0x98, 0x9c, 0xff, 0xff, 0x4f, 0x9c, 0x4c, 0x9e, 0x89, 0xff, 0xff, 
	0xff, 0xc1, 0x7c, 0xef, 0xf7, 0x18, 0x13, 0xfe, 0xff, 0x1f, 0xce, 0x6c, 0x9e, 0x99, 0xff, 0xff, 
	0xff, 0xc1, 0x7c, 0xef, 0xf7, 0x18, 0x13, 0xfe, 0xff, 0x81, 0x9c, 0x6c, 0x1e, 0xc9, 0xff, 0xff, 
	0x7f, 0xce, 0xfb, 0x13, 0xc6, 0x38, 0x90, 0xff, 0xff, 0xc1, 0x10, 0x6c, 0x3e, 0xc3, 0xff, 0xff, 
	0xff, 0xe7, 0x7c, 0x6c, 0xfe, 0xc1, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf7, 0x7e, 0x6c, 0xfe, 0xc1, 0x0c, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x30, 0x81, 0x03, 0x30, 0x01, 0x7f, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x30, 0x83, 0x03, 0x30, 0x01, 0x7f, 0xfe, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0xc0, 0xe7, 0x90, 0x07, 0xf9, 0x6f, 0xfe, 0x7f, 0xe8, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 
	0x7f, 0xc0, 0xe7, 0x90, 0x07, 0xf9, 0x4f, 0xff, 0x7f, 0xfe, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 
	0x7f, 0xf0, 0x81, 0x8c, 0x07, 0x9e, 0x8c, 0xff, 0x7f, 0x7e, 0x18, 0x0c, 0x86, 0x01, 0xfe, 0xff, 
	0x7f, 0xce, 0x87, 0x1c, 0xf8, 0x9e, 0x80, 0xff, 0x7f, 0x3e, 0x92, 0x49, 0x22, 0x38, 0xff, 0xff, 
	0x7f, 0xce, 0x87, 0x1c, 0xf8, 0xde, 0x80, 0xff, 0x7f, 0x3c, 0x93, 0xc9, 0x00, 0x3c, 0xff, 0xff, 
	0xff, 0x0f, 0x98, 0xfc, 0x39, 0xdf, 0x63, 0xfe, 0xff, 0x60, 0x92, 0xc9, 0xa6, 0x39, 0xff, 0xff, 
	0xff, 0x1f, 0xd8, 0xfc, 0x39, 0x9f, 0x43, 0xfe, 0xff, 0x61, 0x98, 0xc9, 0x84, 0x21, 0xfe, 0xff, 
	0xff, 0xff, 0xe6, 0x6c, 0xc6, 0x80, 0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x1c, 0xdb, 0x10, 0xfe, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x08, 0x9b, 0x10, 0xfe, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x9f, 0xec, 0xfd, 0xf9, 0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xdf, 0xcc, 0xf9, 0xf9, 0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x00, 0xf8, 0x03, 0x08, 0x98, 0x70, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x14, 0x78, 0x03, 0x0c, 0xd8, 0x70, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0xfe, 0x19, 0x93, 0x07, 0xf9, 0x90, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x06, 0x9b, 0x8c, 0x07, 0x00, 0x60, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x06, 0x99, 0x8c, 0x07, 0x00, 0x60, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x06, 0x7b, 0x1f, 0x3e, 0x27, 0x83, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x06, 0x79, 0x3f, 0x3e, 0x67, 0x83, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x06, 0xfb, 0xfc, 0x39, 0xc6, 0x03, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0xfe, 0xdb, 0x6c, 0x36, 0x98, 0xc7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0xfe, 0x99, 0x6c, 0x36, 0x98, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x00, 0x98, 0x13, 0x3e, 0x1f, 0x90, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x7f, 0x00, 0xd8, 0x33, 0x3e, 0x1f, 0xb8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// 'SCAN TO GO WEBSITE', 128x64px
const unsigned char epd_bitmap_SCAN_TO_GO_WEBSITE [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0xe0, 0xf9, 0x1f, 0x02, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0xe0, 0xf9, 0x1f, 0x02, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xfc, 0x67, 0x18, 0x18, 0xf2, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xfc, 0x67, 0x18, 0x18, 0xf3, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0xe4, 0x81, 0xf9, 0x33, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0xe6, 0x81, 0xf9, 0x33, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0x64, 0x98, 0x81, 0x33, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0xe4, 0x98, 0xc1, 0x33, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0xe6, 0x67, 0x7e, 0x32, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xac, 0xe7, 0xe7, 0xfc, 0xf3, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xfc, 0x67, 0xe6, 0xf9, 0xf3, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x60, 0x66, 0x76, 0x03, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x60, 0x66, 0x66, 0x02, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x7f, 0xe6, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0x7f, 0xe6, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xf9, 0x7f, 0xfe, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x0e, 0x60, 0xde, 0xdd, 0xcc, 0xff, 0xff, 0x8f, 0xff, 0x7f, 0xfe, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x06, 0x60, 0x9e, 0xcd, 0xcc, 0xff, 0xff, 0xcf, 0x31, 0x3c, 0x84, 0xff, 0xff, 0xff, 
	0xff, 0x3f, 0x9f, 0xf9, 0x87, 0xf1, 0x3f, 0xff, 0xff, 0xcf, 0x90, 0x79, 0x22, 0xff, 0xff, 0xff, 
	0xff, 0x3f, 0x9f, 0xf9, 0x87, 0xf1, 0x3f, 0xff, 0xff, 0x9f, 0x93, 0x79, 0x32, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x80, 0x19, 0x7e, 0x02, 0xf3, 0xff, 0xff, 0x1f, 0x18, 0x78, 0x26, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x80, 0x19, 0x7e, 0x02, 0xf3, 0xff, 0xff, 0x3f, 0x3c, 0x7c, 0x8c, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x9f, 0x81, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x9f, 0x81, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf0, 0xe3, 0x99, 0x9f, 0x0f, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf0, 0xe1, 0x99, 0x9f, 0x0f, 0x0c, 0xff, 0x7f, 0xe6, 0xfc, 0xfd, 0xdf, 0xff, 0xff, 0xff, 
	0xff, 0xcc, 0x9b, 0x61, 0x78, 0x00, 0xff, 0xff, 0x7f, 0xc6, 0xfc, 0xfd, 0xdf, 0xfc, 0xff, 0xff, 
	0xff, 0xcc, 0x91, 0x61, 0x38, 0x00, 0xff, 0xff, 0x7f, 0x46, 0xfe, 0xfd, 0xff, 0xfc, 0xff, 0xff, 
	0xff, 0x0c, 0x63, 0xe6, 0x01, 0x32, 0x33, 0xff, 0xff, 0x44, 0x86, 0xc1, 0x58, 0x08, 0xff, 0xff, 
	0xff, 0x0c, 0x61, 0xe6, 0x01, 0x32, 0x33, 0xff, 0xff, 0x50, 0x22, 0xc9, 0xdc, 0x44, 0xfe, 0xff, 
	0xff, 0xcc, 0xfe, 0x79, 0xf8, 0xfd, 0x0f, 0xff, 0xff, 0x10, 0x03, 0x9d, 0xd8, 0x04, 0xfe, 0xff, 
	0xff, 0xcc, 0xfe, 0x79, 0xf8, 0xf8, 0x0f, 0xff, 0xff, 0x18, 0xa7, 0xc9, 0xd1, 0x44, 0xff, 0xff, 
	0xff, 0x0c, 0x87, 0x61, 0x06, 0x00, 0x3c, 0xff, 0xff, 0x39, 0x87, 0xc1, 0xd8, 0x08, 0xff, 0xff, 
	0xff, 0x8e, 0x8f, 0x61, 0x07, 0x00, 0x3c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0x7f, 0xfe, 0x7f, 0x3e, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x60, 0x00, 0x1f, 0x36, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x60, 0x00, 0x1e, 0x32, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xfc, 0xe7, 0x99, 0x07, 0x3e, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xfc, 0xe7, 0x99, 0x07, 0x3e, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0x66, 0x86, 0x1f, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0x66, 0x86, 0x1f, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0x66, 0x66, 0x20, 0x02, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0x64, 0x66, 0x60, 0x02, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0x66, 0xe6, 0x81, 0x0f, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x0c, 0x66, 0xe6, 0x81, 0x0f, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xfc, 0x67, 0x7e, 0x18, 0x33, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xfc, 0x67, 0x7e, 0x18, 0x36, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x60, 0x66, 0x66, 0xfe, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0x00, 0x60, 0x66, 0x66, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// 'wifi', 20x16px
const unsigned char epd_bitmap_wifi [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x1f, 0x00, 0xe0, 0x79, 0x00, 0x30, 
	0xc0, 0x00, 0x80, 0x1f, 0x00, 0xc0, 0x30, 0x00, 0x00, 0x06, 0x00, 0x80, 0x1f, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void timeToDrink(){
    u8g2.clearBuffer();
    u8g2.drawXBMP(0, 0, 20, 16, epd_bitmap_wifi);
    u8g2.drawStr(5,25,"time to drink:");
    u8g2.drawStr(5,35,"time to drink");
    // u8g2.print(" ");
    // u8g2.print("biogesic"); //name of the med
    u8g2.sendBuffer(); 
}

String  firstName(){
if (Firebase.RTDB.getString(&fbdo, "usr/12345/first_name")) {
  if (fbdo.dataType() == "string") {
    String firstName = fbdo.stringData();
    firstName.toUpperCase();
    Serial.print("First Name: ");
    Serial.println(firstName);
    return firstName;
}
  } else {
    Serial.print("FAILED");
    Serial.print("REASON: ");
    Serial.println(fbdo.errorReason());
}
return "";
}

void setup() {
FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  Serial.begin(115200);
  u8g2.begin(); // start the u8g2 library
  // CD74HC4067
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SIG, INPUT);
      // Start WiFi scan
  Serial.println("Starting WiFi scan...");
  WiFi.mode(WIFI_STA);

  SavedCredentials();
  animation();
  u8g2.clearBuffer();
}


void loop() {
  server.handleClient();
  int currentStationCount = WiFi.softAPgetStationNum();
  bool isConnected = WiFi.status() == WL_CONNECTED; 
  server.handleClient();

// save values for CD74HC4067 
  int value0 = channel0();
  // Serial.println(value0);
  int resetbutton = digitalRead(reset);
// check if wifi is interupted
  if (wasConnected && !isConnected) {
    Serial.println("Internet connection was interrupted.");
    checkAndReconnectWiFi();
  }
// button force wifi reset 
  if (resetbutton == HIGH && pressed == false && WiFi.status() == WL_CONNECTED){
    Serial.print("network dissconnected");
    Serial.print(" button value: ");
    Serial.println(resetbutton);
    preferences.begin("credentials", false);
    preferences.clear();
    preferences.end();
    WiFi.disconnect();
    pressed = true;
  }



  





  if (WiFi.status() == WL_CONNECTED && Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0 )){
    struct tm timeinfo;
    u8g2.clearBuffer();
    u8g2.drawXBMP(0, 0, 20, 16, epd_bitmap_wifi);
    setTime();   
    u8g2.setCursor(5,55);
    //make a gm gn and afternoon 
    u8g2.print("Goodevening ");
    u8g2.print(firstName().c_str()); 
    u8g2.sendBuffer(); 

    sendDataPrevMillis = millis();
    int value0 = channel0();
    pressed = false;
    if (Firebase.RTDB.setInt(&fbdo, "usr/12345/sensor_value/slot_1", value0)){
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
