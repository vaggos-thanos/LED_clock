/* font5x7_display_driver.h
 *
 * Bitmapped font table and display functions for 8x8 LED matrix
 * Supports digits, letters, symbols (ASCII 32–126), centered in 8x8 grid
 */

#ifndef FONT5X7_DISPLAY_DRIVER_H
#define FONT5X7_DISPLAY_DRIVER_H

#include <stdint.h>

// Font parameters
#define FONT5X7_WIDTH 5
#define FONT5X7_HEIGHT 7
#define FONT5X7_FIRST_CHAR 32
#define FONT5X7_LAST_CHAR 126
#define FONT5X7_CHAR_COUNT (FONT5X7_LAST_CHAR - FONT5X7_FIRST_CHAR + 1)

// Font data: each character is 5 bytes, each byte is one vertical column (top to bottom)
const uint8_t font5x7[FONT5X7_CHAR_COUNT][FONT5X7_WIDTH] = {
  // 32 ' '
  {0x00,0x00,0x00,0x00,0x00},  // 33 '!'
  {0x00,0x00,0x5F,0x00,0x00},  // 34 '"'
  {0x00,0x07,0x00,0x07,0x00},  // 35 '#'
  {0x14,0x7F,0x14,0x7F,0x14},  // 36 '$'
  {0x24,0x2A,0x7F,0x2A,0x12},  // 37 '%'
  {0x23,0x13,0x08,0x64,0x62},  // 38 '&'
  {0x36,0x49,0x55,0x22,0x50},  // 39 '''
  {0x00,0x05,0x03,0x00,0x00},  // 40 '('
  {0x00,0x1C,0x22,0x41,0x00},  // 41 ')'
  {0x00,0x41,0x22,0x1C,0x00},  // 42 '*'
  {0x14,0x08,0x3E,0x08,0x14},  // 43 '+'
  {0x08,0x08,0x3E,0x08,0x08},  // 44 ','
  {0x00,0x50,0x30,0x00,0x00},  // 45 '-'
  {0x08,0x08,0x08,0x08,0x08},  // 46 '.'
  {0x40,0x00,0x00,0x00,0x00},  // 47 '/'
  {0x20,0x10,0x08,0x04,0x02},

  // Digits 0-9
  {0x3c,0x42,0x42,0x3c,0x00}, // '0'
  {0x00,0x44,0x7e,0x40,0x00}, // '1'
  {0x44,0x62,0x52,0x4c,0x00}, // '2'
  {0x22,0x4a,0x56,0x22,0x00}, // '3'
  {0x18,0x14,0x7e,0x10,0x00}, // '4'
  {0x2e,0x4a,0x4a,0x32,0x00}, // '5'
  {0x38,0x54,0x52,0x30,0x00}, // '6'
  {0x02,0x72,0x0a,0x06,0x00}, // '7'
  {0x24,0x5a,0x5a,0x24,0x00}, // '8'
  {0x04,0x4a,0x2a,0x1c,0x00}, // '9'

  // Symbols
  {0x66,0x00,0x00,0x00,0x00}, // ':'
  {0x00,0x56,0x36,0x00,0x00}, // ';'
  {0x08,0x14,0x22,0x41,0x00}, // '<'
  {0x14,0x14,0x14,0x14,0x14}, // '='
  {0x00,0x41,0x22,0x14,0x08}, // '>'
  {0x02,0x01,0x51,0x09,0x06}, // '?'
  {0x32,0x49,0x79,0x41,0x3E}, // '@'

  // A-Z uppercase
  {0x7c,0x12,0x12,0x7c,0x00}, // 'A'
  {0x7e,0x4a,0x4a,0x34,0x00}, // 'B'
  {0x3c,0x42,0x42,0x24,0x00}, // 'C'
  {0x7e,0x42,0x42,0x3c,0x00}, // 'D'
  {0x7e,0x4a,0x4a,0x42,0x00}, // 'E'
  {0x7e,0x0a,0x0a,0x02,0x00}, // 'F'
  {0x3c,0x42,0x52,0x34,0x00}, // 'G'
  {0x7e,0x08,0x08,0x7e,0x00}, // 'H'
  {0x00,0x42,0x7e,0x42,0x00}, // 'I'
  {0x20,0x40,0x42,0x3e,0x00}, // 'J'
  {0x7e,0x08,0x14,0x62,0x00}, // 'K'
  {0x7e,0x40,0x40,0x40,0x00}, // 'L'
  {0x7e,0x04,0x08,0x04,0x7e}, // 'M'
  {0x7e,0x04,0x08,0x10,0x7e}, // 'N'
  {0x3c,0x42,0x42,0x3c,0x00}, // 'O'
  {0x7e,0x12,0x12,0x0c,0x00}, // 'P'
  {0x3c,0x42,0x52,0x6c,0x00}, // 'Q'
  {0x7e,0x12,0x32,0x4c,0x00}, // 'R'
  {0x24,0x4a,0x4a,0x30,0x00}, // 'S'
  {0x02,0x02,0x7e,0x02,0x02}, // 'T'
  {0x3e,0x40,0x40,0x3e,0x00}, // 'U'
  {0x1e,0x20,0x40,0x20,0x1e}, // 'V'
  {0x3e,0x40,0x30,0x40,0x3e}, // 'W'
  {0x66,0x18,0x18,0x66,0x00}, // 'X'
  {0x06,0x08,0x70,0x08,0x06}, // 'Y'
  {0x62,0x52,0x4a,0x46,0x00}, // 'Z'

  // Missing symbols [ \ ] ^ _ `
  {0x00,0x7E,0x42,0x00,0x00}, // '['
  {0x02,0x04,0x08,0x10,0x20}, // '\'
  {0x00,0x42,0x7E,0x00,0x00}, // ']'
  {0x04,0x02,0x01,0x02,0x04}, // '^'
  {0x40,0x40,0x40,0x40,0x40}, // '_'
  {0x00,0x01,0x02,0x04,0x00}, // '`'

  // a-z lowercase
  {0x20,0x54,0x54,0x78,0x00}, // 'a'
  {0x7E,0x48,0x48,0x30,0x00}, // 'b'
  {0x38,0x44,0x44,0x00,0x00}, // 'c'
  {0x30,0x48,0x48,0x7E,0x00}, // 'd'
  {0x38,0x54,0x54,0x18,0x00}, // 'e'
  {0x08,0x7C,0x0A,0x00,0x00}, // 'f'
  {0x18,0xA4,0xA4,0x7C,0x00}, // 'g'
  {0x7E,0x08,0x08,0x70,0x00}, // 'h'
  {0x00,0x48,0x7A,0x40,0x00}, // 'i'
  {0x40,0x80,0x88,0x7A,0x00}, // 'j'
  {0x7E,0x10,0x28,0x44,0x00}, // 'k'
  {0x00,0x42,0x7E,0x40,0x00}, // 'l'
  {0x7C,0x04,0x18,0x04,0x78}, // 'm'
  {0x7C,0x04,0x04,0x78,0x00}, // 'n'
  {0x38,0x44,0x44,0x38,0x00}, // 'o'
  {0xFC,0x24,0x24,0x18,0x00}, // 'p'
  {0x18,0x24,0x24,0xFC,0x00}, // 'q'
  {0x7C,0x08,0x04,0x00,0x00}, // 'r'
  {0x48,0x54,0x54,0x24,0x00}, // 's'
  {0x04,0x3E,0x44,0x00,0x00}, // 't'
  {0x3C,0x40,0x40,0x7C,0x00}, // 'u'
  {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
  {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
  {0x44,0x28,0x10,0x28,0x44}, // 'x'
  {0x1C,0xA0,0xA0,0x7C,0x00}, // 'y'
  {0x44,0x64,0x54,0x4C,0x00}, // 'z'
};

// Function prototypes
void drawChar(uint8_t x, uint8_t y, char c, uint8_t color);
void drawString(uint8_t x, uint8_t y, const char *str, uint8_t color);
void scrollText(const char *str, uint8_t speed, uint8_t color);
void setPixel(uint8_t x, uint8_t y, uint8_t color);

#endif
