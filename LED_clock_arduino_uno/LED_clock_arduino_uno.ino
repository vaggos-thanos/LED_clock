// ====== LED Clock Display on 4x 8x8 Dual-Color Matrix ======
// Author: ChatGPT + Vaggos
// Description: Real-time clock display with blinking colon on 32x8 dual-color LED matrix

#include <Arduino.h>

// Pin definitions
const int dataPin = 4;
const int clockPinRed = 2;
const int clockPinGreen = 3;
const int strobePin = 5;
const int rowPins[8] = {6, 7, 8, 9, 10, 11, 12, 13};

// Font data (5x7, ASCII 32–126 only)
#include "font5x7.h"

// Buffers to hold pixel states (0/1)
uint8_t redBuffer[8][32] = {0};
uint8_t greenBuffer[8][32] = {0};

bool colonVisible = true;
unsigned long lastBlink = 0;
const unsigned long blinkInterval = 500;

// Simulated time for testing (no RTC)
unsigned long lastTimeUpdate = 0;
unsigned long lastSecondUpdate = 0;
const unsigned long timeInterval = 60000;
int fakeHour = 0;
int fakeMinute = 0;
int fakeSecond = 0;

// Start Pos x
int startPos = 1;

// Test mode
bool testMode = false;
char testMessage[32] = "";

// === Setup ===
void setup() {
  Serial.begin(9600);

  pinMode(dataPin, OUTPUT);
  pinMode(clockPinRed, OUTPUT);
  pinMode(clockPinGreen, OUTPUT);
  pinMode(strobePin, OUTPUT);

  for (int i = 0; i < 8; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], LOW);
  }

  Serial.println("Ready. Use 'TIME HH:MM:SS' or 'TEXT your message'");
}

// === Main Loop ===
const unsigned int ROW_ACTIVE_TIME_US = 350;
void loop() {
  static uint8_t scrollOffset = 0;
  static unsigned long lastScroll = 0;
  const unsigned long scrollDelay = 100;
  handleSerial();

  if (!testMode) {
    // Blink colon
    if (millis() - lastBlink >= blinkInterval) {
      colonVisible = !colonVisible;
      lastBlink = millis();
    }

    // Fake time increment every second
    if (millis() - lastSecondUpdate >= 1000) {
      lastSecondUpdate = millis();
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
    }

    drawClockDigits(fakeHour, fakeMinute, fakeSecond);
  } else {
    drawText(scrollOffset, 0, testMessage, 3); // Yellow scrolling text
    if (millis() - lastScroll > scrollDelay) {
      lastScroll = millis();
      scrollOffset--;
      if (scrollOffset < -((int)strlen(testMessage) * 6)) scrollOffset = 32;
    }
  }

  // Display framebuffer
  for (int row = 0; row < 8; row++) {
    uint32_t redPattern = 0;
    uint32_t greenPattern = 0;

    for (int col = 0; col < 32; col++) {
      if (redBuffer[row][col]) redPattern |= (1UL << (31 - col));
      if (greenBuffer[row][col]) greenPattern |= (1UL << (31 - col));
    }

    deactivateAllRows();                  // Ensure all rows are off before data changes
    sendPattern(0, 0);                    // Clear column lines
    delayMicroseconds(1);               // Let lines discharge

    sendPattern(redPattern, greenPattern); // Load next row pattern
    delayMicroseconds(1);               // Settle time to avoid ghosting

    activateRow(row);                    // Enable only one row
    delayMicroseconds(ROW_ACTIVE_TIME_US);  // Row hold time controls brightness
    deactivateRow(row);                  // Disable before next
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
        int s = input.substring(10, 12).toInt();

        if (h >= 0 && h < 24 && m >= 0 && m < 60 && s >= 0 && s <= 60) {
          fakeHour = h;
          fakeMinute = m;
          fakeSecond = s;
          testMode = false;
          Serial.println("Clock set to " + String(h) + ":" + String(m) + ":" + String(s));
        }
      } else if (input.startsWith("TEXT ")) {
        strncpy(testMessage, input.substring(5).c_str(), sizeof(testMessage) - 1);
        testMode = true;
        Serial.println("Displaying text: " + String(testMessage));
      }
      input = "";
    } else {
      input += c;
    }
  }
}

// === Draw Pixel ===
void setPixel(uint8_t x, uint8_t y, uint8_t color) {
  if (x >= 32 || y >= 8) return;

  switch (color) {
    case 1: redBuffer[y][x] = 1; greenBuffer[y][x] = 0; break;
    case 2: redBuffer[y][x] = 0; greenBuffer[y][x] = 1; break;
    case 3: redBuffer[y][x] = 1; greenBuffer[y][x] = 1; break;
    default: redBuffer[y][x] = 0; greenBuffer[y][x] = 0; break;
  }
}

// === Draw Character ===
void drawChar(uint8_t x, uint8_t y, char c, uint8_t color) {
  if (c < 32 || c > 126) return;
  uint8_t index = c - 32;
  for (uint8_t col = 0; col < 5; col++) {
    uint8_t line = font5x7[index][col];
    for (uint8_t row = 0; row < 7; row++) {
      if (line & (1 << row)) {
        setPixel(x + col, y + row, color);
      }
    }
  }
}

// === Draw Text ===
void drawText(uint8_t x, uint8_t y, const char* str, uint8_t color) {
  memset(redBuffer, 0, sizeof(redBuffer));
  memset(greenBuffer, 0, sizeof(greenBuffer));
  while (*str && x < 32) {
    drawChar(x, y, *str, color);
    x += 6;
    str++;
  }
}

// === Draw Time Digits ===
void drawClockDigits(int hour, int minute, int second) {
  memset(redBuffer, 0, sizeof(redBuffer));
  memset(greenBuffer, 0, sizeof(greenBuffer));

  char h1 = '0' + hour / 10;
  char h2 = '0' + hour % 10;
  char m1 = '0' + minute / 10;
  char m2 = '0' + minute % 10;
  char s1 = '0' + second / 10;
  char s2 = '0' + second % 10;

  drawChar(0 + startPos, 0, h1, 2);
  drawChar(5 + startPos, 0, h2, 2);
  if (colonVisible) drawChar(9 + startPos, 0, ':', 1);
  drawChar(10 + startPos, 0, m1, 2);
  drawChar(15 + startPos, 0, m2, 2);
  if (colonVisible) drawChar(19 + startPos, 0, ':', 1);
  drawChar(20 + startPos, 0, s1, 2);
  drawChar(25 + startPos, 0, s2, 2);
}

// === Shift Data ===
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

// === Row Control ===
void activateRow(int row) { digitalWrite(rowPins[row], HIGH); }
void deactivateRow(int row) { digitalWrite(rowPins[row], LOW); }
void activateAllRows() { for (int i = 0; i < 8; i++) digitalWrite(rowPins[i], HIGH); }
void deactivateAllRows() { for (int i = 0; i < 8; i++) digitalWrite(rowPins[i], LOW); }
