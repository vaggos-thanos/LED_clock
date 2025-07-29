// ====== LED Clock Display on 4x 8x8 Dual-Color Matrix ======
// Author: Vaggos
// Description: Real-time clock display with blinking colon on 32x8 dual-color LED matrix with ESP32 RTC + NTP + WebSockets
// Credits: GPT assisted on the pixel and pattern draw

#include <Arduino.h>
#include <ArduinoJson.h>

// NTP and WiFi
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// OTA
#include <WebServer.h>
#include <Update.h>
// Server Index Page
#include "html.h"

// WS
#include <WebSocketsClient_Generic.h>
#include <SocketIOclient_Generic.h>
#define _WEBSOCKETS_LOGLEVEL_     4

// Custom display chars
#include "font5x7.h"

// Store data
#include "EEPROM.h"

// PINS
const int dataPin = 12;
const int strobePin = 14;
const int clockPinGreen = 25;
const int clockPinRed = 26;
const int rowPins[8] = {19, 18, 5, 17, 16, 4, 2, 15};

uint8_t redBuffer[8][32] = {0};
uint8_t greenBuffer[8][32] = {0};

bool colonVisible = true;
unsigned long lastBlink = 0;
const unsigned long blinkInterval = 500;  // Increased blink for visibility
const unsigned int ROW_ACTIVE_TIME_US = 1200;  // Increased brightness

unsigned long lastSecondUpdate = 0;

int fakeModeTimeLeft = 0;

int fakeHour = 0;
int fakeMinute = 0;
int fakeSecond = 0;

int startPos = 1;
bool testMode = false;
bool fakeMode = true;
char testMessage[32] = "";

// WIFI
const char* host = "esp32";
const char* ssid = "iot-network";
const char* password = "HpM#v#vpbQ@8f*";
#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64

// SOCKET IO
IPAddress serverIP(192, 168, 101, 2); //Enter server adress
uint16_t serverPort = 3000; // Enter server port
SocketIOclient  socketIO;

// NTP - Modified for better timing
const char* ntpServer = "gr.pool.ntp.org";
const long gmtOffset_sec = 3 * 3600;
const int daylightOffset_sec = 0;
unsigned long ntpSyncInterval = 120000; // 15 minutes

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec + daylightOffset_sec, ntpSyncInterval); // 15 minutes

bool ntpStarted = false;
bool ntpInitialized = false;
bool connection = false;

// Improved timing variables
unsigned long lastNTPSync = 0;
unsigned long lastSecondMillis = 0;
int lastSyncedHour = -1;
int lastSyncedMinute = -1;
int lastSyncedSecond = -1;
bool timeInitialized = false;
unsigned long networkLatencyMs = 150; // Estimate your network latency

// NTP accuracy tracking
int ntpSyncAttempts = 0;
const int maxSyncAttempts = 5;
const int syncRetryDelay = 5000; // 5 seconds between retries
unsigned long lastSyncAttempt = 0;
bool syncInProgress = false;

// CLOCK COLORS
int hour_color = 2;
int minute_color = 2;
int seconds_color = 2;

int colon_color = 1;

// Colon Modes
// O -> disabled
// 1 -> enabled static
// 2 -> 1 second blink

int colon_mode = 1; 

// EEPROM ADDRESS
#define EEPROM_SIZE 4096
int ADDR_hour = 0;
int ADDR_minute = 1;
int ADDR_seconds = 2;

int ADDR_hour_color = 3;
int ADDR_minute_color = 4;
int ADDR_seconds_color = 5;

int ADDR_colon = 5;
int ADDR_colon_color = 6;

int ADDR_wifi_client_ssid = 8;
int ADDR_wifi_client_pass = 40;

// init EEPROM with magic number so we can be sure
// if the EEPROM has been set previously or not
const int ADDR_MAGIC = 7;
const int MAGIC_NUMBER = 42;

int nextUpdate = 0;

