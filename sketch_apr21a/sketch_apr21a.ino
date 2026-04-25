// Select your modem type BEFORE including the TinyGSM library
#define TINY_GSM_MODEM_SIM800 

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- GPRS Credentials ---
const char apn[]      = "http.globe.com.ph"; // TM / Globe APN
const char gprsUser[] = "";
const char gprsPass[] = "";

// --- Cloud Server Details (Make.com Middleman) ---
const char server[]   = "sgp1.blynk.cloud"; 
const int  port       = 80; // We keep standard Port 80!
const char auth[]     = "6fub_AeSZfywBab9j-d7KRXWKFPMwIxz";

// --- OLED Configuration ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Hardware Pins ---
// GPS (Serial 1)
#define GPS_RX 16
#define GPS_TX 17
HardwareSerial gpsSerial(1);
TinyGPSPlus gps;

// SIM800L (Serial 2)
#define SIM_RX 26
#define SIM_TX 27
HardwareSerial simSerial(2);

// --- Network Clients ---
TinyGsm modem(simSerial);
TinyGsmClient client(modem);
HttpClient http(client, server, port);

// --- State Variables ---
unsigned long lastUpdate = 0;
const long updateInterval = 15000; // Send data every 15 seconds
String cloudStatus = "Standby";

// --- Function Prototypes ---
void updateDisplay();
void sendDataToCloud(float lat, float lng, int sats);

void setup() {
  Serial.begin(115200);
  delay(10);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); 
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Car Rental Tracker");
  display.println("Starting up...");
  display.display();

  // Initialize Serial connections
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  simSerial.begin(9600, SERIAL_8N1, SIM_RX, SIM_TX);

  display.setCursor(0, 20);
  display.println("Init SIM800L...");
  display.display();
  
  modem.restart();
  
  display.println("Finding Cell Tower...");
  display.display();
  if (!modem.waitForNetwork()) {
    display.println("Network FAIL.");
    display.display();
    delay(10000);
    ESP.restart();
  }

  display.print("GPRS: ");
  display.println(apn);
  display.display();
  
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    display.println("GPRS FAIL.");
    display.display();
    delay(10000);
    ESP.restart();
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("System Ready!");
  display.display();
  delay(2000);
}

void loop() {
  // 1. Constantly feed data to the GPS object
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // 2. Refresh the OLED screen rapidly
  updateDisplay();

  // 3. Check if it's time to send an update AND we have a valid location
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    if (gps.location.isValid()) {
      float lat = gps.location.lat();
      float lng = gps.location.lng();
      int sats = gps.satellites.value();

      sendDataToCloud(lat, lng, sats);
    } else {
      cloudStatus = "Waiting for GPS...";
    }
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Car Rental Tracker");

  // GPRS Status
  display.setCursor(0, 12);
  display.print("Net: ");
  if (modem.isGprsConnected()) {
    display.print("Connected (TM)");
  } else {
    display.print("Disconnected!");
  }

  // GPS Status
  display.setCursor(0, 24);
  display.print("Sats: ");
  display.print(gps.satellites.value());
  
  display.setCursor(0, 36);
  if (gps.location.isValid()) {
    display.print("Lat: ");
    display.print(gps.location.lat(), 5);
    display.setCursor(0, 46);
    display.print("Lng: ");
    display.print(gps.location.lng(), 5);
  } else {
    display.print("Locating satellites..");
  }

  // Cloud Post Status
  display.setCursor(0, 56);
  display.print("Web: ");
  display.print(cloudStatus);
  
  display.display();
}

void sendDataToCloud(float lat, float lng, int sats) {
  cloudStatus = "Syncing...";
  updateDisplay(); 
  
  // Create the URL
  String url = "/external/api/batch/update?token=" + String(auth);
  url += "&v1=" + String(lat, 6);
  url += "&v2=" + String(lng, 6);
  url += "&v3=" + String(sats);
  
  // FIX 1: Added the '=' sign
  // FIX 2: Added '()' to millis
  url += "&v4=" + String(millis()); 
  
  Serial.print("Batch Sending: ");
  Serial.println(url);

  http.beginRequest();
  http.get(url);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  // We don't need to store the body, but calling it clears the buffer
  http.responseBody(); 

  if (statusCode == 200) {
    cloudStatus = "Success (200)";
  } else {
    cloudStatus = "HTTP Err: " + String(statusCode);
  }
}