#include <WiFi.h>
#include <Wire.h>
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2lib.h>
#include "ani.h"

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#define WIFI_SSID "ASUS_2G"
#define WIFI_PASSWORD "09231610!"

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


void setup() {
  Serial.begin(115200);
  u8g2.begin(); // start the u8g2 library


// CD74HC4067
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SIG, INPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

//  Sign up 
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


  // // initialize with the I2C addr 0x3D (for the 128x64)
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("failed to start SSD1306 OLED"));
    while (1);
  }

  // //stream
  // if(!Firebase. RTDB.beginStream(&fbdo_s1, "sensor/slot-1")) {
  // Serial.printf("stream 1 begin error, %s\n\n", fbdo_s1.errorReason ().c_str());}
  // if(!Firebase. RTDB.beginStream(&fbdo_s2, "sensor/slot-2")) {
  // Serial.printf("stream 1 begin error, %s\n\n", fbdo_s2.errorReason ().c_str());}

// for lcd
  oled.clearDisplay(); // clear display
  oled.setTextSize(1);         // set text size
  oled.setTextColor(WHITE);    // set text color
// animation
while (true) { 
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

void selectMuxChannel(int channel) {
  digitalWrite(S0, bitRead(channel, 0));
  digitalWrite(S1, bitRead(channel, 1));
  digitalWrite(S2, bitRead(channel, 2));
  digitalWrite(S3, bitRead(channel, 3));
}

bool channel0() {
    selectMuxChannel(0);
    int analogValue = analogRead(SIG);
    if(analogValue > 300){
      return true;
    } else{
      return false;
    }
}


void loop() {

  oled.setCursor(0, 10);       
  oled.println("good day! "); 
  oled.setCursor(0, 20);       
  oled.println("welcome / user"); 
  oled.setCursor(0, 30);       
  oled.println("next intake in: 00/00"); 
  oled.setCursor(0, 40);       
  oled.println("date: 00/00/00");
  oled.display();   


  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 500 || sendDataPrevMillis == 0)){
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

}