void saveSettings() {

  EEPROM.write(ADDR_hour, lastSyncedHour);
  EEPROM.write(ADDR_minute, lastSyncedMinute);
  EEPROM.write(ADDR_seconds, lastSyncedSecond);

  EEPROM.write(ADDR_hour_color, hour_color);
  EEPROM.write(ADDR_minute_color, minute_color);
  EEPROM.write(ADDR_seconds_color, seconds_color);

  EEPROM.write(ADDR_colon, colon_mode);
  EEPROM.write(ADDR_colon_color, colon_color);

  // Store SSID and password as strings
  for (int i = 0; i < MAX_SSID_LEN; i++) {
    EEPROM.write(ADDR_wifi_client_ssid + i, i < strlen(ssid) ? ssid[i] : 0);
  }
  for (int i = 0; i < MAX_PASS_LEN; i++) {
    EEPROM.write(ADDR_wifi_client_pass + i, i < strlen(password) ? password[i] : 0);
  }

  EEPROM.write(ADDR_MAGIC, MAGIC_NUMBER);

  EEPROM.commit();
  
  Serial.println("Settings saved to EEPROM");
}

void loadSettings() {
  if (EEPROM.read(ADDR_MAGIC) == MAGIC_NUMBER) {
    lastSyncedHour = EEPROM.read(ADDR_hour);
    lastSyncedMinute = EEPROM.read(ADDR_minute);
    lastSyncedSecond = EEPROM.read(ADDR_seconds);

    fakeHour = lastSyncedHour;
    fakeMinute = lastSyncedMinute;
    fakeSecond = lastSyncedSecond;

    hour_color = EEPROM.read(ADDR_hour_color);
    minute_color = EEPROM.read(ADDR_minute_color);
    seconds_color = EEPROM.read(ADDR_seconds_color);

    colon_mode = EEPROM.read(ADDR_colon);
    colon_color = EEPROM.read(ADDR_colon_color);

    // Load SSID and password
    static char storedSSID[MAX_SSID_LEN];
    static char storedPASS[MAX_PASS_LEN];
    for (int i = 0; i < MAX_SSID_LEN; i++) {
      storedSSID[i] = EEPROM.read(ADDR_wifi_client_ssid + i);
    }
    storedSSID[MAX_SSID_LEN - 1] = '\0';

    for (int i = 0; i < MAX_PASS_LEN; i++) {
      storedPASS[i] = EEPROM.read(ADDR_wifi_client_pass + i);
    }
    storedPASS[MAX_PASS_LEN - 1] = '\0';

    ssid = storedSSID;
    password = storedPASS;

    Serial.printf("Loaded SSID: %s\n", ssid);
    Serial.printf("Loaded PASS: %s\n", password);
    
    Serial.printf("Loaded lastSyncedHour: %i\n", lastSyncedHour);
    Serial.printf("Loaded lastSyncedMinute: %i\n", lastSyncedMinute);
    Serial.printf("Loaded lastSyncedSecond: %i\n", lastSyncedSecond);
    
    Serial.printf("Loaded fakeHour: %i\n", fakeHour);
    Serial.printf("Loaded fakeMinute: %i\n", fakeMinute);
    Serial.printf("Loaded fakeSecond: %i\n", fakeSecond);
    
    Serial.printf("Loaded hour_color: %i\n", hour_color);
    Serial.printf("Loaded minute_color: %i\n", minute_color);
    Serial.printf("Loaded seconds_color: %i\n", seconds_color);

    Serial.printf("Loaded colon_mode: %i\n", colon_mode);
    Serial.printf("Loaded colon_color: %i\n", colon_color);
   
  } else {
    Serial.println("No saved settings found, using defaults");
    saveSettings();
  }
}

// OTA
WebServer server(80);
bool matrixPreviewEnabled = true;
unsigned long lastMatrixStateUpdate = 0;
const unsigned long MATRIX_UPDATE_INTERVAL = 100;

 
// Multithreding
#if CONFIG_FREERTOS_UNICORE
  #define TASK_RUNNING_CORE 0
#else
  #define TASK_RUNNING_CORE 1
#endif

void handleWiFiConnection(void *pvParameters);
TaskHandle_t WiFiTaskHandle;


// ===== FIXED GREETING MANAGEMENT =====

// Global variables for greeting system
bool greetingShown = false;
bool greetingDisplayed = false; // NEW: track if greeting was actually displayed
unsigned long greetingStartTime = 0;
const unsigned long greetingDuration = 60000; // Show greeting for 8 seconds
String currentHostname = "";


void setup() {
  Serial.begin(115200);
  xTaskCreatePinnedToCore(handleWiFiConnection, "WiFi handler", 4096, NULL, 1, &WiFiTaskHandle, TASK_RUNNING_CORE);

  pinMode(dataPin, OUTPUT);
  pinMode(clockPinRed, OUTPUT);
  pinMode(clockPinGreen, OUTPUT);
  pinMode(strobePin, OUTPUT);
  for (int i = 0; i < 8; i++) pinMode(rowPins[i], OUTPUT);

  EEPROM.begin(EEPROM_SIZE);
  loadSettings();
}

