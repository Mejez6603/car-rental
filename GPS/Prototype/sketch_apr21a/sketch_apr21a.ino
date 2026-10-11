#define TINY_GSM_MODEM_SIM800

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

#include "index.h"

// --- Network Credentials ---
const char gprsUser[] = "";
const char gprsPass[] = "";

// Compiled-in fallbacks, used only until the WiFi portal saves its own values
const char* DEFAULT_APN    = "http.globe.com.ph";
const char* DEFAULT_SERVER = "sgp1.blynk.cloud";
const uint16_t DEFAULT_PORT = 80;
const char* DEFAULT_AUTH   = "6fub_AeSZfywBab9j-d7KRXWKFPMwIxz";

// Live cloud config, editable from the portal and persisted in NVS
Preferences prefs;
String cfgApn;
String cfgServer;
uint16_t cfgPort;
String cfgAuth;

// --- Local Wi-Fi AP Settings ---
const char* CAR_ID = "Car-001";
char apSSID[32];
const char* apPassword = "";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer webServer(80);

// --- OLED Configuration ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Hardware Pins ---
#define GPS_RX 16
#define GPS_TX 17
HardwareSerial gpsSerial(1);
TinyGPSPlus gps;

#define SIM_RX 26
#define SIM_TX 27
HardwareSerial simSerial(2);

TinyGsm modem(simSerial);
TinyGsmClient client(modem);

// --- State Variables ---
unsigned long lastUpdate = 0;
const long updateInterval = 15000;
String cloudStatus = "Standby";

unsigned long lastReconnectAttempt = 0;
const long reconnectInterval = 30000; // don't hammer the modem while it's down

unsigned long lastDisplay = 0;
const long displayInterval = 500;
unsigned long privacyMsgUntil = 0;

const unsigned long GPS_STALE_MS = 5000; // treat a fix as gone if it hasn't updated in this long

bool screenOn = true;

double lat = 0.0, lng = 0.0;
int vsat = 0;
bool isLocationValid = false;

void updateDisplay();
void sendDataToCloud(double l_lat, double l_lng, int l_sats);
void ensureCellularConnection();
void loadCloudConfig();
void handleRoot();
void handleToggle();
void handleData();
void handleSettings();

void setup() {
  Serial.begin(115200);
  delay(1000);

  loadCloudConfig();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for (;;); }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Car Rental Tracker");
  display.println("Starting up...");
  display.display();

  // --- 1. START NEO-6M GPS ---
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  // --- 2. START LOCAL AP & CAPTIVE PORTAL ---
  snprintf(apSSID, sizeof(apSSID), "%s-Local", CAR_ID);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID, apPassword);

  dnsServer.start(DNS_PORT, "*", apIP);

  webServer.on("/", handleRoot);
  webServer.on("/toggle", handleToggle);
  webServer.on("/data", handleData);
  webServer.on("/settings", HTTP_POST, handleSettings);
  webServer.onNotFound(handleRoot);
  webServer.begin();

  display.println("Wi-Fi AP Started");
  display.display();

  // --- 3. AIR780E CELLULAR ---
  simSerial.begin(115200, SERIAL_8N1, SIM_RX, SIM_TX);

  display.println("Init Air780E...");
  display.display();
  modem.restart();

  display.println("Finding Cell Tower...");
  display.display();

  if (modem.waitForNetwork(60000)) {
    delay(3000);

    display.print("GPRS: ");
    display.println(cfgApn);
    display.display();

    if (modem.gprsConnect(cfgApn.c_str(), gprsUser, gprsPass)) {
      display.println("GPRS Connected!");
      cloudStatus = "Connected";
    } else {
      display.println("GPRS Failed!");
      cloudStatus = "GPRS Fail";
    }
  } else {
    display.println("No Signal!");
    cloudStatus = "No Signal";
  }
  display.display();
  delay(1000);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System Ready!");
  display.display();
  delay(1000);
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // age() catches a fix that stopped updating (lost antenna, dead module)
  // but that isValid() alone would keep reporting as good forever.
  isLocationValid = gps.location.isValid() && (gps.location.age() < GPS_STALE_MS);
  if (isLocationValid) {
    lat = gps.location.lat();
    lng = gps.location.lng();
  }
  vsat = gps.satellites.value();

  if (screenOn && millis() >= privacyMsgUntil && millis() - lastDisplay >= (unsigned long)displayInterval) {
    lastDisplay = millis();
    updateDisplay();
  }

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    ensureCellularConnection();

    if (isLocationValid && lat != 0.0) {
      sendDataToCloud(lat, lng, vsat);
    }
  }
}

// --- WEB SERVER ENDPOINTS ---
void handleRoot() {
  String html = String(INDEX_HTML);

  html.replace("{CAR_ID}", CAR_ID);
  html.replace("{GPS_STATUS}", isLocationValid ? "Locked" : "Searching...");
  html.replace("{SATS}", String(vsat));
  html.replace("{NET_STATUS}", cloudStatus);

  if (isLocationValid && lat != 0.0) {
    html.replace("{SHOW_COORDS}", "block");
    html.replace("{LAT}", String(lat, 5));
    html.replace("{LNG}", String(lng, 5));
  } else {
    html.replace("{SHOW_COORDS}", "none");
    html.replace("{LAT}", "0.0");
    html.replace("{LNG}", "0.0");
  }

  html.replace("{PRIVACY_STATE}", screenOn ? "OFF (Screen Active)" : "ON (Screen Hidden)");
  html.replace("{PRIVACY_CHECKED}", screenOn ? "" : "checked");

  html.replace("{CFG_APN}", cfgApn);
  html.replace("{CFG_SERVER}", cfgServer);
  html.replace("{CFG_PORT}", String(cfgPort));

  webServer.send(200, "text/html", html);
}

