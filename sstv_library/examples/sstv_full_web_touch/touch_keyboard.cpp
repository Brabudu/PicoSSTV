// Copyright (c) Francesco Capuzzi 2025 IS0JSV
//Thanks to Adafruit forums member Asteroid and Andrew Mascolo Jr for the original sketch!

// MIT License

#include "touch_keyboard.h"
#include "XPT2046_Bitbang.h"
#include "ili934x.h"
#include "font_16x12.h"
#include <Arduino.h>

#define IsWithin(x, a, b) ((x >= a) && (x <= b))
#define TS_MINX 10
#define TS_MINY 10
#define TS_MAXX 300
#define TS_MAXY 190

#define MARGIN_TOP 50

extern ILI934X* display;
extern XPT2046_Bitbang touchscreen;
extern Stream* s;


void touch_keyboard ::draw_kb_button_c(int x, int y, int w, int h, const char c) {
  display->fillRoundedRect(x - 3, y + 3, h, w, 3, COLOUR_GREY);                  //Button Shading
  display->fillRoundedRect(x, y, h, w, 3, COLOUR_WHITE);                         // outter button color
  display->fillRoundedRect(x + 1, y + 1, h - 1 * 2, w - 1 * 2, 3, COLOUR_BLUE);  //inner button color
  display->drawChar(x + 5, y + 5, font_16x12, c, COLOUR_ORANGE, COLOUR_BLUE);
}

void touch_keyboard ::draw_color_button(int x, int y, int w, int h, const uint16_t color) {
  display->fillRoundedRect(x - 3, y + 3, h, w, 3, COLOUR_GREY);            //Button Shading
  display->fillRoundedRect(x, y, h, w, 3, COLOUR_WHITE);                   // outter button color
  display->fillRoundedRect(x + 1, y + 1, h - 1 * 2, w - 1 * 2, 3, color);  //inner button color
}

void touch_keyboard ::draw_kb_button(int x, int y, int w, int h, const char* c) {
  display->fillRoundedRect(x - 3, y + 3, h, w, 3, COLOUR_GREY);                  //Button Shading
  display->fillRoundedRect(x, y, h, w, 3, COLOUR_WHITE);                         // outter button color
  display->fillRoundedRect(x + 1, y + 1, h - 1 * 2, w - 1 * 2, 3, COLOUR_BLUE);  //inner button color
  display->drawString(x + 5, y + 5, font_16x12, c, COLOUR_ORANGE, COLOUR_BLUE);
}

void touch_keyboard ::make_kb() {
  for (int y = 0; y < 4; y++) {
    int ShiftRight = 15 * keyboard[y][0];
    for (int x = 2; x < 12; x++) {
      if (x >= keyboard[y][1]) break;

      draw_kb_button_c(15 + (30 * (x - 2)) + ShiftRight, MARGIN_TOP + (30 * y), 20, 25, keyboard[y][x]);  // this will draw the button on the screen by so many pixels
    }
  }

  draw_kb_button(15 + 20, MARGIN_TOP + (30 * 4), 40, 25, "<-");
  draw_kb_button(15 + 70, MARGIN_TOP + (30 * 4), 20, 25, "_");
  draw_kb_button(15 + 110, MARGIN_TOP + (30 * 4), 70, 25, "Clear");
  draw_kb_button(15 + 190, MARGIN_TOP + (30 * 4), 80, 25, "Enter");

  mode = mode_keys;
}

void touch_keyboard ::make_rsv_kb(const char* rsv[], int count) {

  for (int i = 0; i < count; i++) {
    draw_kb_button(60 + 70 * (i % 3), MARGIN_TOP + 35 * (i / 3), 50, 25, rsv[i]);
  }

  mode = mode_rsv;
}

bool touch_button(int x, int y, int w, int h, int X, int Y) {
  return (IsWithin(X, x, x + w) & IsWithin(Y, y, y + h));
}


char touch_keyboard ::get_key_press() {
  char key = 0;
  static uint8_t count = 0;
  static int mX, mY;

  static char last_char = '-';
  int ShiftRight = 0;


  int X, Y;
  // Retrieve a point
  TouchPoint p = touchscreen.getTouch();
  X = p.x;
  Y = p.y;

  if (p.zRaw < 600) {
    count = 0;
    mX = 0;
    mY = 0;
    return '-';
  }

  if (mX == 0) mX = X;
  if (mY == 0) mY = Y;

  mX = (mX + X) / 2;
  mY = (mY + Y) / 2;

  count++;

  if (count < 5) return '-';
  count = 0;

  //Map touch for calibration

  //X = map(mX, TS_MINX, TS_MAXX, 0, 320);
  //Y = map(mY, TS_MINY, TS_MAXY, 0, 200);

  //display->drawCircle(X,Y,3,COLOUR_WHITE); //For calibration purpose

  switch (mode) {
    case mode_keys:
      {
        //bs
        if (touch_button(35, MARGIN_TOP + (30 * 4), 40, 25, X, Y)) {
          return '<';
        }

        if (touch_button(85, MARGIN_TOP + (30 * 4), 20, 25, X, Y)) {
          return '_';
        }

        //clear
        if (touch_button(125, MARGIN_TOP + (30 * 4), 70, 25, X, Y)) {
          return '!';
        }

        if (touch_button(205, MARGIN_TOP + (30 * 4), 80, 25, X, Y)) {
          return '#';
        }

        for (int y = 0; y < 4; y++) {
          ShiftRight = 15 * keyboard[y][0];

          for (int x = 2; x < 12; x++) {
            if (x >= keyboard[y][1]) break;

            if (touch_button(15 + (30 * (x - 2)) + ShiftRight, MARGIN_TOP + (30 * y), 20, 25, X, Y))  // this will draw the button on the screen by so many pixels
            {
              return keyboard[y][x];
              break;
            }
          }
        }
      }
      break;
    case mode_rsv:
      {
        for (int i = 0; i < 9; i++) {
          if (touch_button(60 + 70 * (i % 3), MARGIN_TOP + 35 * (i / 3), 50, 25, X, Y)) return i;
        }
      }
      break;
    case mode_color:
      {
        for (int i = 0; i < 16; i++) {
          if (touch_button(30 + 70 * (i % 4), MARGIN_TOP + 35 * (i / 4), 50, 25, X, Y)) return i;
        }
      }
      break;
  }
  return '-';
}

void touch_keyboard ::make_color_kb() {
  

  for (int i = 0; i < 16; i++) {
    draw_color_button(30 + 70 * (i % 4), MARGIN_TOP + 35 * (i / 4), 50, 25, palette[i]);
  }

  mode = mode_color;
}