void setPixel(uint8_t x, uint8_t y, uint8_t color) {
  if (x >= 32 || y >= 8) return;
  redBuffer[y][x] = (color & 1);
  greenBuffer[y][x] = (color & 2) >> 1;
}

void drawClockDigits(int hour, int minute, int second) {
  memset(redBuffer, 0, sizeof(redBuffer));
  memset(greenBuffer, 0, sizeof(greenBuffer));
  drawChar(0 + startPos, 0, '0' + hour / 10, hour_color);
  drawChar(5 + startPos, 0, '0' + hour % 10, hour_color);

  if (colonVisible) drawChar(9 + startPos, 0, ':', colon_color);
  drawChar(10 + startPos, 0, '0' + minute / 10, minute_color);
  drawChar(15 + startPos, 0, '0' + minute % 10, minute_color);

  if (colonVisible) drawChar(19 + startPos, 0, ':', colon_color);
  drawChar(20 + startPos, 0, '0' + second / 10, seconds_color);
  drawChar(25 + startPos, 0, '0' + second % 10, seconds_color);
}

void sendPattern(uint32_t redPattern, uint32_t greenPattern) {
  for (int i = 31; i >= 0; i--) {
    digitalWrite(dataPin, (redPattern >> i) & 1);
    digitalWrite(clockPinRed, HIGH); digitalWrite(clockPinRed, LOW);
  }
  for (int i = 31; i >= 0; i--) {
    digitalWrite(dataPin, (greenPattern >> i) & 1);
    digitalWrite(clockPinGreen, HIGH); digitalWrite(clockPinGreen, LOW);
  }
  digitalWrite(strobePin, HIGH); delayMicroseconds(2); digitalWrite(strobePin, LOW);
}

void activateRow(int row) { digitalWrite(rowPins[row], HIGH); }
void deactivateRow(int row) { digitalWrite(rowPins[row], LOW); }
void activateAllRows() { for (int i = 0; i < 8; i++) digitalWrite(rowPins[i], HIGH); }
void deactivateAllRows() { for (int i = 0; i < 8; i++) digitalWrite(rowPins[i], LOW); }

void updateRows() {
  // Add complete blanking at start
  deactivateAllRows();
  sendPattern(0, 0);  // Clear shift registers
  delayMicroseconds(75);  // Longer blanking period
  
  for (int row = 0; row < 8; row++) {
    uint32_t redPattern = 0, greenPattern = 0;
    for (int col = 0; col < 32; col++) {
      if (redBuffer[row][col]) redPattern |= (1UL << (31 - col));
      if (greenBuffer[row][col]) greenPattern |= (1UL << (31 - col));
    }
    
    // Send data while all rows are off
    sendPattern(redPattern, greenPattern);
    delayMicroseconds(10);  // Let data settle
    
    // Activate row
    activateRow(row);
    delayMicroseconds(ROW_ACTIVE_TIME_US);
    
    // Deactivate with delay
    deactivateRow(row);
    delayMicroseconds(30);  // Ensure row is fully off
  }
}

// Improved drawChar function with boundary checking
void drawChar(uint8_t x, uint8_t y, char c, uint8_t color) {
  if (c < 32 || c > 126) return;
  
  uint8_t index = c - 32;
  
  for (uint8_t col = 0; col < 5; col++) {
    // Check if we're still within display bounds
    if (x + col >= 32) break;
    
    uint8_t line = font5x7[index][col];
    for (uint8_t row = 0; row < 7; row++) {
      if (y + row >= 8) break; // Don't draw outside display
      
      if (line & (1 << row)) {
        setPixel(x + col, y + row, color);
      }
    }
  }
}

// NEW: Improved drawText with optional clear and scroll support
void drawText(int16_t x, uint8_t y, const char* str, uint8_t color, bool clearBuffer = true) {
  if (clearBuffer) {
    memset(redBuffer, 0, sizeof(redBuffer));
    memset(greenBuffer, 0, sizeof(greenBuffer));
  }
  
  int16_t currentX = x;
  const char* ptr = str;
  
  while (*ptr && currentX < 32) {
    // Only draw characters that are at least partially visible
    if (currentX + 5 > 0) {
      drawChar(currentX, y, *ptr, color);
    }
    
    currentX += 6; // 5 pixels wide + 1 pixel spacing
    ptr++;
  }
}