void handleToggle() {
  screenOn = !screenOn;

  display.clearDisplay();
  if (screenOn) {
    display.setCursor(0, 20);
    display.println("Privacy Mode: OFF");
    privacyMsgUntil = millis() + 2000;
  }
  display.display();

  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}

void handleData() {
  String json = "{";
  json += "\"gps\":\""; json += (isLocationValid ? "Locked" : "Searching..."); json += "\",";
  json += "\"sats\":"; json += vsat; json += ",";
  json += "\"valid\":"; json += ((isLocationValid && lat != 0.0) ? "true" : "false"); json += ",";
  json += "\"lat\":"; json += String(lat, 6); json += ",";
  json += "\"lng\":"; json += String(lng, 6); json += ",";
  json += "\"net\":\""; json += cloudStatus; json += "\",";
  json += "\"privacyOn\":"; json += (screenOn ? "false" : "true");
  json += "}";
  webServer.send(200, "application/json", json);
}

// Fields left blank keep their current value.
void handleSettings() {
  prefs.begin("cloudcfg", false);

  if (webServer.hasArg("apn") && webServer.arg("apn").length() > 0) {
    cfgApn = webServer.arg("apn");
    prefs.putString("apn", cfgApn);
  }

  if (webServer.hasArg("server") && webServer.arg("server").length() > 0) {
    cfgServer = webServer.arg("server");
    prefs.putString("server", cfgServer);
  }

  if (webServer.hasArg("port") && webServer.arg("port").length() > 0) {
    int p = webServer.arg("port").toInt();
    if (p > 0 && p <= 65535) {
      cfgPort = (uint16_t)p;
      prefs.putUShort("port", cfgPort);
    }
  }

  if (webServer.hasArg("auth") && webServer.arg("auth").length() > 0) {
    cfgAuth = webServer.arg("auth");
    prefs.putString("auth", cfgAuth);
  }

  prefs.end();

  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}

// --- DISPLAY & CLOUD LOGIC ---
void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(CAR_ID); display.print(" Tracker");
  display.setCursor(0, 12);
  display.print("Net: "); display.print(cloudStatus);
  display.setCursor(0, 24);
  display.print("Sats: "); display.print(vsat);
  display.setCursor(0, 36);

  if (isLocationValid && lat != 0.0) {
    display.print("Lat: "); display.print(lat, 5);
    display.setCursor(0, 46);
    display.print("Lng: "); display.print(lng, 5);
  } else {
    display.print("Locating...");
  }

  display.setCursor(0, 56);
  display.print("AP: "); display.print(apSSID);
  display.display();
}

void sendDataToCloud(double l_lat, double l_lng, int l_sats) {
  if (cloudStatus == "No Signal" || cloudStatus == "GPRS Fail") return;

  cloudStatus = "Syncing...";
  String url = "/external/api/batch/update?token=" + cfgAuth + "&v1=" + String(l_lat, 6) + "&v2=" + String(l_lng, 6) + "&v3=" + String(l_sats) + "&v4=" + String(millis());
  HttpClient http(client, cfgServer.c_str(), cfgPort);
  http.setHttpResponseTimeout(10000); // default is 30s, which would freeze the portal
  http.beginRequest();
  http.get(url);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  http.stop();
  // Negative codes = request/connection never got a real response (timeout,
  // dropped link, no data balance). 4xx/5xx = it reached the server, which
  // rejected it (bad auth token, wrong server/port, Blynk-side issue).
  cloudStatus = (statusCode == 200) ? "Connected" : ("HTTP Err (" + String(statusCode) + ")");
}

void loadCloudConfig() {
  prefs.begin("cloudcfg", true);
  cfgApn = prefs.getString("apn", DEFAULT_APN);
  cfgServer = prefs.getString("server", DEFAULT_SERVER);
  cfgPort = prefs.getUShort("port", DEFAULT_PORT);
  cfgAuth = prefs.getString("auth", DEFAULT_AUTH);
  prefs.end();
}

// Retries a dropped/failed cellular link every reconnectInterval instead of
// giving up forever after one failed attempt at boot.
void ensureCellularConnection() {
  if (modem.isGprsConnected()) return;

  if (millis() - lastReconnectAttempt < (unsigned long)reconnectInterval) return;
  lastReconnectAttempt = millis();

  cloudStatus = "Reconnecting...";

  if (!modem.isNetworkConnected() && !modem.waitForNetwork(15000)) {
    cloudStatus = "No Signal";
    return;
  }

  cloudStatus = modem.gprsConnect(cfgApn.c_str(), gprsUser, gprsPass) ? "Connected" : "GPRS Fail";
}
