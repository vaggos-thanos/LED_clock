// ====== LED Clock Display on 4x 8x8 Dual-Color Matrix ======
// Author: ChatGPT + Vaggos
// Description: Real-time clock display with blinking colon on 32x8 dual-color LED matrix with ESP32 RTC + NTP + WebSockets

#include <Arduino.h>

// NTP and WiFi
#include <WiFi.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// OTA
#include <WebServer.h>
#include <Update.h>

// Custom display chars
#include "font5x7.h"

const int dataPin = 32;
const int strobePin = 33;
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

int fakeHour = 0;
int fakeMinute = 0;
int fakeSecond = 0;

int startPos = 1;
bool testMode = false;
bool fakeMode = true;
char testMessage[32] = "";

const char* host = "esp32";
const char* ssid = "iot-network";
const char* password = "HpM#v#vpbQ@8f*";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3 * 3600;
const int daylightOffset_sec = 0;
bool connection = false;

#if CONFIG_FREERTOS_UNICORE
  #define TASK_RUNNING_CORE 0
#else
  #define TASK_RUNNING_CORE 1
#endif

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "gr.pool.ntp.org", gmtOffset_sec + daylightOffset_sec, 60000);
bool ntpStarted = false;
bool ntpInitialized = false;

// OTA
WebServer server(80);
/*
 * Login page
 */
const char* loginIndex = 
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<td>Username:</td>"
        "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
  "</form>"
  "<script>"
      "function check(form)"
      "{"
      "if(form.userid.value=='admin' && form.pwd.value=='admin')"
      "{"
      "window.open('/serverIndex')"
      "}"
      "else"
      "{"
      " alert('Error Password or Username')/*displays error message*/"
      "}"
      "}"
  "</script>";
 
/*
 * Server Index Page
 */
 
const char* serverIndex = 
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
          "<input type='submit' value='Update'>"
      "</form>"
  "<div id='prg'>progress: 0%</div>"
  "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')" 
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
"</script>";
 
// Multithreding
void handleWiFiConnection(void *pvParameters);
TaskHandle_t WiFiTaskHandle;


void setup() {
  Serial.begin(115200);
  xTaskCreatePinnedToCore(handleWiFiConnection, "WiFi handler", 4096, NULL, 1, &WiFiTaskHandle, TASK_RUNNING_CORE);

  pinMode(dataPin, OUTPUT);
  pinMode(clockPinRed, OUTPUT);
  pinMode(clockPinGreen, OUTPUT);
  pinMode(strobePin, OUTPUT);
  for (int i = 0; i < 8; i++) pinMode(rowPins[i], OUTPUT);
}

void loop() {
  static uint8_t scrollOffset = 0;
  static unsigned long lastScroll = 0;
  const unsigned long scrollDelay = 100;

  handleSerial();

  if (WiFi.isConnected() && !ntpInitialized) {
    timeClient.begin();
    ntpStarted = true;
    ntpInitialized = true;
    handleOTA();
  }

  if(WiFi.isConnected() && ntpStarted){
    static int lastDrawnSecond = -1;
    timeClient.update();

    int currentSecond = timeClient.getSeconds();
    int currentMinute = timeClient.getMinutes();
    int currentHour = timeClient.getHours();

    if (currentSecond != lastDrawnSecond) {
      lastDrawnSecond = currentSecond;
      colonVisible = !colonVisible;
      drawClockDigits(currentHour, currentMinute, currentSecond);
    }

    fakeMode = false;

    server.handleClient();
  }

  if(fakeMode) {
    static unsigned long lastFakeUpdate = 0;
    if (!testMode) {
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
        updateRows();

      }
    } else {
      drawText(scrollOffset, 0, testMessage, 3);
      if (millis() - lastScroll > scrollDelay) {
        lastScroll = millis();
        scrollOffset--;
        if (scrollOffset < -((int)strlen(testMessage) * 6)) scrollOffset = 32;
      }
    }
  }
  
  updateRows();

}

void updateRows() {
  for (int row = 0; row < 8; row++) {
    uint32_t redPattern = 0, greenPattern = 0;
    for (int col = 0; col < 32; col++) {
      if (redBuffer[row][col]) redPattern |= (1UL << (31 - col));
      if (greenBuffer[row][col]) greenPattern |= (1UL << (31 - col));
    }

    deactivateAllRows();
    sendPattern(0, 0);
    delayMicroseconds(1);
    sendPattern(redPattern, greenPattern);
    delayMicroseconds(1);
    activateRow(row);
    delayMicroseconds(ROW_ACTIVE_TIME_US);
    deactivateRow(row);
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
          fakeHour = h; fakeMinute = m; fakeSecond = s; testMode = false;
        }
      } else if (input.startsWith("TEXT ")) {
        strncpy(testMessage, input.substring(5).c_str(), sizeof(testMessage) - 1);
        testMode = true;
      }
      Serial.println("Serial: " + input);
      input = "";
    } else {
      input += c;
    }
  }
}

void setPixel(uint8_t x, uint8_t y, uint8_t color) {
  if (x >= 32 || y >= 8) return;
  redBuffer[y][x] = (color & 1);
  greenBuffer[y][x] = (color & 2) >> 1;
}

void drawChar(uint8_t x, uint8_t y, char c, uint8_t color) {
  if (c < 32 || c > 126) return;
  uint8_t index = c - 32;
  for (uint8_t col = 0; col < 5; col++) {
    uint8_t line = font5x7[index][col];
    for (uint8_t row = 0; row < 7; row++) {
      if (line & (1 << row)) setPixel(x + col, y + row, color);
    }
  }
}

void drawText(uint8_t x, uint8_t y, const char* str, uint8_t color) {
  memset(redBuffer, 0, sizeof(redBuffer));
  memset(greenBuffer, 0, sizeof(greenBuffer));
  while (*str && x < 32) {
    drawChar(x, y, *str, color);
    x += 6;
    str++;
  }
}

void drawClockDigits(int hour, int minute, int second) {
  memset(redBuffer, 0, sizeof(redBuffer));
  memset(greenBuffer, 0, sizeof(greenBuffer));
  drawChar(0 + startPos, 0, '0' + hour / 10, 2);
  drawChar(5 + startPos, 0, '0' + hour % 10, 2);
  if (colonVisible) drawChar(9 + startPos, 0, ':', 1);
  drawChar(10 + startPos, 0, '0' + minute / 10, 2);
  drawChar(15 + startPos, 0, '0' + minute % 10, 2);
  if (colonVisible) drawChar(19 + startPos, 0, ':', 1);
  drawChar(20 + startPos, 0, '0' + second / 10, 2);
  drawChar(25 + startPos, 0, '0' + second % 10, 2);
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

void handleOTA() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });

  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      Serial.println("Writing file: " + String(upload.currentSize) + " Upload total size: " + String(upload.totalSize));
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.begin();

  Serial.println("Web Server Started!");
}