// NEW: Calculate text width in pixels
int16_t getTextWidth(const char* str) {
  int len = strlen(str);
  if (len == 0) return 0;
  return (len * 6) - 1; // Each char is 6 pixels wide (5 + 1 spacing), minus 1 for no trailing space
}

// NEW: Scrolling text function with better control
class TextScroller {
  private:
    char text[256];
    int16_t position;
    int16_t textWidth;
    uint8_t yPos;
    uint8_t textColor;
    unsigned long lastUpdate;
    unsigned long scrollSpeed;
    bool isActive;
    
  public:
    TextScroller() : position(32), textWidth(0), yPos(0), textColor(1), 
                    lastUpdate(0), scrollSpeed(100), isActive(false) {
      text[0] = '\0';
    }
    
    void setText(const char* newText, uint8_t y = 0, uint8_t color = 1, unsigned long speed = 100) {
      strncpy(text, newText, sizeof(text) - 1);
      text[sizeof(text) - 1] = '\0';
      
      textWidth = getTextWidth(text);
      yPos = y;
      textColor = color;
      scrollSpeed = speed;
      position = 32; // Start from right edge
      isActive = true;
      lastUpdate = millis();
    }
    
    void stop() {
      isActive = false;
    }
    
    bool update() {
      if (!isActive) return false;
      
      unsigned long currentTime = millis();
      if (currentTime - lastUpdate < scrollSpeed) {
        return true; // Still active, but no update needed yet
      }
      
      lastUpdate = currentTime;
      position--;
      
      // Reset when text has completely scrolled off the left
      if (position < -textWidth) {
        position = 32;
      }
      
      // Draw the text at current position
      drawText(position, yPos, text, textColor, true);
      
      return true;
    }
    
    bool isScrolling() const {
      return isActive;
    }
    
    void setSpeed(unsigned long speed) {
      scrollSpeed = speed;
    }
    
    int16_t getPosition() const {
      return position;
    }
};

// Global scroller instance
TextScroller textScroller;

void setupHostname() {
  // Check if hostname is already set, if not set it to matrixclock.local
  currentHostname = WiFi.getHostname();
  
  if (currentHostname.isEmpty() || currentHostname.equals("esp32")) {
    const char* newHostname = "matrixclock";
    if (WiFi.setHostname(newHostname)) {
      currentHostname = String(newHostname) + ".local";
      Serial.println("Hostname set to: " + currentHostname);
    } else {
      Serial.println("Failed to set hostname, using default");
      currentHostname = "esp32.local";
    }
  } else {
    if (!currentHostname.endsWith(".local")) {
      currentHostname += ".local";
    }
    Serial.println("Current hostname: " + currentHostname);
  }
}

void showGreetingMessage() {
  if (!greetingDisplayed) { // Only show once per boot/connection
    String greetingText = "Matrix Clock Ready! IP: " + WiFi.localIP().toString() + " Host: " + currentHostname;
    textScroller.setText(greetingText.c_str(), 0, 2, 120); // Green text, readable speed
    greetingShown = true;
    greetingDisplayed = true;
    greetingStartTime = millis();
    Serial.println("Showing greeting: " + greetingText);
  }
}

