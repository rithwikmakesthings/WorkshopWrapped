#include <Wire.h>
#include <Adafruit_PN532.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h> // <-- This is the one you already have
#include <NetworkClient.h> // <-- ADD THIS LINE FIRST

// --- WI-FI CONFIGURATION ---
const char* ssid     = "";     // Put your Wi-Fi name here
const char* password = ""; // Put your Wi-Fi password here

// --- NEW: GOOGLE APPS SCRIPT WEB APP GATEWAY URL ---
// Paste your exact deployment script URL here! Keep the "https://" format prefix.
const char* googleScriptUrl = "INSERT";

// --- TIMEZONE CONFIGURATION ---
const char* time_zone = "AEST-10AEDT,M10.1.0,M4.1.0/3"; 
const char* ntpServer = "pool.ntp.org";



// --- PIN DEFINITIONS ---
#define I2C_SDA       8
#define I2C_SCL       9
#define PN532_IRQ     11
#define PN532_RESET   12

#define LED_PIN       10   
#define NUM_LEDS      13  
#define SWITCH_PIN    4   

// --- DEBOUNCE CONFIGURATION ---
const unsigned long debounceDelay = 75; 

// --- PRE-DEFINED TAG UIDS ---
const uint8_t TARGET_TAG_A[] = { 0x04, 0x51, 0xB7, 0xFA, 0x12, 0x72, 0x80 }; 
const uint8_t TARGET_TAG_B[] = { 0x04, 0xF3, 0xB0, 0xFA, 0x12, 0x72, 0x80 }; 

// --- OBJECT INITIALIZATION ---
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- TRAY TRACKING STATES ---
bool tagAStowed = false; 
bool tagBStowed = false; 

time_t tagA_leftScannerTime = 0; 
time_t tagB_leftScannerTime = 0; 

// --- DEBOUNCER STATE VARIABLES ---
bool stableSwitchState = false;       
bool lastRawReading = false;          
unsigned long lastDebounceTime = 0;   

char currentStowedTrayChar = ' ';     

// Helper function to set the whole strip to one solid color
void setStripColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

// Flash specific colors with 100ms on / 100ms off intervals
void flashLEDs(uint8_t r, uint8_t g, uint8_t b, int count) {
  for (int i = 0; i < count; i++) {
    setStripColor(r, g, b);   
    delay(100);               
    setStripColor(0, 0, 0);   
    delay(100);               
  }
}

// Non-Blocking White Breathing Animation
void runBreathingAnimation() {
  float speedFactor = 0.002; 
  float val = (sin(millis() * speedFactor) + 1.0) / 2.0; 
  uint8_t breathingBrightness = 15 + (val * 75); 

  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(breathingBrightness, breathingBrightness, breathingBrightness));
  }
  strip.show();
}

// Helper function to compare two UIDs byte-by-byte
bool compareUIDs(const uint8_t *scanned, uint8_t scannedLength, const uint8_t *target, uint8_t targetLength) {
  if (scannedLength != targetLength) return false;
  for (uint8_t i = 0; i < scannedLength; i++) {
    if (scanned[i] != target[i]) return false;
  }
  return true;
}

// Helper function to get wall clock time as a formatted String string
String getFormattedTimestamp() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "2026-01-01 00:00:00"; // Fallback default anchor
  }
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

// Helper function to print actual wall-clock time
void printTimestamp() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.print("[Time Error] ");
    return;
  }
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "[%Y-%m-%d %H:%M:%S] ", &timeinfo);
  Serial.print(timeStringBuff);
}

// Helper function to calculate and print duration out in use
void printDurationSinceRemoved(time_t removalTime) {
  if (removalTime == 0) {
    Serial.println("Duration out: Unknown (System booted while tray was out)");
    return;
  }
  
  time_t currentTime;
  time(&currentTime); 
  
  long diffSeconds = difftime(currentTime, removalTime);
  long hours = diffSeconds / 3600;
  long minutes = (diffSeconds / 60) % 60;
  long seconds = diffSeconds % 60;
  
  Serial.print("⏱️ Total Time Out in Use: ");
  if (hours > 0) { Serial.print(hours); Serial.print("h "); }
  if (minutes > 0 || hours > 0) { Serial.print(minutes); Serial.print("m "); }
  Serial.print(seconds); Serial.println("s");
}

// Helper function to print a clean dashboard of where everything is
void printTrayDashboard() {
  Serial.println("\n===== TRAY STATUS DASHBOARD =====");
  printTimestamp(); Serial.print("Blue Tray:   "); Serial.println(tagAStowed ? "[STOWED AWAY]" : "[OUT IN USE]");
  printTimestamp(); Serial.print("Yellow Tray: "); Serial.println(tagBStowed ? "[STOWED AWAY]" : "[OUT IN USE]");
  Serial.println("==================================\n");
}




