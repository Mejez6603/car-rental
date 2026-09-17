#define TINY_GSM_MODEM_SIM800

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- GPRS Credentials ---
const char apn[]      = "http.globe.com.ph";
const char gprsUser[] = "";
const char gprsPass[] = "";

const char server[]   = "sgp1.blynk.cloud";
const int  port       = 80;
const char auth[]     = "6fub_AeSZfywBab9j-d7KRXWKFPMwIxz";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Hardware Pins ---
#define BUTTON_PIN 4 
#define GPS_RX 16
#define GPS_TX 17
HardwareSerial gpsSerial(1);
TinyGPSPlus gps;

#define SIM_RX 26
#define SIM_TX 27
HardwareSerial simSerial(2);

TinyGsm modem(simSerial);
TinyGsmClient client(modem);
HttpClient http(client, server, port);

// --- State Variables ---
unsigned long lastUpdate = 0;
const long updateInterval = 15000;
String cloudStatus = "Standby";

bool screenOn = true;
bool lastButtonState = HIGH; 

void updateDisplay();
void sendDataToCloud(float lat, float lng, int sats);
void checkButton();

void setup() {
  Serial.begin(115200);
  
  // 1. Stabilization Delay
  delay(1000); 

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  display.clearDisplay();
  display.display();
  
  // Startup UI
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Car Rental Tracker");
  display.println("Starting up...");
  display.display();

  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  simSerial.begin(115200, SERIAL_8N1, SIM_RX, SIM_TX);

  // --- YOUR REQUESTED STARTUP FLOW ---
  display.println("Init Air780E...");
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
  while (gpsSerial.available() > 0) gps.encode(gpsSerial.read());

  checkButton(); 

  if (screenOn) {
    updateDisplay();
  } else {
    display.clearDisplay();
    display.display();
  }

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    if (gps.location.isValid()) {
      sendDataToCloud(gps.location.lat(), gps.location.lng(), gps.satellites.value());
    } else {
      cloudStatus = "Waiting for GPS...";
    }
  }
}

void checkButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      screenOn = !screenOn; 
      
      display.clearDisplay();
      display.setCursor(0, 20);
      display.println(screenOn ? "Privacy Mode: OFF" : "Privacy Mode: ON");
      display.display();
      delay(2000);
    }
  }
  lastButtonState = currentButtonState;
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Car Rental Tracker");
  display.setCursor(0, 12);
  display.print("Net: Connected");
  display.setCursor(0, 24);
  display.print("Sats: "); display.print(gps.satellites.value());
  display.setCursor(0, 36);
  if (gps.location.isValid()) {
    display.print("Lat: "); display.print(gps.location.lat(), 5);
    display.setCursor(0, 46);
    display.print("Lng: "); display.print(gps.location.lng(), 5);
  } else {
    display.print("Locating...");
  }
  display.setCursor(0, 56);
  display.print("Web: "); display.print(cloudStatus);
  display.display();
}

void sendDataToCloud(float lat, float lng, int sats) {
  cloudStatus = "Syncing...";
  String url = "/external/api/batch/update?token=" + String(auth) + "&v1=" + String(lat, 6) + "&v2=" + String(lng, 6) + "&v3=" + String(sats) + "&v4=" + String(millis());
  http.beginRequest();
  http.get(url);
  http.endRequest();
  cloudStatus = (http.responseStatusCode() == 200) ? "Success (200)" : "HTTP Err";
}