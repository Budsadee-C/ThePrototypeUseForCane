#include "M5StickC.h"
#include "WiFi.h"
#include "TinyGPS++.h"
#include "SoftwareSerial.h"
#include "WiFiUdp.h"
#include "NTPClient.h"
#include "IOXhop_FirebaseESP32.h"

#define WIFI_SSID "xxxxx"
#define WIFI_PASSWORD "xxxxxx"

#define FIREBASE_HOST "xxxxxxx"
#define FIREBASE_AUTH "xxxxx"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

TinyGPSPlus gps;
HardwareSerial ss(2);

const int mid_button = 37; // 37 middle button 
const int rig_button = 39; // 39 right buttons
int countMid_botton = 0; 

const int servo_pin = 26;// speaker hat 26 36 0
int freq = 1000;//50
int ledChannel = 0;
int resolution = 20;

double init_long = 0.0;
double init_lat = 0.0;
String gpsRead_long ="";
String gpsRead_lat ="";
double distance =0.0;
String DateRead ="";
String TimeRead = "";

int stepCount = 0;


void setup() 
{
  // put your setup code here, to run once:
  M5.begin();
  ss.begin(9600, SERIAL_8N1, 33, 32);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  M5.Lcd.setRotation(3);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setCursor(40, 0); 
  M5.Lcd.println("MyProjectGPS");    
  pinMode(mid_button, INPUT);
  pinMode(rig_button, INPUT);

  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(servo_pin, ledChannel);
  ledcWrite(ledChannel, 0);//256°
  connectWifi();
}

void loop() 
{
  if(WiFi.status() != WL_CONNECTED)// In cast, dont connect internet
  {
    connectWifi();
  }
  int midB_value = digitalRead(mid_button); // value of middle button
  int rigB_value = digitalRead(rig_button); // value of middle button
  Sos(midB_value); // Check have press button
  GetInitGPS();
  ReadGps();

  
}
void connectWifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("\nWait for WiFi ....\n");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Start WiFi Connecting..");
  Serial.print("IP address: ");Serial.println(WiFi.localIP());
}
void Sos(int button)
{
  Serial.print("Button Status: ");Serial.println(button);
  if(button==0) // when mid_button have pressed then have sound (SOS)
  {
    countMid_botton++;
  }
  if(countMid_botton%2 == 1)
  {
    Serial.println("Play");
    PlaySpeaker();
  }
  else
  {
    Serial.println("Close");
    CloseSpeaker();
  }
  Serial.print("count Button: ");Serial.println(countMid_botton);
}
void PlaySpeaker()
{
  ledcWriteTone(ledChannel, 9000);//1250
  delay(1000);
  ledcWriteTone(ledChannel, 0);
}
void CloseSpeaker()
{
   ledcWriteTone(ledChannel, 0);
}
void GetInitGPS()
{
  init_lat = Firebase.getFloat("InitGPS/lat");
  init_long = Firebase.getFloat("InitGPS/long");
}
void GetInitPedo()
{
  stepCount = Firebase.getInt("InitPedo/StepCount");
}
void InsertGPS()
{
  if(distance>50) // if distance (from initial) more than 50 metre that will upload new lat & long
  {
    StaticJsonBuffer<400> jsonBuffer;
    JsonObject& GPS = jsonBuffer.createObject();
    GPS["lat"] = gpsRead_lat.toFloat();
    GPS["long"] = gpsRead_long.toFloat();
    GPS["date"] = DateRead;
    GPS["time"] = TimeRead;
    Firebase.set("ReadGps", GPS);
    Firebase.setFloat("Distance/distance", distance);
    Firebase.setString("Distance/status", "Alert");
  }
  else
  {
    Firebase.setFloat("Distance/distance", distance);
    Firebase.setString("Distance/status", "Normal");
  }
  if (Firebase.failed()) 
  {
      Serial.print("Firebase can't insert data");
      Serial.println(Firebase.error());
      return;
  }
  else{
      Serial.println(", Firebase pushed: /GPS");
  }
}
void ReadGps()
{
  while (ss.available() > 0)
    { gps.encode(ss.read()); }
  gpsRead_lat = String(gps.location.lat(),6);
  gpsRead_long = String(gps.location.lng(),6);
  DateRead = String(gps.date.day())+"/"+String(gps.date.month())+"/"+String(gps.date.year());
  TimeRead = String(gps.time.hour()+7)+":"+String(gps.time.minute())+":"+String(gps.time.second());
  InsertGPS();
  Serial.println(gpsRead_lat);
  Serial.println(gpsRead_long);
  Serial.println(DateRead);
  Serial.println(TimeRead);

  distance = gps.distanceBetween(gps.location.lat(),gps.location.lng(),init_lat,init_long);// 1000.0; // km
  Serial.println(distance);
}