void handleSerial() {
  static String input = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      input.trim();
      if (input.startsWith("TIME ")) {
        int h = input.substring(5, 7).toInt();
        int m = input.substring(8, 10).toInt();
        int s = input.substring(11, 13).toInt();
        if (h >= 0 && h < 24 && m >= 0 && m < 60 && s >= 0 && s < 60) {
          fakeHour = h; fakeMinute = m; fakeSecond = s;
          textScroller.stop(); // Stop scrolling when setting time
          testMode = false;
        }
      } else if (input.startsWith("SCROLL ")) {
        // NEW: SCROLL command with optional parameters
        // Usage: SCROLL Hello World [speed] [color] [y]
        String text = input.substring(7);
        int spacePos1 = text.lastIndexOf(' ');
        int spacePos2 = text.lastIndexOf(' ', spacePos1 - 1);
        int spacePos3 = text.lastIndexOf(' ', spacePos2 - 1);
        
        unsigned long speed = 100;  // Default speed
        uint8_t color = 1;          // Default color (red)
        uint8_t y = 0;              // Default y position
        
        // Parse optional parameters from the end
        if (spacePos1 > 0 && text.substring(spacePos1 + 1).toInt() > 0) {
          y = text.substring(spacePos1 + 1).toInt();
          text = text.substring(0, spacePos1);
          
          if (spacePos2 > 0 && text.substring(spacePos2 + 1).toInt() > 0) {
            color = text.substring(spacePos2 + 1).toInt();
            text = text.substring(0, spacePos2);
            
            if (spacePos3 > 0 && text.substring(spacePos3 + 1).toInt() > 0) {
              speed = text.substring(spacePos3 + 1).toInt();
              text = text.substring(0, spacePos3);
            }
          }
        }
        
        textScroller.setText(text.c_str(), y, color, speed);
        testMode = false;
        Serial.println("Scrolling: " + text + " (speed=" + speed + ", color=" + color + ", y=" + y + ")");
        
      } else if (input.startsWith("TEXT ")) {
        // Keep existing TEXT command for compatibility
        strncpy(testMessage, input.substring(5).c_str(), sizeof(testMessage) - 1);
        textScroller.stop();
        testMode = true;
        
      } else if (input.equals("GREETING")) {
        // NEW: Show greeting message manually
        if (WiFi.isConnected()) {
          String greetingText = "Matrix Clock Ready! IP: " + WiFi.localIP().toString() + " Host: " + currentHostname;
          textScroller.setText(greetingText.c_str(), 0, 2, 120);
          Serial.println("Manual greeting: " + greetingText);
        } else {
          textScroller.setText("Matrix Clock - WiFi Connecting...", 0, 1, 120);
          Serial.println("Manual greeting: WiFi not connected");
        }
        greetingShown = true;
        greetingStartTime = millis();
        
      } else if (input.equals("HOSTNAME")) {
        // NEW: Show current hostname info
        Serial.println("=== HOSTNAME INFO ===");
        Serial.println("Current hostname: " + currentHostname);
        Serial.println("WiFi hostname: " + String(WiFi.getHostname()));
        if (WiFi.isConnected()) {
          Serial.println("IP address: " + WiFi.localIP().toString());
          Serial.println("Access via: http://" + currentHostname);
          Serial.println("Access via: http://" + WiFi.localIP().toString());
        }
        Serial.println("====================");
        
      } else if (input.startsWith("SETHOSTNAME ")) {
        // NEW: Set custom hostname
        String newHostname = input.substring(12);
        newHostname.trim();
        if (newHostname.length() > 0) {
          if (WiFi.setHostname(newHostname.c_str())) {
            currentHostname = newHostname + ".local";
            Serial.println("Hostname changed to: " + currentHostname);
            // Show confirmation message
            textScroller.setText(("Hostname: " + currentHostname).c_str(), 0, 3, 120);
          } else {
            Serial.println("Failed to set hostname to: " + newHostname);
          }
        }
        
      } else if (input.equals("SYNC")) {
        // [Keep your existing SYNC command code]
      } else if (input.startsWith("LATENCY ")) {
        // [Keep your existing LATENCY command code]
      } else if (input.equals("STATUS")) {
        // [Keep your existing STATUS command code, but add scroll status]
        Serial.println("=== NTP SYNC STATUS ===");
        // ... existing status code ...
        Serial.println("Scroll active: " + String(textScroller.isScrolling() ? "YES" : "NO"));
        if (textScroller.isScrolling()) {
          Serial.println("Scroll position: " + String(textScroller.getPosition()));
        }
        Serial.println("Greeting shown: " + String(greetingShown ? "YES" : "NO"));
        Serial.println("Current hostname: " + currentHostname);
        if (WiFi.isConnected()) {
          Serial.println("IP address: " + WiFi.localIP().toString());
        }
        Serial.println("======================");
      }
      Serial.println("Serial: " + input);
      input = "";
    } else {
      input += c;
    }
  }
}