// --- NEW HELPER: Post JSON Data to Google Sheets ---
void logEventToGoogleSheets(String trayName, String actionName, long durationOutSeconds) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🌐 Network Error: Wi-Fi offline, log skipped.");
    return;
  }

  HTTPClient http;
  
  // Connect to Google Script Deployment Endpoint
  // Note: HTTPClient natively handles secure link handshakes behind the scenes on newer ESP32 cores!
  http.begin(googleScriptUrl);
  http.addHeader("Content-Type", "application/json");

  // Format the values into a clean JSON transaction payload string
  String jsonPayload = "{\"timestamp\":\"" + getFormattedTimestamp() + 
                       "\",\"tray\":\"" + trayName + 
                       "\",\"action\":\"" + actionName + 
                       "\",\"duration_out_seconds\":" + String(durationOutSeconds) + "}";

  //Serial.print("🚀 Sending row to Google Sheets: ");
  //Serial.println(jsonPayload);
  printTrayDashboard();
  // Send the payload via HTTP POST
  int httpResponseCode = http.POST(jsonPayload);
  
  // CRITICAL: Google Script execution requests always issue an HTTP 302 redirect redirection flag.
  // We track both standard 200 codes or 302 redirections as complete validation successes.
  if (httpResponseCode == HTTP_CODE_OK || httpResponseCode == 302) {
    Serial.println("📡 Google Sheet spreadsheet updated successfully!");
  } else {
    Serial.print("⚠️ Transaction delivery failed. HTTP Response Code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end(); 
}

// Isolated Scanning Routine
void checkAndScanTray() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  
  uint8_t uidLength;   

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

  if (success) {
    if (compareUIDs(uid, uidLength, TARGET_TAG_A, sizeof(TARGET_TAG_A))) {
      Serial.println("Blue Tray Stowed Away");
      setStripColor(40, 70, 255); 
      tagAStowed = true; 
      currentStowedTrayChar = 'A';
      
      long secondsOut = 0;
      if (tagA_leftScannerTime != 0) {
        time_t currentTime; time(&currentTime);
        secondsOut = difftime(currentTime, tagA_leftScannerTime);
      }
      
      logEventToGoogleSheets("Blue", "stowed", secondsOut);
    } 
    else if (compareUIDs(uid, uidLength, TARGET_TAG_B, sizeof(TARGET_TAG_B))) {
      Serial.println("Yellow Tray Stowed Away!");
      setStripColor(254, 198, 0); 
      tagBStowed = true; 
      currentStowedTrayChar = 'B';
      
      long secondsOut = 0;
      if (tagB_leftScannerTime != 0) {
        time_t currentTime; time(&currentTime);
        secondsOut = difftime(currentTime, tagB_leftScannerTime);
      }
      
      logEventToGoogleSheets("Yellow", "stowed", secondsOut);
    } 
    else {
      Serial.println("❓ Unregistered tag read.");
      setStripColor(255, 255, 255);
      currentStowedTrayChar = 'U'; 
      logEventToGoogleSheets("Unknown", "stowed", 0);
    }
  } 
  else {
    Serial.println("⚠️ Switch closed, no RFID detected.");
    setStripColor(255, 255, 255);
    currentStowedTrayChar = 'U'; 
    logEventToGoogleSheets("EmptyObject", "stowed", 0);
  }
}

void setup(void) {
  Serial.begin(115200);

  pinMode(SWITCH_PIN, INPUT_PULLUP);

  strip.begin();
  strip.setBrightness(255); 
  setStripColor(0, 0, 0); 
  strip.show();

  while (!Serial) delay(10);

  Serial.println("\n--- Final Program Booting ---");
  
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  unsigned long startWifiTime = millis();
  const unsigned long wifiTimeout = 15000; 
  
  while (WiFi.status() != WL_CONNECTED && (millis() - startWifiTime) < wifiTimeout) {
    runBreathingAnimation(); 
    yield();                
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi Connected!");
    flashLEDs(0, 255, 0, 2); 
  } else {
    Serial.println("\n❌ Wi-Fi Connection Failed!");
    flashLEDs(255, 0, 0, 4); 
    while (1) delay(10); 
  }

  configTzTime(time_zone, ntpServer);
  Serial.println("Synchronizing wall clock time...");
  
  unsigned long startTimeSync = millis();
  struct tm timeinfo;
  while(!getLocalTime(&timeinfo)){
    runBreathingAnimation(); 
    yield();
    if (millis() - startTimeSync > 10000) {
      Serial.println("\n⚠️ NTP Time Sync Timeout!");
      break;
    }
  }
  Serial.println("\n✅ Time successfully synchronized!");

  Wire.begin(I2C_SDA, I2C_SCL);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("⚠️ ERROR: PN532 chip not detected!");
    setStripColor(255, 0, 0); 
    while (1) delay(10);
  }
  
  nfc.SAMConfig();
  Serial.println("System Ready.");
  
  stableSwitchState = (digitalRead(SWITCH_PIN) == LOW);
  lastRawReading = stableSwitchState;

  if (stableSwitchState == true) {
    checkAndScanTray();
  }
}

void loop(void) {
  yield(); 

  bool currentRawReading = (digitalRead(SWITCH_PIN) == LOW);

  if (currentRawReading != lastRawReading) {
    lastDebounceTime = millis(); 
  }

  lastRawReading = currentRawReading;

  if ((millis() - lastDebounceTime) > debounceDelay) {
    
    if (currentRawReading != stableSwitchState) {
      stableSwitchState = currentRawReading; 

      if (stableSwitchState == true) {
        checkAndScanTray();
      }
      else {
        runBreathingAnimation();
        if (currentStowedTrayChar == 'A') {
          tagAStowed = false;
          time(&tagA_leftScannerTime); 
          logEventToGoogleSheets("Blue", "removed", 0); 
        } 
        else if (currentStowedTrayChar == 'B') {
          tagBStowed = false;
          time(&tagB_leftScannerTime); 
          logEventToGoogleSheets("Yellow", "removed", 0);
        } 
        else {
          logEventToGoogleSheets("Unknown", "removed", 0);
        }
        currentStowedTrayChar = ' '; 
      }
    }
  }

}