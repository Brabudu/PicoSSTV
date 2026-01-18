
#include <Arduino.h>
#include "ili934x.h"

// Copyright (c) Francesco Capuzzi 2025

const char keyboard[4][12] = {
  { 0, 12, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' },
  { 0, 12, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P' },
  { 1, 11, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L' },
  { 1, 11, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '/', '?' },
};

const uint16_t palette[16] = {
  0x0001, COLOUR_WHITE, COLOUR_RED, COLOUR_CYAN, COLOUR_MAGENTA, COLOUR_GREEN, COLOUR_BLUE, COLOUR_YELLOW,
  COLOUR_ORANGE, COLOUR_MAROON, COLOUR_PINK, COLOUR_DARKGREY, COLOUR_GREY, COLOUR_AQUA, COLOUR_LIME, COLOUR_LIGHTGREY
};

enum k_mode {
  mode_keys,
  mode_rsv,
  mode_color
};

class touch_keyboard {
  k_mode mode = mode_keys;

  void draw_kb_button_c(int x, int y, int w, int h, const char c);
  void draw_kb_button(int x, int y, int w, int h, const char* c);
  void draw_color_button(int x, int y, int w, int h, const uint16_t color);

public:
  void make_kb();
  void make_rsv_kb(const char* rsv[], int count);
  void make_color_kb();
  char get_key_press();
};