// NTP validation function
bool validateNTPTime() {
  int hour = timeClient.getHours();
  int minute = timeClient.getMinutes();
  int second = timeClient.getSeconds();
  
  // Basic sanity checks
  if (hour < 0 || hour > 23) return false;
  if (minute < 0 || minute > 59) return false;
  if (second < 0 || second > 59) return false;
  
  // Check if time seems reasonable
  unsigned long epochTime = timeClient.getEpochTime();
  if (epochTime < 1640995200) return false;
  
  // If we have previous time, check for reasonable progression
  if (timeInitialized && ntpSyncAttempts == 0) { // Only validate on first attempt
    int prevTotalSeconds = lastSyncedHour * 3600 + lastSyncedMinute * 60 + lastSyncedSecond;
    int currentTotalSeconds = hour * 3600 + minute * 60 + second;
    
    // Use actual time since last successful sync
    unsigned long timeSinceLastSync = (millis() - lastSecondMillis) / 1000; // Use lastSecondMillis instead
    int expectedSeconds = (prevTotalSeconds + timeSinceLastSync) % (24 * 3600);
    
    // Allow larger tolerance for time jumps (30 seconds instead of 10)
    int timeDiff = abs(currentTotalSeconds - expectedSeconds);
    if (timeDiff > 30 && timeDiff < (24 * 3600 - 30)) {
      Serial.println("NTP validation failed: time jump detected. Expected ~" + String(expectedSeconds) + "s, got " + String(currentTotalSeconds) + "s");
      return false;
    }
  }
  
  return true;
}

void handleWiFiConnection(void *pvParameters) {
  (void)pvParameters;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid, password);

  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[WiFi] Connected. IP: " + WiFi.localIP().toString());
      vTaskDelete(NULL);
    }
    delay(500);
  }
}

void sendSocketMessage(const char* type, const String& message) {
  DynamicJsonDocument doc(1024);

  JsonArray array = doc.to<JsonArray>();
  array.add("message");  // Event name
  JsonObject data = array.createNestedObject();
  data["status"] = "success";
  data["type"] = type;
  data["message"] = message;

  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);
}

void sendSettingsDebug() {
  StaticJsonDocument<512> doc;

  doc["hour"] = lastSyncedHour;
  doc["minute"] = lastSyncedMinute;
  doc["second"] = lastSyncedSecond;

  doc["fakehour"] = fakeHour;
  doc["fakeminute"] = fakeMinute;
  doc["fakesecond"] = fakeSecond;

  doc["hour_color"] = hour_color;
  doc["minute_color"] = minute_color;
  doc["seconds_color"] = seconds_color;

  doc["colon_mode"] = colon_mode;
  doc["colon_color"] = colon_color;

  doc["ssid"] = ssid;
  doc["password"] = password;

  String jsonString;
  serializeJson(doc, jsonString);

  sendSocketMessage("debug", jsonString);
}

void sendMatrixStateWS() {
    if (!WiFi.isConnected()) return;

    DynamicJsonDocument doc(2048);
    doc["type"] = "matrix_update";

    JsonArray matrix = doc.createNestedArray("data");
    for (int y = 0; y < 8; y++) {
        JsonArray row = matrix.createNestedArray();
        for (int x = 0; x < 32; x++) {
            bool isLit = (redBuffer[y][x] || greenBuffer[y][x]);
            row.add(isLit);
        }
    }

    String output;
    serializeJson(doc, output);
    socketIO.sendEVENT(output);
}

void socketIOEvent(const socketIOmessageType_t& type, uint8_t * payload, const size_t& length) {
  switch (type)  {
    case sIOtype_DISCONNECT:
      Serial.println("[IOc] Disconnected");
      break;

    case sIOtype_CONNECT:
      Serial.print("[IOc] Connected to url: ");
      Serial.println((char*) payload);
      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, "/");

      sendSettingsDebug();
      break;

    case sIOtype_EVENT:
      Serial.print("[Socket.IO] Event: ");
      Serial.println((char*) payload);
    break;

    case sIOtype_BINARY_EVENT: 
      Serial.printf("[Socket.IO] Binary data received: %d bytes\n", length);
    break;

    case sIOtype_ACK:
      Serial.print("[IOc] Get ack: ");
      Serial.println(length);
      //hexdump(payload, length);
      break;

    case sIOtype_ERROR:
      Serial.print("[IOc] Get error: ");
      Serial.println(length);
      //hexdump(payload, length);
      break;

    case sIOtype_BINARY_ACK:
      Serial.print("[IOc] Get binary ack: ");
      Serial.println(length);
      //hexdump(payload, length);
      break;

    default:
      break;
  }
}

void handleOTA() {
      server.on("/", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(404, "text/html", "POU PAS FILE ??");
      });

      server.on("/serverIndex", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
      });

        server.on("/matrix-state", HTTP_GET, []() {
            DynamicJsonDocument doc(2048);
            JsonArray array = doc.createNestedArray();

            // Convert the LED matrix buffers to JSON
            for (int y = 0; y < 8; y++) {
                JsonArray row = array.createNestedArray();
                for (int x = 0; x < 32; x++) {
                    JsonObject pixel = row.createNestedObject();
                    pixel["red"] = redBuffer[y][x] == 1;
                    pixel["green"] = greenBuffer[y][x] == 1;
                }
            }

            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        });

        // Add WiFi status endpoint
        server.on("/status", HTTP_GET, []() {
            DynamicJsonDocument doc(256);
            doc["internet"] = WiFi.isConnected();
            doc["ip"] = WiFi.localIP().toString();
            doc["rssi"] = WiFi.RSSI();
            doc["hostname"] = WiFi.getHostname();

            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        });

  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      sendSocketMessage("debug", "Update: " + String(upload.filename.c_str()) + "\n");
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        sendSocketMessage("debug", "Update Success: " + String(upload.totalSize) + "\nRebooting...\n");
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.begin();

  Serial.println("Web Server Started!");
}

void loop() {
  handleSerial();

  if (WiFi.isConnected() && !ntpInitialized) {
    // Setup hostname when WiFi connects
    setupHostname();
    
    timeClient.begin();
    ntpStarted = true;
    ntpInitialized = true;
    handleOTA();

    // Show greeting message when first connected
    showGreetingMessage();

    socketIO.setReconnectInterval(10000);
    socketIO.begin(serverIP, serverPort);
    Serial.print("Socket IO connecting to: ");
    Serial.print(serverIP);
    Serial.print(":");
    Serial.println(serverPort);
    socketIO.onEvent(socketIOEvent);

    textScroller.stop();
    greetingDisplayed = false;

  }

  if(WiFi.isConnected() && ntpStarted){
    static int lastDrawnSecond = -1;
    
    // Only update NTP every 15 minutes instead of every loop
    unsigned long currentMillis = millis();
    
    // Handle NTP synchronization with retries and validation
    bool needsSync = (currentMillis - lastNTPSync > ntpSyncInterval) || !timeInitialized;
    bool canRetry = !syncInProgress && (currentMillis - lastSyncAttempt > syncRetryDelay);
    
    if ((needsSync || ntpSyncAttempts > 0) && canRetry) {
      syncInProgress = true;
      lastSyncAttempt = currentMillis;
      
      Serial.println("Attempting NTP sync... (attempt " + String(ntpSyncAttempts + 1) + "/" + String(maxSyncAttempts) + ")");
      
      // Force update and measure sync time
      unsigned long syncStartTime = millis();
      bool syncSuccess = timeClient.forceUpdate();
      unsigned long syncDuration = millis() - syncStartTime;
      
      if (syncSuccess && validateNTPTime()) {
        // Successful sync
        unsigned long preciseCurrentMillis = millis();
        lastNTPSync = preciseCurrentMillis;
        timeInitialized = true;
        ntpSyncAttempts = 0;
        syncInProgress = false;
        
        // Store the synced time with improved precision
        lastSyncedHour = timeClient.getHours();
        lastSyncedMinute = timeClient.getMinutes();
        lastSyncedSecond = timeClient.getSeconds();
        
        // Calculate more accurate latency compensation
        unsigned long estimatedLatency = (syncDuration / 2) + networkLatencyMs;
        lastSecondMillis = preciseCurrentMillis - estimatedLatency;
        
        String syncMsg = "NTP sync SUCCESS (took " + String(syncDuration) + "ms, latency ~" + String(estimatedLatency) + "ms): " + String(timeClient.getFormattedTime());
        Serial.println(syncMsg);
        sendSocketMessage("debug", syncMsg);
          
      } else {
        // Failed sync
        ntpSyncAttempts++;
        String failMsg = "NTP sync FAILED (attempt " + String(ntpSyncAttempts) + "/" + String(maxSyncAttempts) + ")";
        
        if (ntpSyncAttempts >= maxSyncAttempts) {
          ntpSyncAttempts = 0;
          syncInProgress = false;
          failMsg += " - Max attempts reached, will retry in " + String(ntpSyncInterval/1000) + " seconds";
          lastNTPSync = currentMillis; // Prevent immediate retry
        } else {
          syncInProgress = false; // Allow retry after delay
          failMsg += " - Retrying in " + String(syncRetryDelay/1000) + " seconds";
        }
        
        Serial.println(failMsg);
        sendSocketMessage("error", failMsg);
      }
    }
    
    if (timeInitialized) {
      // Calculate current time based on milliseconds elapsed since last NTP sync
      unsigned long millisSinceSync = currentMillis - lastSecondMillis;
      int secondsElapsed = millisSinceSync / 1000;
      unsigned long millisInCurrentSecond = millisSinceSync % 1000;
      
      int totalSecondsAtSync = lastSyncedHour * 3600 + lastSyncedMinute * 60 + lastSyncedSecond;
      int currentTotalSeconds = (totalSecondsAtSync + secondsElapsed) % (24 * 3600);
      
      int currentHour = currentTotalSeconds / 3600;
      int currentMinute = (currentTotalSeconds % 3600) / 60;
      int currentSecond = currentTotalSeconds % 60;
      
      // Handle text scrolling vs clock display
      // disable the fake mode aka the greeting
      if (fakeModeTimeLeft == 1 && fakeMode) {
        fakeMode = false;
        fakeModeTimeLeft = 0;
      }
      
      if (!fakeMode) {
        // Update display when second changes (normal clock mode)
        if (currentSecond != lastDrawnSecond) {
          lastDrawnSecond = currentSecond;
          drawClockDigits(currentHour, currentMinute, currentSecond);
        }
        
        // Handle colon blinking with millisecond precision
        if (colon_mode == 0) {
          colonVisible = false;
        } else if (colon_mode == 1) {
          colonVisible = true;
        } else if (colon_mode == 2) {
          colonVisible = (secondsElapsed % 2);
        }
      }
      
      // Re-sync our internal tracking periodically for drift correction
      if (secondsElapsed > 0 && millisInCurrentSecond < 100 && (secondsElapsed % 60) == 0) {
        // Update our sync point every minute to prevent drift
        lastSyncedHour = currentHour;
        lastSyncedMinute = currentMinute;
        lastSyncedSecond = currentSecond;
        lastSecondMillis = currentMillis - millisInCurrentSecond;

        nextUpdate = nextUpdate + 1;
        fakeModeTimeLeft = fakeModeTimeLeft + 1;
      }

      // update settings every 30 min
      if (nextUpdate == 30) {
        sendSocketMessage("debug", "Settings updated!");
        saveSettings();
        loadSettings();
        sendSettingsDebug();
        nextUpdate = 0;
      }
    }

    server.handleClient();
    socketIO.loop();
  }

  if(fakeMode) {
    static unsigned long lastFakeUpdate = 0;
    
    // Handle one-time greeting message in fake mode
    if (!greetingDisplayed && !textScroller.isScrolling()) {
      // Show greeting when entering fake mode (only once)
      String greetingText;
      if (WiFi.isConnected()) {
        greetingText = "IP: " + WiFi.localIP().toString() + " Host: " + currentHostname;
      } else {
        greetingText = "Matrix Clock - WiFi Connecting...";
      }
      textScroller.setText(greetingText.c_str(), 0, 2, 120); // Green text, readable speed
      greetingShown = true;
      greetingDisplayed = true;
      greetingStartTime = millis();
      Serial.println("Fake mode greeting: " + greetingText);
    }
    
    // Check if greeting should timeout and switch to clock mode
    if (greetingShown && textScroller.isScrolling()) {
      if (millis() - greetingStartTime > greetingDuration) {
        textScroller.stop();
        greetingShown = false;
        Serial.println("Greeting timeout - switching to fake clock mode");
      }
    }
    
    // Handle different display modes
    if (textScroller.isScrolling()) {
      // Currently scrolling text (greeting or manual scroll)
      textScroller.update();
    } else if (testMode) {
      // Legacy test mode with old scrolling (for compatibility)
      static uint8_t scrollOffset = 0;
      static unsigned long lastScroll = 0;
      const unsigned long scrollDelay = 100;
      
      drawText(scrollOffset, 0, testMessage, 3);
      if (millis() - lastScroll > scrollDelay) {
        lastScroll = millis();
        scrollOffset--;
        if (scrollOffset < -((int)strlen(testMessage) * 6)) scrollOffset = 32;
      }
    } else if (!greetingShown) {
      // Normal fake clock mode (only when not showing greeting)
      if (millis() - lastFakeUpdate >= 1000) {
        lastFakeUpdate = millis();
        colonVisible = !colonVisible;
        fakeSecond++;
        if (fakeSecond >= 60) {
          fakeSecond = 0;
          fakeMinute++;
          if (fakeMinute >= 60) {
            fakeMinute = 0;
            fakeHour++;
            if (fakeHour >= 24) fakeHour = 0;
          }
        }
        drawClockDigits(fakeHour, fakeMinute, fakeSecond);
      }
    }
  }

  updateRows();
}